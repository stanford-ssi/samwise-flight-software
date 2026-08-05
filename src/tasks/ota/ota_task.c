#include "ota_task.h"

#include "filesys.h"
#include "logger.h"
#include "mram.h"
#include "packet.h"
#include "pico/bootrom.h"
#include "rfm9x.h"

#include "hardware/flash.h"
#include "hardware/sync.h"

static void send_radio_msg(slate_t *slate, const char *msg)
{
    uint8_t data[PACKET_DATA_SIZE];
    int len = snprintf(data, sizeof(data), "%s", msg);
    packet_t pkt;
    rfm9x_format_packet(&pkt, 0, 0, 0, 0, len, &data[0]);
    if (!queue_try_add(&slate->tx_queue, &pkt))
    {
        LOG_ERROR("[ota_task] radio send failed: %s", msg);
    }
}

void ota_task_init(slate_t *slate)
{
}

void ota_task_dispatch(slate_t *slate)
{
    LOG_INFO("[ota_task] Starting OTA for file: %s", slate->ota_target_fname);

    // --- 1. Open file from filesystem ---
    // Feed watchdog before the CRC verification read inside
    // filesys_open_file_read, which reads the entire file once before
    // returning.
    watchdog_feed(&slate->watchdog);

    lfs_file_t file;
    filesys_file_info_t info;
    lfs_ssize_t lfs_err = LFS_ERR_OK;

    filesys_error_t err = filesys_open_file_read(
        slate, &file, slate->ota_target_fname, &info, &lfs_err);
    if (err != FILESYS_OK)
    {
        LOG_ERROR("[ota_task] File not found: %s (err=%d lfs=%d)",
                  slate->ota_target_fname, err, lfs_err);
        send_radio_msg(slate, "OTA ERR: file not found");
        slate->ota_requested = false;
        return;
    }

    FILESYS_BUFFERED_FILE_LEN_T file_size = info.file_size;
    LOG_INFO("[ota_task] File size: %u bytes", file_size);

    // --- 2. Find partition B offset from bootrom partition table ---
    uint32_t pt_buffer[128];
    uint32_t pt_flags = PT_INFO_PT_INFO | PT_INFO_PARTITION_LOCATION_AND_FLAGS;
    int pt_result = rom_get_partition_table_info(
        pt_buffer, sizeof(pt_buffer) / sizeof(uint32_t), pt_flags);

    if (pt_result < 0)
    {
        LOG_ERROR("[ota_task] Failed to read partition table: %d", pt_result);
        send_radio_msg(slate, "OTA ERR: partition table read failed");
        filesys_close_file_read(slate, &file, &lfs_err);
        slate->ota_requested = false;
        return;
    }

    // Use rom_get_boot_info to get the actual booted partition index.
    // The ota_mvp code used (sys_info_buf[1] & 0xF00) which reads the boot_type
    // byte instead of the partition byte — boot_info_t.partition is bits
    // [23:16].
    boot_info_t boot_info;
    if (!rom_get_boot_info(&boot_info))
    {
        LOG_ERROR("[ota_task] Failed to get boot info");
        send_radio_msg(slate, "OTA ERR: boot info read failed");
        filesys_close_file_read(slate, &file, &lfs_err);
        slate->ota_requested = false;
        return;
    }

    // boot_info_t.boot_word layout (packed):
    // [diagnostic|boot_type|partition|tbyb] partition is byte 2, bits [23:16]
    int current_partition = (int8_t)((boot_info.boot_word >> 16) & 0xFF);
    LOG_INFO("[ota_task] Currently booted partition: %d", current_partition);

    // rom_get_b_partition only accepts an A partition index.
    // If we're already on B, use the current partition directly.
    // TODO: this might be a source of error -- need to inspect docs + source
    // code to see if this is accurate.
    int b_partition_index = rom_get_b_partition((uint)current_partition);
    if (b_partition_index < 0)
    {
        // Current partition has no B partner — we're already on B.
        LOG_INFO("[ota_task] Already on partition B (%d), targeting self",
                 current_partition);
        b_partition_index = current_partition;
    }

    LOG_INFO("[ota_task] Target partition B index: %d", b_partition_index);

    // Partition table entries start at word 4, 2 words each.
    // Word 0 layout (from picobin.h): permissions[31:26] | last_sector[25:13] |
    // first_sector[12:0] Both sector fields are 4KB-sector indices; last_sector
    // is inclusive.
    uint32_t perms_and_loc = pt_buffer[4 + b_partition_index * 2];
    uint32_t b_first_sector = (perms_and_loc & 0x1FFFu);
    uint32_t b_last_sector = (perms_and_loc >> 13) & 0x1FFFu;
    uint32_t b_partition_offset = b_first_sector * 4096;
    uint32_t b_partition_size = (b_last_sector - b_first_sector + 1) * 4096;
    LOG_INFO("[ota_task] Partition B flash offset: 0x%08x  size: %u bytes",
             b_partition_offset, b_partition_size);

    if (file_size > b_partition_size)
    {
        LOG_ERROR("[ota_task] File (%u B) exceeds partition B (%u B)",
                  file_size, b_partition_size);
        send_radio_msg(slate, "OTA ERR: file too large for partition");
        filesys_close_file_read(slate, &file, &lfs_err);
        slate->ota_requested = false;
        return;
    }

    // --- 3. Erase partition B ---
    uint32_t erase_size =
        ((file_size + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE) *
        FLASH_SECTOR_SIZE;

    LOG_INFO("[ota_task] Erasing %u bytes at offset 0x%08x", erase_size,
             b_partition_offset);

    // Erase one sector at a time so we can feed the watchdog between each.
    // A single flash_range_erase over a large binary can take 6+ seconds
    // (128 sectors × ~50ms), exceeding the 5-second watchdog period.
    uint32_t ints;
    for (uint32_t erased = 0; erased < erase_size; erased += FLASH_SECTOR_SIZE)
    {
        watchdog_feed(&slate->watchdog);
        ints = save_and_disable_interrupts();
        mram_clear(b_partition_offset + erased, FLASH_SECTOR_SIZE);
        restore_interrupts(ints);
    }

    // --- 4. Program partition B in 256-byte pages from filesystem ---
    // Interrupts are restored coming out of the erase loop.
    uint8_t page_buf[FLASH_PAGE_SIZE];
    uint32_t bytes_written = 0;

    while (bytes_written < file_size)
    {
        memset(page_buf, 0xFF, FLASH_PAGE_SIZE);

        // Feed watchdog and read next page from MRAM (interrupts must be on)
        watchdog_feed(&slate->watchdog);

        FILESYS_BUFFERED_FILE_LEN_T bytes_read = 0;
        filesys_error_t read_err = filesys_read_data(
            slate, &file, page_buf, FLASH_PAGE_SIZE, &bytes_read, &lfs_err);

        if (read_err != FILESYS_OK || bytes_read == 0)
        {
            LOG_ERROR("[ota_task] Read failed at offset %u (err=%d lfs=%d)",
                      bytes_written, read_err, lfs_err);
            send_radio_msg(slate, "OTA ERR: filesystem read failed");
            filesys_close_file_read(slate, &file, &lfs_err);
            slate->ota_requested = false;
            return;
        }

        ints = save_and_disable_interrupts();
        mram_write(b_partition_offset + bytes_written, page_buf,
                   FLASH_PAGE_SIZE);
        restore_interrupts(ints);

        bytes_written += bytes_read;
    }

    filesys_close_file_read(slate, &file, &lfs_err);
    LOG_INFO("[ota_task] Flash write complete (%u bytes)", bytes_written);

    // --- 5. Notify ground and reboot into partition B ---
    send_radio_msg(slate, "OTA OK: rebooting");

    slate->ota_requested = false;

    int ret = rom_reboot(BOOT_TYPE_FLASH_UPDATE, 200,
                         XIP_BASE + b_partition_offset, 0);
    // rom_reboot should not return on success — if we get here it failed
    LOG_ERROR("[ota_task] rom_reboot failed: %d", ret);
    send_radio_msg(slate, "OTA ERR: rom_reboot failed");
    // ota_requested is already false so the state machine returns to RUNNING,
    // where radio_task will drain the error message on the next tick
    return;
}

sched_task_t ota_task = {.name = "ota",
                         .dispatch_period_ms = 0,
                         .task_init = &ota_task_init,
                         .task_dispatch = &ota_task_dispatch,
                         .next_dispatch = 0};
