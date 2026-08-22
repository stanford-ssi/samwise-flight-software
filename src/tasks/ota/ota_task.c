#include "ota_task.h"

#include "crc32.h"
#include "filesys.h"
#include "logger.h"
#include "ota_device.h"
#include "packet.h"
#include "pico/bootrom.h"
#include "radio_task.h"
#include "rfm9x.h"

#include "hardware/flash.h"
#include "hardware/sync.h"

/*
 * Constants from the pico-sdk picobin format
 * (src/common/boot_picobin_headers/include/boot/picobin.h).
 */
#define PICOBIN_BLOCK_MARKER_START 0xffffded3u
#define PICOBIN_BLOCK_ITEM_1BS_IMAGE_TYPE 0x42u
#define PICOBIN_IMAGE_TYPE_EXE_TBYB_BITS 0x8000u

// The start block lives early in the image; a max-size IMAGE_DEF block is
// 0x180 bytes, and the SDK emits the marker within the first few hundred.
#define OTA_HEADER_SCAN_BYTES 1024u

// Static rather than on the stack: the OTA task is not reentrant, and this
// runs in the scheduler's context.
static uint8_t ota_header_buf[OTA_HEADER_SCAN_BYTES];

/*
 * Report whether the image at partition_offset carries the Try-Before-You-Buy
 * flag.
 *
 * This is the assumption the whole OTA safety model rests on. SAMWISE never
 * calls rom_explicit_buy, so a TBYB image stays on probation indefinitely and
 * the bootrom reverts to partition A if it ever stops petting the watchdog.
 * An image *without* the flag is committed the moment it boots: if it hangs,
 * nothing brings the satellite back.
 *
 * Parses the picobin start block: locate PICOBIN_BLOCK_MARKER_START on a word
 * boundary, then walk the items looking for IMAGE_TYPE and test bit 0x8000.
 * Item headers encode the type in the low byte; bit 7 of the type selects a
 * 2-byte size field rather than 1-byte. Sizes are in words.
 *
 * Returns false if the flag is absent OR the header cannot be parsed -- an
 * unparseable image is not one to reboot into either.
 */
static bool ota_image_has_tbyb(uint32_t partition_offset, uint32_t image_size)
{
    uint32_t to_scan = OTA_HEADER_SCAN_BYTES;
    if (image_size < to_scan)
    {
        to_scan = image_size;
    }

    for (uint32_t off = 0; off < to_scan; off += FLASH_PAGE_SIZE)
    {
        uint32_t chunk = to_scan - off;
        if (chunk > FLASH_PAGE_SIZE)
        {
            chunk = FLASH_PAGE_SIZE;
        }
        if (!ota_dev_read_page(partition_offset + off, &ota_header_buf[off],
                               chunk))
        {
            LOG_ERROR("[ota_task] TBYB check: readback failed at %u", off);
            return false;
        }
    }

    for (uint32_t i = 0; i + 4 <= to_scan; i += 4)
    {
        uint32_t word;
        memcpy(&word, &ota_header_buf[i], sizeof(word));
        if (word != PICOBIN_BLOCK_MARKER_START)
        {
            continue;
        }

        // Walk the items following the marker.
        uint32_t p = i + 4;
        for (int item = 0; item < 16 && p + 4 <= to_scan; item++)
        {
            uint32_t hdr;
            memcpy(&hdr, &ota_header_buf[p], sizeof(hdr));

            uint32_t type = hdr & 0xFFu;
            uint32_t size_words =
                (type & 0x80u) ? ((hdr >> 8) & 0xFFFFu) : ((hdr >> 8) & 0xFFu);

            if (type == PICOBIN_BLOCK_ITEM_1BS_IMAGE_TYPE)
            {
                uint32_t flags = (hdr >> 16) & 0xFFFFu;
                LOG_INFO("[ota_task] Image type flags: 0x%04x", flags);
                return (flags & PICOBIN_IMAGE_TYPE_EXE_TBYB_BITS) != 0;
            }

            if (size_words == 0)
            {
                break; // malformed; avoid looping forever
            }
            p += size_words * 4;
        }
    }

    LOG_ERROR("[ota_task] TBYB check: no picobin IMAGE_TYPE item found");
    return false;
}

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

/*
 * Marker handed from partition B to partition A across a reboot, so an OTA
 * requested while running on B resumes by itself.
 *
 * B cannot perform the update (A is the golden image and is never written), so
 * it reboots to A. Without this, ground had to notice and resend the command -
 * a second contact pass. rom_reboot's two parameters survive the reboot and
 * reappear in boot_info.reboot_params[], which is enough to carry the request:
 * a magic value plus the 2-character filename.
 */
#define OTA_RESUME_MAGIC 0x07AC0DE5u

// Delay before the reboot, so the queued "OTA OK" packet finishes going out.
#define OTA_REBOOT_DELAY_MS 1000

void ota_task_init(slate_t *slate)
{
    boot_info_t boot_info;
    if (!rom_get_boot_info(&boot_info))
    {
        return;
    }

    if (boot_info.reboot_params[0] != OTA_RESUME_MAGIC)
    {
        return;
    }

    /*
     * Worst case if these params are stale from some unrelated reboot: we try
     * to OTA a file that does not exist and report "file not found". Harmless.
     */
    slate->ota_target_fname[0] = (char)(boot_info.reboot_params[1] & 0xFF);
    slate->ota_target_fname[1] =
        (char)((boot_info.reboot_params[1] >> 8) & 0xFF);
    slate->ota_target_fname[2] = '\0';
    slate->ota_requested = true;

    LOG_INFO("[ota_task] Resuming OTA for '%s' handed over from partition B",
             slate->ota_target_fname);
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
        /*
         * Running from B, which cannot update itself: A is the golden image
         * and is never written, so the update has to be driven from A. Reboot
         * there and carry the request in the reboot parameters, so A picks it
         * up in ota_task_init() without ground having to resend.
         */
        LOG_INFO("[ota_task] On partition B — rebooting to A to continue");
        send_radio_msg(slate, "OTA: on B, rebooting to A to continue");

        uint32_t fname_packed =
            (uint32_t)(uint8_t)slate->ota_target_fname[0] |
            ((uint32_t)(uint8_t)slate->ota_target_fname[1] << 8);

        slate->ota_requested = false;
        int ret =
            rom_reboot(BOOT_TYPE_NORMAL, 200, OTA_RESUME_MAGIC, fname_packed);
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

    // Running CRC over the bytes handed to the device, so step 6 can confirm
    // the device actually holds them. Not inverted until the comparison.
    unsigned int src_crc = 0xFFFFFFFFu;

    while (bytes_written < file_size)
    {
        memset(page_buf, 0xFF, FLASH_PAGE_SIZE);

        // Feed watchdog and read next page from MRAM (interrupts must be on)
        watchdog_feed(&slate->watchdog);

        /*
         * Fill a whole page before writing. The device can only take
         * page-sized writes, so advancing by a short read would make the next
         * write overlap the previous one - harmless on MRAM, but on NOR flash
         * that is a re-program without an erase, which corrupts.
         */
        uint32_t want = file_size - bytes_written;
        if (want > FLASH_PAGE_SIZE)
        {
            want = FLASH_PAGE_SIZE;
        }

        uint32_t page_filled = 0;
        while (page_filled < want)
        {
            FILESYS_BUFFERED_FILE_LEN_T bytes_read = 0;
            filesys_error_t read_err =
                filesys_read_data(slate, &file, &page_buf[page_filled],
                                  want - page_filled, &bytes_read, &lfs_err);

            if (read_err != FILESYS_OK || bytes_read == 0)
            {
                LOG_ERROR("[ota_task] Read failed at offset %u (err=%d lfs=%d)",
                          bytes_written + page_filled, read_err, lfs_err);
                send_radio_msg(slate, "OTA ERR: filesystem read failed");
                filesys_close_file_read(slate, &file, &lfs_err);
                slate->ota_requested = false;
                return;
            }

            page_filled += bytes_read;
        }

        ints = save_and_disable_interrupts();
        bool write_ok = ota_dev_write_page(b_partition_offset + bytes_written,
                                           page_buf, FLASH_PAGE_SIZE);
        restore_interrupts(ints);

        if (!write_ok)
        {
            LOG_ERROR("[ota_task] %s write failed at offset %u", ota_dev_name(),
                      bytes_written);
            send_radio_msg(slate, "OTA ERR: device write failed");
            filesys_close_file_read(slate, &file, &lfs_err);
            slate->ota_requested = false;
            return;
        }

        src_crc = crc32_continue(page_buf, page_filled, src_crc);
        bytes_written += page_filled;
    }

    filesys_close_file_read(slate, &file, &lfs_err);
    LOG_INFO("[ota_task] Flash write complete (%u bytes)", bytes_written);

    /*
     * --- 6. Verify partition B holds what we just wrote ---
     *
     * Neither device reports a bad write reliably: flash_range_program returns
     * void, and programming un-erased NOR flash silently yields garbage. The
     * source file was CRC-checked when it was opened, so re-reading the target
     * and comparing tells us the bytes actually landed.
     *
     * Catching it here means the satellite stays on A and can report the
     * failure on this same pass, rather than rebooting into a broken image and
     * waiting for its TBYB probation to lapse.
     */
    unsigned int dst_crc = 0xFFFFFFFFu;
    uint32_t verified = 0;

    while (verified < file_size)
    {
        uint32_t chunk = file_size - verified;
        if (chunk > FLASH_PAGE_SIZE)
        {
            chunk = FLASH_PAGE_SIZE;
        }

        watchdog_feed(&slate->watchdog);

        ints = save_and_disable_interrupts();
        bool read_ok =
            ota_dev_read_page(b_partition_offset + verified, page_buf, chunk);
        restore_interrupts(ints);

        if (!read_ok)
        {
            LOG_ERROR("[ota_task] %s readback failed at offset %u",
                      ota_dev_name(), verified);
            send_radio_msg(slate, "OTA ERR: verify read failed");
            slate->ota_requested = false;
            return;
        }

        dst_crc = crc32_continue(page_buf, chunk, dst_crc);
        verified += chunk;
    }

    if (dst_crc != src_crc)
    {
        // Do not reboot: partition A is still good, so stay on it.
        LOG_ERROR("[ota_task] Verify failed: wrote 0x%08x, read back 0x%08x",
                  ~src_crc, ~dst_crc);
        send_radio_msg(slate, "OTA ERR: verify failed, staying on A");
        slate->ota_requested = false;
        return;
    }

    LOG_INFO("[ota_task] Verified %u bytes in partition B (crc 0x%08x)",
             verified, ~dst_crc);

    /*
     * --- 7. Refuse to boot an image that is not marked Try-Before-You-Buy ---
     *
     * Rollback is what makes OTA survivable, and it only happens for images
     * built with PICO_CRT0_IMAGE_TYPE_TBYB=1. Without the flag the bootrom
     * commits to the image permanently on first boot; if it then hangs, there
     * is nothing to bring the satellite back to partition A.
     *
     * Partition A is untouched at this point, so refusing here costs nothing
     * but a resend.
     */
    watchdog_feed(&slate->watchdog);
    if (!ota_image_has_tbyb(b_partition_offset, file_size))
    {
        LOG_ERROR("[ota_task] Image is not TBYB; refusing to boot it");
        send_radio_msg(slate, "OTA ERR: image not TBYB, staying on A");
        slate->ota_requested = false;
        return;
    }

    /*
     * --- 8. Release the staged image ---
     *
     * It has been applied and verified, and the data partition is only 196 KiB
     * - leaving a firmware-sized file there crowds out telemetry and FTP. If B
     * later fails its probation the image has to be replaced anyway, so
     * keeping it would not save a re-upload.
     *
     * A failure here is not fatal: the image is already in partition B.
     */
    lfs_ssize_t del_err = LFS_ERR_OK;
    if (filesys_delete_file(slate, slate->ota_target_fname, &del_err) !=
        FILESYS_OK)
    {
        LOG_ERROR("[ota_task] Could not delete staged image %s (lfs=%d); "
                  "continuing",
                  slate->ota_target_fname, del_err);
    }

    // --- 9. Notify ground and reboot into partition B (TBYB) ---
    send_radio_msg(slate, "OTA OK: rebooting");

    /*
     * Queueing is not enough here. radio_task drains tx_queue on a scheduler
     * tick, and it runs *before* ota_task in this state, so the next tick
     * never arrives - we reboot first. The success message was therefore never
     * transmitted, leaving ground unable to tell a completed OTA from a dead
     * radio. Kick the transmit chain directly, and give it time to finish
     * before the reboot.
     */
    radio_task_dispatch(slate);

    slate->ota_requested = false;

    int ret = rom_reboot(BOOT_TYPE_FLASH_UPDATE, OTA_REBOOT_DELAY_MS,
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
