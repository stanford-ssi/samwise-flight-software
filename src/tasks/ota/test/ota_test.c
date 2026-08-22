/**
 * Unit tests for ota_task_dispatch.
 *
 * Each test calls ota_task_dispatch() against a controlled mock environment:
 *   - LittleFS on mock MRAM  (src/drivers/mram/mram_mock.c)
 *   - Bootrom mock           (src/test_mocks/pico/bootrom.c)
 *
 * Default bootrom mock layout (matches ota_mvp/pt.json sectors):
 *   partition 0 "A"  sectors 2..39   -> offset 0x02000, 152 KB
 *   partition 1 "B"  sectors 40..77  -> offset 0x28000, 152 KB
 *   partition 0 linked to partition 1 as its B partner
 *
 * CRC values precomputed with Python: zlib.crc32(data) & 0xFFFFFFFF
 *   256 x 0xAA  ->  0xafbd4cf6
 *   10000 x 0x42 -> 0x683480f5
 */

#include "ota_test.h"

// Mock-only hook, defined in src/drivers/mram/mram_mock.c
extern void mram_mock_set_corrupt_writes(bool enable);

#include "config.h"
#include "crc32.h"
#include "filesys.h"
#include "hardware/flash.h"
#include "mram.h"
#include "ota_task.h"
#include "pico/bootrom.h"

#include <stdint.h>
#include <string.h>

// CRC32 (zlib) of 256 bytes of 0xAA
static const uint32_t ota_fw_page_crc = 0xafbd4cf6u;

// CRC32 (zlib) of 10000 bytes of 0x42
#define OTA_LARGE_FW_SIZE 10000
static const uint32_t ota_large_fw_crc = 0x683480f5u;

// LFS filename used for the test firmware file
static const char ota_fw_fname[] = "FW";

// ============================================================================
// Helpers
// ============================================================================

// Write `size` bytes of `data` with the given `crc` into the filesystem under
// `ota_fw_fname`. Returns 0 on success or -1 on any filesystem error.

/*
 * Build a minimal but structurally valid image: 0xAA filler with a picobin
 * start block at offset 0 carrying an IMAGE_TYPE item.
 *
 * ota_task now parses this to decide whether an image is Try-Before-You-Buy,
 * so fixtures have to look like real images rather than arbitrary bytes.
 * The flag values were read out of real builds: --config=ota-blink produces
 * 0x9021, a normal build produces 0x1021, differing only in bit 0x8000.
 */
static void fill_fw_image(uint8_t *buf, uint32_t len, bool tbyb)
{
    memset(buf, 0xAA, len);

    // PICOBIN_BLOCK_MARKER_START, then an IMAGE_TYPE item: type in the low
    // byte, size 1 word, flags in the top half.
    uint32_t marker = 0xffffded3u;
    uint32_t flags = tbyb ? 0x9021u : 0x1021u;
    uint32_t item = 0x42u | (1u << 8) | (flags << 16);

    memcpy(&buf[0], &marker, sizeof(marker));
    memcpy(&buf[4], &item, sizeof(item));
}

static int write_firmware_to_fs(slate_t *slate, const uint8_t *data,
                                uint32_t size, uint32_t crc)
{
    lfs_ssize_t lfs_err;
    lfs_ssize_t blocks_left;

    FILESYS_BUFFERED_FNAME_STR_T fname;
    strncpy(fname, ota_fw_fname, sizeof(fname));

    filesys_error_t err = filesys_start_file_write(slate, fname, size, crc,
                                                   &lfs_err, &blocks_left);
    TEST_ASSERT(err == FILESYS_OK,
                "filesys_start_file_write failed: err=%d lfs=%d", err, lfs_err);

    for (uint32_t i = 0; i < size; i += FILESYS_BUFFER_SIZE)
    {
        FILESYS_BUFFER_SIZE_T chunk =
            (size - i) < (uint32_t)FILESYS_BUFFER_SIZE
                ? (FILESYS_BUFFER_SIZE_T)(size - i)
                : (FILESYS_BUFFER_SIZE_T)FILESYS_BUFFER_SIZE;

        err = filesys_write_data_to_buffer(slate, data + i, chunk, 0, &lfs_err);
        TEST_ASSERT(err == FILESYS_OK,
                    "filesys_write_data_to_buffer failed at offset %u: err=%d",
                    i, err);

        err = filesys_write_buffer_to_mram(slate, chunk, &lfs_err);
        TEST_ASSERT(err == FILESYS_OK,
                    "filesys_write_buffer_to_mram failed at offset %u: err=%d",
                    i, err);
    }

    err = filesys_complete_file_write(slate, &lfs_err);
    TEST_ASSERT(err == FILESYS_OK,
                "filesys_complete_file_write failed: err=%d lfs=%d", err,
                lfs_err);
    return 0;
}

// ============================================================================
// Per-test init: fresh filesystem + reset bootrom mock
// ============================================================================

int ota_test_setup(slate_t *slate)
{
    TEST_ASSERT(clear_and_init_slate(slate) == 0, "Failed to init slate");

    lfs_ssize_t lfs_err;
    filesys_error_t err = filesys_reformat_initialize(slate, &lfs_err);
    TEST_ASSERT(err == FILESYS_OK,
                "filesys_reformat_initialize failed: err=%d lfs=%d", err,
                lfs_err);

    bootrom_mock_reset();
    return 0;
}

// ============================================================================
// Test 1: Happy path — firmware written to partition B, BOOT_TYPE_FLASH_UPDATE
// ============================================================================
int ota_test_success(slate_t *slate)
{
    // Write one flash page (256 bytes of 0xAA) as the firmware file
    uint8_t fw[FLASH_PAGE_SIZE];
    fill_fw_image(fw, FLASH_PAGE_SIZE, true);

    if (write_firmware_to_fs(slate, fw, FLASH_PAGE_SIZE,
                             crc32(fw, FLASH_PAGE_SIZE)) != 0)
        return -1;

    strncpy(slate->ota_target_fname, ota_fw_fname,
            sizeof(slate->ota_target_fname));
    slate->ota_requested = true;
    // note: this does not test that we enter the OTA state machine, just that
    // the dispatch function works correctly

    ota_task_dispatch(slate); // we are in Partition A in this test.

    TEST_ASSERT(bootrom_mock_reboot_count == 1,
                "Expected exactly one reboot, got %d",
                bootrom_mock_reboot_count);
    TEST_ASSERT(bootrom_mock_last_reboot_flags ==
                    (uint32_t)BOOT_TYPE_FLASH_UPDATE,
                "Expected BOOT_TYPE_FLASH_UPDATE (%d), got %u",
                BOOT_TYPE_FLASH_UPDATE, bootrom_mock_last_reboot_flags);
    TEST_ASSERT(!slate->ota_requested,
                "ota_requested should be cleared after dispatch");

    // Default mock: partition B is sectors 40..77 -> offset = 40 * 4096 =
    // 0x28000
    uint32_t b_offset = 40u * 4096u;
    TEST_ASSERT(bootrom_mock_last_reboot_p0 == XIP_BASE + b_offset,
                "Reboot p0 should be XIP_BASE + 0x%05x (partition B), got 0x%x",
                b_offset, bootrom_mock_last_reboot_p0);

    // Verify firmware bytes were written to partition B in the MRAM mock
    uint8_t readback[FLASH_PAGE_SIZE];
    mram_read(b_offset, readback, FLASH_PAGE_SIZE);
    TEST_ASSERT(
        memcmp(readback, fw, FLASH_PAGE_SIZE) == 0,
        "Firmware bytes at partition B offset do not match what was written");

    return 0;
}

// ============================================================================
// Test 2: Currently running from partition B — must reboot to A
// ============================================================================
int ota_test_on_b_reboots_to_a(slate_t *slate)
{
    // Simulate running from partition B (index 1); rom_get_b_partition(1)
    // returns BOOTROM_ERROR_NOT_FOUND, signalling we are already on the B side.
    bootrom_mock_set_current_partition(1);

    strncpy(slate->ota_target_fname, ota_fw_fname,
            sizeof(slate->ota_target_fname));
    slate->ota_requested = true;

    ota_task_dispatch(slate);

    TEST_ASSERT(bootrom_mock_reboot_count == 1, "Expected one reboot to A");
    TEST_ASSERT(bootrom_mock_last_reboot_flags == (uint32_t)BOOT_TYPE_NORMAL,
                "Reboot from B must use BOOT_TYPE_NORMAL, got %u",
                bootrom_mock_last_reboot_flags);
    TEST_ASSERT(!slate->ota_requested, "ota_requested should be cleared");

    // Simulate the BOOT_TYPE_NORMAL reboot actually completing and verify
    // the mock lands on partition A with no TBYB state.
    bootrom_mock_simulate_boot();
    boot_info_t info;
    TEST_ASSERT(rom_get_boot_info(&info),
                "Should get boot info after simulated reboot");
    TEST_ASSERT(info.partition == 0,
                "Should be on partition A after BOOT_TYPE_NORMAL");
    TEST_ASSERT(
        !(info.tbyb_and_update_info & BOOT_TBYB_AND_UPDATE_FLAG_BUY_PENDING),
        "BUY_PENDING must not be set after a normal reboot to A");

    return 0;
}

// ============================================================================
// Test 3: rom_get_boot_info fails — should abort without rebooting
// ============================================================================
int ota_test_boot_info_failure(slate_t *slate)
{
    bootrom_mock_fail_boot_info(true);

    strncpy(slate->ota_target_fname, ota_fw_fname,
            sizeof(slate->ota_target_fname));
    slate->ota_requested = true;

    ota_task_dispatch(slate);

    TEST_ASSERT(bootrom_mock_reboot_count == 0,
                "Should not reboot when boot info read fails");
    TEST_ASSERT(!slate->ota_requested, "ota_requested should be cleared");

    return 0;
}

// ============================================================================
// Test 4: Firmware file is missing from filesystem — should abort
// ============================================================================
int ota_test_file_not_found(slate_t *slate)
{
    // Filesystem is empty (ota_test_setup left it freshly formatted)
    strncpy(slate->ota_target_fname, ota_fw_fname,
            sizeof(slate->ota_target_fname));
    slate->ota_requested = true;

    ota_task_dispatch(slate);

    TEST_ASSERT(bootrom_mock_reboot_count == 0,
                "Should not reboot when firmware file is not found");
    TEST_ASSERT(!slate->ota_requested, "ota_requested should be cleared");

    return 0;
}

// ============================================================================
// Test 5: Firmware file exceeds partition B — should abort before writing
// ============================================================================
int ota_test_file_too_large(slate_t *slate)
{
    // Replace the default partition layout with a tiny partition B (2 sectors =
    // 8192 bytes) so that the 10000-byte firmware file does not fit.
    bootrom_mock_clear_partitions();
    bootrom_mock_add_partition(2, 3); // A: sectors 2..3 = 8192 bytes
    bootrom_mock_add_partition(4, 5); // B: sectors 4..5 = 8192 bytes
    bootrom_mock_link_ab(0, 1);
    bootrom_mock_set_current_partition(0);

    // Write a 10000-byte firmware file (10000 > 8192 -> too large)
    static uint8_t big_fw[OTA_LARGE_FW_SIZE];
    memset(big_fw, 0x42, sizeof(big_fw));
    if (write_firmware_to_fs(slate, big_fw, OTA_LARGE_FW_SIZE,
                             ota_large_fw_crc) != 0)
        return -1;

    strncpy(slate->ota_target_fname, ota_fw_fname,
            sizeof(slate->ota_target_fname));
    slate->ota_requested = true;

    ota_task_dispatch(slate);

    TEST_ASSERT(bootrom_mock_reboot_count == 0,
                "Should not reboot when firmware file exceeds partition B");
    TEST_ASSERT(!slate->ota_requested, "ota_requested should be cleared");

    return 0;
}

// ============================================================================
// Test 6: rom_get_partition_table_info fails — should abort after opening file
// ============================================================================
int ota_test_partition_table_failure(slate_t *slate)
{
    // Write valid firmware so the dispatch gets past filesys_open_file_read
    uint8_t fw[FLASH_PAGE_SIZE];
    fill_fw_image(fw, FLASH_PAGE_SIZE, true);
    if (write_firmware_to_fs(slate, fw, FLASH_PAGE_SIZE,
                             crc32(fw, FLASH_PAGE_SIZE)) != 0)
        return -1;

    bootrom_mock_fail_partition_table_info(BOOTROM_ERROR_NOT_FOUND);

    strncpy(slate->ota_target_fname, ota_fw_fname,
            sizeof(slate->ota_target_fname));
    slate->ota_requested = true;

    ota_task_dispatch(slate);

    TEST_ASSERT(bootrom_mock_reboot_count == 0,
                "Should not reboot when partition table read fails");
    TEST_ASSERT(!slate->ota_requested, "ota_requested should be cleared");

    return 0;
}

// ============================================================================
// Test 7: After OTA + simulate_boot, boot_info reports B with BUY_PENDING
// ============================================================================
int ota_test_tbyb_state_after_ota(slate_t *slate)
{
    uint8_t fw[FLASH_PAGE_SIZE];
    fill_fw_image(fw, FLASH_PAGE_SIZE, true);
    if (write_firmware_to_fs(slate, fw, FLASH_PAGE_SIZE,
                             crc32(fw, FLASH_PAGE_SIZE)) != 0)
        return -1;

    strncpy(slate->ota_target_fname, ota_fw_fname,
            sizeof(slate->ota_target_fname));
    slate->ota_requested = true;
    ota_task_dispatch(slate);

    TEST_ASSERT(bootrom_mock_reboot_count == 1, "OTA should have rebooted");

    // Advance mock state to "satellite is now running on B in TBYB mode"
    bootrom_mock_simulate_boot();

    boot_info_t info;
    TEST_ASSERT(rom_get_boot_info(&info), "Should get boot info");
    TEST_ASSERT(info.partition == 1,
                "Should be on partition B after TBYB boot, got %d",
                info.partition);
    TEST_ASSERT(info.tbyb_and_update_info &
                    BOOT_TBYB_AND_UPDATE_FLAG_BUY_PENDING,
                "BUY_PENDING must be set when running in TBYB mode");

    return 0;
}

// ============================================================================
// Test 8: TBYB rollback — simulate_rollback returns to A, clears BUY_PENDING
// ============================================================================
int ota_test_tbyb_rollback_to_a(slate_t *slate)
{
    uint8_t fw[FLASH_PAGE_SIZE];
    fill_fw_image(fw, FLASH_PAGE_SIZE, true);
    if (write_firmware_to_fs(slate, fw, FLASH_PAGE_SIZE,
                             crc32(fw, FLASH_PAGE_SIZE)) != 0)
        return -1;

    strncpy(slate->ota_target_fname, ota_fw_fname,
            sizeof(slate->ota_target_fname));
    slate->ota_requested = true;
    ota_task_dispatch(slate);

    bootrom_mock_simulate_boot(); // now on B, BUY_PENDING

    // Watchdog timer expires — bootrom rolls back to A
    bootrom_mock_simulate_rollback();

    boot_info_t info;
    TEST_ASSERT(rom_get_boot_info(&info),
                "Should get boot info after rollback");
    TEST_ASSERT(info.partition == 0,
                "Should be back on partition A after rollback, got %d",
                info.partition);
    TEST_ASSERT(
        !(info.tbyb_and_update_info & BOOT_TBYB_AND_UPDATE_FLAG_BUY_PENDING),
        "BUY_PENDING must be cleared after rollback");

    return 0;
}

// ============================================================================
// Test 9: Full TBYB cycle — OTA to B, rollback to A, OTA to B again
// ============================================================================
int ota_test_tbyb_full_cycle(slate_t *slate)
{
    uint8_t fw[FLASH_PAGE_SIZE];
    fill_fw_image(fw, FLASH_PAGE_SIZE, true);
    if (write_firmware_to_fs(slate, fw, FLASH_PAGE_SIZE,
                             crc32(fw, FLASH_PAGE_SIZE)) != 0)
        return -1;

    // --- First OTA: A -> flash B -> boot B in TBYB ---
    strncpy(slate->ota_target_fname, ota_fw_fname,
            sizeof(slate->ota_target_fname));
    slate->ota_requested = true;
    ota_task_dispatch(slate);

    TEST_ASSERT(bootrom_mock_reboot_count == 1, "First OTA should reboot");
    TEST_ASSERT(bootrom_mock_last_reboot_flags ==
                    (uint32_t)BOOT_TYPE_FLASH_UPDATE,
                "First OTA must use BOOT_TYPE_FLASH_UPDATE");

    bootrom_mock_simulate_boot();     // satellite now on B, BUY_PENDING
    bootrom_mock_simulate_rollback(); // watchdog fires, rolls back to A

    /*
     * A successful OTA releases the staged image, so a second attempt needs
     * the file uploaded again -- which is what ground would do after seeing a
     * rollback, since the image that failed probation needs replacing anyway.
     */
    if (write_firmware_to_fs(slate, fw, FLASH_PAGE_SIZE,
                             crc32(fw, FLASH_PAGE_SIZE)) != 0)
        return -1;

    boot_info_t info;
    TEST_ASSERT(rom_get_boot_info(&info),
                "Should get boot info after rollback");
    TEST_ASSERT(info.partition == 0, "Should be on A after rollback");

    // --- Second OTA: A -> flash B again ---
    strncpy(slate->ota_target_fname, ota_fw_fname,
            sizeof(slate->ota_target_fname));
    slate->ota_requested = true;
    ota_task_dispatch(slate);

    TEST_ASSERT(bootrom_mock_reboot_count == 2,
                "Second OTA should produce a second reboot, got %d",
                bootrom_mock_reboot_count);
    TEST_ASSERT(bootrom_mock_last_reboot_flags ==
                    (uint32_t)BOOT_TYPE_FLASH_UPDATE,
                "Second OTA must also use BOOT_TYPE_FLASH_UPDATE");
    TEST_ASSERT(!slate->ota_requested, "ota_requested should be cleared");

    return 0;
}

// ============================================================================
// Test 10: Write silently corrupted — verification must refuse to reboot
// ============================================================================
// Models a device that accepts a write but does not hold it: mram_write
// returns true, yet the stored bytes are wrong. Neither the return value nor
// the source CRC catches this, so the post-write readback is the only thing
// that can. Getting this wrong means rebooting into a corrupt image.
int ota_test_verify_catches_corrupt_write(slate_t *slate)
{
    uint8_t fw[FLASH_PAGE_SIZE];
    fill_fw_image(fw, FLASH_PAGE_SIZE, true);

    if (write_firmware_to_fs(slate, fw, FLASH_PAGE_SIZE,
                             crc32(fw, FLASH_PAGE_SIZE)) != 0)
        return -1;

    strncpy(slate->ota_target_fname, ota_fw_fname,
            sizeof(slate->ota_target_fname));
    slate->ota_requested = true;

    mram_mock_set_corrupt_writes(true);
    ota_task_dispatch(slate);
    mram_mock_set_corrupt_writes(false);

    TEST_ASSERT(bootrom_mock_reboot_count == 0,
                "Must NOT reboot into an image that failed verification "
                "(reboot_count=%d)",
                bootrom_mock_reboot_count);
    TEST_ASSERT(!slate->ota_requested,
                "ota_requested should be cleared after a failed verify");

    return 0;
}

// ============================================================================
// Test 11: Image lacks the TBYB flag — must refuse to boot it
// ============================================================================
// Rollback only happens for images built with PICO_CRT0_IMAGE_TYPE_TBYB=1.
// Without it the bootrom commits to the image on first boot, so if it hangs
// there is nothing to bring the satellite back to partition A. The image here
// is byte-identical to the happy-path fixture except for bit 0x8000 in the
// picobin IMAGE_TYPE flags -- the same single bit that differs between a real
// --config=ota-blink build and a normal one.
int ota_test_rejects_non_tbyb_image(slate_t *slate)
{
    uint8_t fw[FLASH_PAGE_SIZE];
    fill_fw_image(fw, FLASH_PAGE_SIZE, false); // no TBYB flag

    if (write_firmware_to_fs(slate, fw, FLASH_PAGE_SIZE,
                             crc32(fw, FLASH_PAGE_SIZE)) != 0)
        return -1;

    strncpy(slate->ota_target_fname, ota_fw_fname,
            sizeof(slate->ota_target_fname));
    slate->ota_requested = true;

    ota_task_dispatch(slate);

    TEST_ASSERT(bootrom_mock_reboot_count == 0,
                "Must NOT boot an image without TBYB (reboot_count=%d)",
                bootrom_mock_reboot_count);
    TEST_ASSERT(!slate->ota_requested,
                "ota_requested should be cleared after refusing");

    return 0;
}

// ============================================================================
// Test 12: An OTA requested on B is handed to A and resumes by itself
// ============================================================================
// B cannot update itself - A is the golden image and is never written - so it
// reboots to A. Previously that lost the request and ground had to resend on a
// later pass. The request now rides across in rom_reboot's parameters and is
// picked up by ota_task_init().
int ota_test_resume_handover_from_b(slate_t *slate)
{
    // Pretend we are running from partition B and an OTA was commanded.
    bootrom_mock_set_current_partition(1);
    strncpy(slate->ota_target_fname, ota_fw_fname,
            sizeof(slate->ota_target_fname));
    slate->ota_requested = true;

    ota_task_dispatch(slate);

    TEST_ASSERT(bootrom_mock_reboot_count == 1, "Expected a reboot to A");
    TEST_ASSERT(bootrom_mock_last_reboot_flags == (uint32_t)BOOT_TYPE_NORMAL,
                "Handover reboot should be BOOT_TYPE_NORMAL");
    TEST_ASSERT(!slate->ota_requested,
                "ota_requested should be cleared before the reboot");

    // Now we are partition A, booting with those reboot parameters present.
    bootrom_mock_set_current_partition(0);
    ota_task_init(slate);

    TEST_ASSERT(slate->ota_requested,
                "A should have picked up the handed-over OTA request");
    TEST_ASSERT(strncmp(slate->ota_target_fname, ota_fw_fname,
                        sizeof(slate->ota_target_fname)) == 0,
                "Filename should survive the handover, got '%s'",
                slate->ota_target_fname);

    return 0;
}

// ============================================================================
// Test 13: A successful OTA releases the staged image
// ============================================================================
// The data partition is only 196 KiB, so a firmware-sized file left behind
// crowds out telemetry and FTP. If partition B later fails its probation the
// image has to be replaced anyway, so keeping it would not save a re-upload.
int ota_test_deletes_staged_image(slate_t *slate)
{
    uint8_t fw[FLASH_PAGE_SIZE];
    fill_fw_image(fw, FLASH_PAGE_SIZE, true);

    if (write_firmware_to_fs(slate, fw, FLASH_PAGE_SIZE,
                             crc32(fw, FLASH_PAGE_SIZE)) != 0)
        return -1;

    strncpy(slate->ota_target_fname, ota_fw_fname,
            sizeof(slate->ota_target_fname));
    slate->ota_requested = true;

    ota_task_dispatch(slate);

    TEST_ASSERT(bootrom_mock_reboot_count == 1,
                "OTA should have succeeded and rebooted");

    // The staged file should be gone: opening it must now fail.
    lfs_file_t file;
    filesys_file_info_t info;
    lfs_ssize_t lfs_err = 0;
    FILESYS_BUFFERED_FNAME_STR_T fname;
    strncpy(fname, ota_fw_fname, sizeof(fname));

    filesys_error_t err =
        filesys_open_file_read(slate, &file, fname, &info, &lfs_err);
    TEST_ASSERT(err != FILESYS_OK,
                "Staged image should have been deleted after a successful OTA");

    return 0;
}

// ============================================================================
// Test table
// ============================================================================

const test_harness_case_t ota_tests[] = {
    {0, ota_test_success,
     "OTA success: firmware flashed to B, BOOT_TYPE_FLASH_UPDATE"},
    {1, ota_test_on_b_reboots_to_a,
     "OTA on partition B: reboots to A with BOOT_TYPE_NORMAL"},
    {2, ota_test_boot_info_failure, "OTA aborts when rom_get_boot_info fails"},
    {3, ota_test_file_not_found,
     "OTA aborts when firmware file is missing from filesystem"},
    {4, ota_test_file_too_large,
     "OTA aborts when firmware exceeds partition B size"},
    {5, ota_test_partition_table_failure,
     "OTA aborts when rom_get_partition_table_info fails"},
    {6, ota_test_tbyb_state_after_ota,
     "TBYB: after OTA + simulate_boot, partition B with BUY_PENDING"},
    {7, ota_test_tbyb_rollback_to_a,
     "TBYB: simulate_rollback returns to partition A, clears BUY_PENDING"},
    {8, ota_test_tbyb_full_cycle,
     "TBYB: full cycle — OTA to B, rollback to A, OTA to B again"},
    {9, ota_test_verify_catches_corrupt_write,
     "Verify: corrupt write is detected and does not reboot"},
    {10, ota_test_rejects_non_tbyb_image,
     "TBYB: image without the TBYB flag is refused"},
    {11, ota_test_resume_handover_from_b,
     "Resume: OTA requested on B is handed to A and resumes"},
    {12, ota_test_deletes_staged_image,
     "Cleanup: staged image is deleted after a successful OTA"},
};

const size_t ota_tests_len = sizeof(ota_tests) / sizeof(ota_tests[0]);

int main(void)
{
    return test_harness_run("OTA", ota_tests, ota_tests_len, ota_test_setup);
}
