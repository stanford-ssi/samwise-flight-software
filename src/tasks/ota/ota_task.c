#include "ota_task.h"

#include "filesys.h"
#include "logger.h"
#include "ota_device.h"
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

    // --- 1. Check which partition we are running from ---
    watchdog_feed(&slate->watchdog);

    boot_info_t boot_info;
    if (!rom_get_boot_info(&boot_info))
    {
        LOG_ERROR("[ota_task] Failed to get boot info");
        send_radio_msg(slate, "OTA ERR: boot info read failed");
        slate->ota_requested = false;
        return;
    }

    // boot_word = 0xttppbbdd (MSB→LSB): tbyb[31:24] | partition[23:16] |
    // boot_type[15:8] | diagnostic[7:0]. We want the partition byte.
    int current_partition = (int8_t)((boot_info.boot_word >> 16) & 0xFF);
    LOG_INFO("[ota_task] Currently booted partition: %d", current_partition);

    // rom_get_b_partition returns the B index for a given A partition.
    // A negative return means we passed it a B partition index — i.e. we are
    // currently running from partition B.
    int b_partition_index = rom_get_b_partition((uint)current_partition);
    if (b_partition_index < 0)
    {
        // Running from B. Reboot to A; ground must resend the OTA command.
        LOG_INFO("[ota_task] On partition B — rebooting to A");
        send_radio_msg(slate, "OTA: on B, rebooting to A, resend command");
        slate->ota_requested = false;
        int ret = rom_reboot(BOOT_TYPE_NORMAL, 200, 0, 0);
        if (ret != BOOTROM_OK)
            LOG_ERROR("[ota_task] Reboot failed: %d", ret);
        return;
    }

    LOG_INFO("[ota_task] On partition A, writing to partition B (index %d)",
             b_partition_index);

    // --- 2. Open firmware file from filesystem ---
    // Feed watchdog before filesys_open_file_read, which reads the entire
    // file once for CRC verification before returning.
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

    // --- 3. Get partition B size from the bootrom partition table ---
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

    LOG_INFO("[ota_task] Target partition B index: %d", b_partition_index);

    // Partition table buffer layout (PT_INFO_PT_INFO = 3 words, then
    // PT_INFO_PARTITION_LOCATION_AND_FLAGS = 2 words per partition):
    //   buf[0]       = flags actually filled
    //   buf[1..3]    = PT_INFO (partition_count, unpartitioned location/flags)
    //   buf[4 + n*2] = permissions_and_location for partition n
    //   buf[5 + n*2] = permissions_and_flags for partition n
    // permissions_and_location bit layout (picobin.h):
    //   first_sector = bits [12:0], last_sector = bits [25:13] (inclusive)
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

    uint32_t ints;

    // --- 4. Erase partition B, if the boot device requires it ---
    // MRAM is written in place and skips this entirely. NOR flash must be
    // erased first, one sector at a time so the watchdog can be fed between
    // each -- a single erase across a whole partition takes several seconds
    // and would otherwise trip it.
    if (ota_dev_needs_erase())
    {
        uint32_t erase_size =
            ((file_size + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE) *
            FLASH_SECTOR_SIZE;

        LOG_INFO("[ota_task] Erasing %u bytes at offset 0x%08x on %s",
                 erase_size, b_partition_offset, ota_dev_name());

        for (uint32_t erased = 0; erased < erase_size;
             erased += FLASH_SECTOR_SIZE)
        {
            watchdog_feed(&slate->watchdog);
            ints = save_and_disable_interrupts();
            ota_dev_erase_sector(b_partition_offset + erased);
            restore_interrupts(ints);
        }
    }

    // --- 5. Program partition B in 256-byte pages from filesystem ---
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
        bool write_ok = ota_dev_write_page(b_partition_offset + bytes_written,
                                           page_buf, FLASH_PAGE_SIZE);
        restore_interrupts(ints);

        if (!write_ok)
        {
            LOG_ERROR("[ota_task] %s write failed at offset %u",
                      ota_dev_name(), bytes_written);
            send_radio_msg(slate, "OTA ERR: device write failed");
            filesys_close_file_read(slate, &file, &lfs_err);
            slate->ota_requested = false;
            return;
        }

        bytes_written += bytes_read;
    }

    filesys_close_file_read(slate, &file, &lfs_err);
    LOG_INFO("[ota_task] Flash write complete (%u bytes)", bytes_written);

    // --- 5. Notify ground and reboot into partition B (TBYB) ---
    send_radio_msg(slate, "OTA OK: rebooting");

    slate->ota_requested = false;

    int ret = rom_reboot(BOOT_TYPE_FLASH_UPDATE, 200,
                         XIP_BASE + b_partition_offset, 0);
    if (ret != BOOTROM_OK)
    {
        LOG_ERROR("[ota_task] rom_reboot failed: %d", ret);
        send_radio_msg(slate, "OTA ERR: rom_reboot failed");
    }
    return;
}

sched_task_t ota_task = {.name = "ota",
                         .dispatch_period_ms = 0,
                         .task_init = &ota_task_init,
                         .task_dispatch = &ota_task_dispatch,
                         .next_dispatch = 0};
