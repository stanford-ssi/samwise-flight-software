/***
 * @author Ayush Garg, Luis Cantoran, Joziah Uribe-Lopez
 * @date   2025-10-18
 *
 *  Test for mram driver
 */
#include "mram_test.h"

#ifdef TEST
#include "lfs_gen_flash_wrapper.h"
#endif

int mram_test_setup(slate_t *slate)
{
    memset(slate, 0, sizeof(slate_t));
    return 0;
}

int test_mram_write_read(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("Testing MRAM write...\n");
    TEST_ASSERT(mram_write(0x000000, (uint8_t *)"Hello, MRAM!", 13),
                "mram_write should succeed");

    LOG_DEBUG("Testing MRAM read...\n");

    uint8_t buffer[13];
    memset(buffer, 0x00, sizeof(buffer));
    mram_read(0x000000, buffer, 13);

    LOG_DEBUG("Read from MRAM: %s\n", buffer);
    LOG_DEBUG("Read hex:");
    for (int i = 0; i < 13; i++)
        LOG_DEBUG(" %02X", buffer[i]);
    LOG_DEBUG("\n");
    TEST_ASSERT(memcmp(buffer, "Hello, MRAM!", 13) == 0,
                "Read back should match written data");

    return 0;
}

int test_mram_write_disable_enable(slate_t *slate)
{
    (void)slate;
    const uint32_t test_addr = 0x000100;

    /* Verify that mram_write_disable doesn't block subsequent writes
       (mram_write calls mram_write_enable internally). */
    LOG_DEBUG("Testing MRAM write succeeds despite prior write-disable...\n");
    mram_write_disable();
    TEST_ASSERT(
        mram_write(test_addr, (const uint8_t *)"This should be written", 22),
        "Write should succeed despite prior write-disable");
    uint8_t verify_buf[22];
    mram_read(test_addr, verify_buf, 22);
    TEST_ASSERT(memcmp(verify_buf, "This should be written", 22) == 0,
                "Data should match after write despite prior write-disable");

    /* Verify write-disable then write-enable don't corrupt a subsequent
       write/read cycle. */
    LOG_DEBUG("Testing write cycle after disable/enable sequence...\n");
    mram_write_disable();
    mram_write_enable();
    TEST_ASSERT(
        mram_write(test_addr, (const uint8_t *)"After enable/disable", 21),
        "Write after enable/disable should succeed");
    mram_read(test_addr, verify_buf, 21);
    TEST_ASSERT(memcmp(verify_buf, "After enable/disable", 21) == 0,
                "Data should match after enable/disable cycle");
    return 0;
}

int test_mram_preserve_data_on_sleep(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("Testing MRAM preserve data across sleep/wake...\n");
    const char *test_str = "SleepTest123";
    size_t len = strlen(test_str) + 1; // include null terminator
    uint8_t read_buf[32];
    memset(read_buf, 0x00, sizeof(read_buf));
    TEST_ASSERT(mram_write(0x000200, (const uint8_t *)test_str, len),
                "Write before sleep should succeed");
    mram_read(0x000200, read_buf, len);
    TEST_ASSERT(memcmp(read_buf, test_str, len) == 0,
                "Read before sleep should match written data");
    mram_sleep();
    sleep_us(1000);
    mram_wake();
    memset(read_buf, 0x00, sizeof(read_buf));
    mram_read(0x000200, read_buf, len);
    TEST_ASSERT(memcmp(read_buf, test_str, len) == 0,
                "Data should be preserved after sleep/wake cycle");
    return 0;
}

int test_mram_clear(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("Testing MRAM clear...\n");
    const uint32_t addr = 0x000010;
    const size_t len = 16;
    /* Write non-zero data into MRAM region */
    uint8_t write_buf[len];
    for (size_t i = 0; i < len; i++)
    {
        write_buf[i] = (uint8_t)(i + 1);
    }
    TEST_ASSERT(mram_write(addr, write_buf, len), "Setup write should succeed");
    /* Clear the region */
    mram_clear(addr, len);
    /* Read back and ensure region is zeroed */
    uint8_t read_buf_c[len];
    memset(read_buf_c, 0xFF, len);
    mram_read(addr, read_buf_c, len);
    for (size_t i = 0; i < len; i++)
    {
        TEST_ASSERT(read_buf_c[i] == 0x00,
                    "Each byte should be zero after clear");
    }
    return 0;
}

int test_mram_read_status(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("Testing MRAM read status register...\n");

    /* Read status and log its value. On some hardware the MRAM always
       returns 0x00 for RDSR — this is a known chip-level behaviour,
       not a driver bug (see README.md). The test verifies the RDSR
       command doesn't hang or corrupt data, not the bit values. */
    uint8_t status = mram_read_status();
    LOG_DEBUG("MRAM status register: 0x%02X\n", status);

    /* Verify that RDSR doesn't interfere with normal read/write. */
    const uint32_t addr = 0x000C00;
    const uint8_t pattern[8] = {0xCA, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD, 0xBE, 0xEF};
    TEST_ASSERT(mram_write(addr, pattern, sizeof(pattern)),
                "Write after RDSR should succeed");
    uint8_t read_buf[8];
    mram_read(addr, read_buf, sizeof(read_buf));
    TEST_ASSERT(memcmp(read_buf, pattern, sizeof(pattern)) == 0,
                "Read after RDSR should match written data");
    return 0;
}

int test_mram_write_max_length_boundary(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Write max length boundary ===\n");
    const uint32_t addr = 0x000300;
    uint8_t write_buf[256];
    for (int i = 0; i < 256; i++)
    {
        write_buf[i] = (i % 2 == 0) ? 0x55 : 0xAA;
    }
    TEST_ASSERT(mram_write(addr, write_buf, 256),
                "Write of 256 bytes should succeed");
    uint8_t read_buf[256];
    mram_read(addr, read_buf, 256);
    TEST_ASSERT(memcmp(read_buf, write_buf, 256) == 0,
                "Read back should match written data for max-length write");
    return 0;
}

int test_mram_write_exceeds_max_length(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Write exceeds max length ===\n");
    uint8_t buf[257];
    memset(buf, 0xAB, sizeof(buf));
    TEST_ASSERT(!mram_write(0x000400, buf, 257),
                "Write of 257 bytes should return false");
    return 0;
}

int test_mram_clear_exceeds_max_length(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Clear exceeds max length ===\n");
    const uint32_t addr = 0x000500;
    const uint8_t pattern[16] = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD,
                                 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                 0xDE, 0xAD, 0xBE, 0xEF};
    TEST_ASSERT(mram_write(addr, pattern, sizeof(pattern)),
                "Setup write should succeed");
    mram_clear(addr, 257);
    uint8_t read_buf[16];
    mram_read(addr, read_buf, sizeof(read_buf));
    TEST_ASSERT(memcmp(read_buf, pattern, sizeof(pattern)) == 0,
                "Data should be unchanged after over-length clear");
    return 0;
}

int test_mram_write_overwrite(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Write overwrite ===\n");
    const uint32_t addr = 0x000600;
    const uint8_t first[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    const uint8_t second[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11};
    TEST_ASSERT(mram_write(addr, first, sizeof(first)),
                "First write should succeed");
    TEST_ASSERT(mram_write(addr, second, sizeof(second)),
                "Second (overwrite) should succeed");
    uint8_t read_buf[8];
    mram_read(addr, read_buf, sizeof(read_buf));
    TEST_ASSERT(memcmp(read_buf, second, sizeof(second)) == 0,
                "Read should return second (overwriting) pattern");
    return 0;
}

int test_mram_multiple_independent_regions(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Multiple independent regions ===\n");
    const uint32_t addr_a = 0x000700;
    const uint32_t addr_b = 0x000710;
    const uint32_t addr_c = 0x000720;
    const uint8_t data_a[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    const uint8_t data_b[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    const uint8_t data_c[8] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
    TEST_ASSERT(mram_write(addr_a, data_a, sizeof(data_a)),
                "Write A should succeed");
    TEST_ASSERT(mram_write(addr_b, data_b, sizeof(data_b)),
                "Write B should succeed");
    TEST_ASSERT(mram_write(addr_c, data_c, sizeof(data_c)),
                "Write C should succeed");
    uint8_t buf[8];
    mram_read(addr_a, buf, sizeof(buf));
    TEST_ASSERT(memcmp(buf, data_a, sizeof(data_a)) == 0,
                "Region A read should match data_a");
    mram_read(addr_b, buf, sizeof(buf));
    TEST_ASSERT(memcmp(buf, data_b, sizeof(data_b)) == 0,
                "Region B read should match data_b");
    mram_read(addr_c, buf, sizeof(buf));
    TEST_ASSERT(memcmp(buf, data_c, sizeof(data_c)) == 0,
                "Region C read should match data_c");
    return 0;
}

int test_mram_adjacent_regions_no_bleed(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Adjacent regions no bleed ===\n");
    const uint32_t addr_lo = 0x000800;
    const size_t len = 16;
    const uint32_t addr_hi = addr_lo + len;
    uint8_t lo_pattern[16];
    uint8_t hi_pattern[16];
    memset(lo_pattern, 0xAA, len);
    memset(hi_pattern, 0x55, len);
    TEST_ASSERT(mram_write(addr_lo, lo_pattern, len),
                "Low region write should succeed");
    TEST_ASSERT(mram_write(addr_hi, hi_pattern, len),
                "High region write should succeed");
    uint8_t read_lo[16];
    uint8_t read_hi[16];
    mram_read(addr_lo, read_lo, len);
    mram_read(addr_hi, read_hi, len);
    TEST_ASSERT(memcmp(read_lo, lo_pattern, len) == 0,
                "Low region should hold 0xAA pattern after adjacent write");
    TEST_ASSERT(memcmp(read_hi, hi_pattern, len) == 0,
                "High region should hold 0x55 pattern");
    return 0;
}

int test_mram_full_byte_range(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Full byte range write/read ===\n");
    const uint32_t addr = 0x000900;
    uint8_t write_buf[256];
    for (int i = 0; i < 256; i++)
    {
        write_buf[i] = (uint8_t)i;
    }
    TEST_ASSERT(mram_write(addr, write_buf, 256),
                "Full-range write should succeed");
    uint8_t read_buf[256];
    mram_read(addr, read_buf, 256);
    TEST_ASSERT(memcmp(read_buf, write_buf, 256) == 0,
                "All 256 byte values should round-trip correctly");
    return 0;
}

int test_mram_single_byte_write_read(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Single byte write/read ===\n");
    const uint32_t addr = 0x000A00;
    const uint8_t byte_val = 0x7E;
    TEST_ASSERT(mram_write(addr, &byte_val, 1),
                "Single-byte write should succeed");
    uint8_t result = 0x00;
    mram_read(addr, &result, 1);
    TEST_ASSERT(result == byte_val,
                "Single-byte read should match written value");
    return 0;
}

int test_mram_large_write_read(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Large write/read ===\n");
    const uint32_t addr = 0x000B00;
    const size_t large_size = 256; // Max allowed size
    uint8_t write_buf[large_size];
    for (size_t i = 0; i < large_size; i++)
    {
        write_buf[i] = (uint8_t)(i & 0xFF);
    }
    TEST_ASSERT(mram_write(addr, write_buf, large_size),
                "Large write should succeed");
    uint8_t read_buf[large_size];
    mram_read(addr, read_buf, large_size);
    TEST_ASSERT(memcmp(read_buf, write_buf, large_size) == 0,
                "Large read should match written data");
    return 0;
}

#ifdef TEST
// ============================================================================
// Flash wrapper mock tests
// ============================================================================

static const struct lfs_config test_flash_cfg = {
    .read = lfs_gen_flash_wrap_read,
    .prog = lfs_gen_flash_wrap_prog,
    .erase = lfs_gen_flash_wrap_erase,
    .sync = lfs_gen_flash_wrap_sync,
    .read_size = 16,
    .prog_size = 16,
    .block_size = 4096,
    .block_count = (1024 * 1024) / 4096,
    .cache_size = 256,
    .lookahead_size = 16,
    .block_cycles = 500,
};

int test_flash_wrap_prog_read(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Flash wrapper prog/read ===\n");
    const lfs_block_t block = 0;
    const lfs_off_t off = 0;
    const uint8_t write_data[16] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02,
                                    0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                    0x09, 0x0A, 0x0B, 0x0C};
    int rc = lfs_gen_flash_wrap_prog(&test_flash_cfg, block, off, write_data,
                                     sizeof(write_data));
    TEST_ASSERT(rc == 0, "prog should return 0");

    uint8_t read_buf[16];
    rc = lfs_gen_flash_wrap_read(&test_flash_cfg, block, off, read_buf,
                                 sizeof(read_buf));
    TEST_ASSERT(rc == 0, "read should return 0");
    TEST_ASSERT(memcmp(read_buf, write_data, sizeof(write_data)) == 0,
                "read should return what was programmed");
    return 0;
}

int test_flash_wrap_erase(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Flash wrapper erase fills 0xFF ===\n");
    const lfs_block_t block = 1;

    uint8_t pattern[16];
    memset(pattern, 0xAB, sizeof(pattern));
    lfs_gen_flash_wrap_prog(&test_flash_cfg, block, 0, pattern,
                            sizeof(pattern));

    int rc = lfs_gen_flash_wrap_erase(&test_flash_cfg, block);
    TEST_ASSERT(rc == 0, "erase should return 0");

    uint8_t read_buf[16];
    lfs_gen_flash_wrap_read(&test_flash_cfg, block, 0, read_buf,
                            sizeof(read_buf));
    for (int i = 0; i < 16; i++)
    {
        TEST_ASSERT(read_buf[i] == 0xFF,
                    "erased block should read 0xFF");
    }
    return 0;
}

int test_flash_wrap_sync(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Flash wrapper sync ===\n");
    int rc = lfs_gen_flash_wrap_sync(&test_flash_cfg);
    TEST_ASSERT(rc == 0, "sync should return 0");
    return 0;
}

int test_flash_wrap_offset_within_block(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Flash wrapper prog/read at offset ===\n");
    const lfs_block_t block = 2;
    const lfs_off_t off = 128;

    lfs_gen_flash_wrap_erase(&test_flash_cfg, block);

    const uint8_t data[16] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
                              0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x00};
    lfs_gen_flash_wrap_prog(&test_flash_cfg, block, off, data, sizeof(data));

    uint8_t read_buf[16];
    lfs_gen_flash_wrap_read(&test_flash_cfg, block, off, read_buf,
                            sizeof(read_buf));
    TEST_ASSERT(memcmp(read_buf, data, sizeof(data)) == 0,
                "read at offset should match programmed data");

    uint8_t before[16];
    lfs_gen_flash_wrap_read(&test_flash_cfg, block, 0, before, sizeof(before));
    for (int i = 0; i < 16; i++)
    {
        TEST_ASSERT(before[i] == 0xFF,
                    "bytes before offset should still be 0xFF");
    }
    return 0;
}

int test_flash_wrap_lfs_format_mount(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Flash wrapper LFS format and mount ===\n");

    static uint8_t lfs_read_buf[256];
    static uint8_t lfs_prog_buf[256];
    static uint8_t lfs_lookahead_buf[16];

    struct lfs_config cfg = test_flash_cfg;
    cfg.read_buffer = lfs_read_buf;
    cfg.prog_buffer = lfs_prog_buf;
    cfg.lookahead_buffer = lfs_lookahead_buf;

    lfs_t lfs;
    int err = lfs_format(&lfs, &cfg);
    TEST_ASSERT(err == 0, "lfs_format should succeed");

    err = lfs_mount(&lfs, &cfg);
    TEST_ASSERT(err == 0, "lfs_mount should succeed after format");

    lfs_unmount(&lfs);
    return 0;
}

int test_flash_wrap_lfs_write_read_file(slate_t *slate)
{
    (void)slate;
    LOG_DEBUG("=== Test: Flash wrapper LFS write and read file ===\n");

    static uint8_t lfs_read_buf[256];
    static uint8_t lfs_prog_buf[256];
    static uint8_t lfs_lookahead_buf[16];
    static uint8_t file_buf[256];

    struct lfs_config cfg = test_flash_cfg;
    cfg.read_buffer = lfs_read_buf;
    cfg.prog_buffer = lfs_prog_buf;
    cfg.lookahead_buffer = lfs_lookahead_buf;

    lfs_t lfs;
    lfs_format(&lfs, &cfg);
    int err = lfs_mount(&lfs, &cfg);
    TEST_ASSERT(err == 0, "lfs_mount should succeed");

    lfs_file_t file;
    struct lfs_file_config file_cfg = {.buffer = file_buf};

    err = lfs_file_opencfg(&lfs, &file, "test.txt",
                           LFS_O_WRONLY | LFS_O_CREAT, &file_cfg);
    TEST_ASSERT(err == 0, "file open for write should succeed");

    const char *msg = "Hello from flash mock!";
    lfs_ssize_t written = lfs_file_write(&lfs, &file, msg, strlen(msg));
    TEST_ASSERT(written == (lfs_ssize_t)strlen(msg),
                "all bytes should be written");

    lfs_file_close(&lfs, &file);

    err = lfs_file_opencfg(&lfs, &file, "test.txt", LFS_O_RDONLY, &file_cfg);
    TEST_ASSERT(err == 0, "file open for read should succeed");

    char read_buf[64] = {0};
    lfs_ssize_t bytes_read = lfs_file_read(&lfs, &file, read_buf, sizeof(read_buf));
    TEST_ASSERT(bytes_read == (lfs_ssize_t)strlen(msg),
                "should read back same number of bytes");
    TEST_ASSERT(memcmp(read_buf, msg, strlen(msg)) == 0,
                "file content should match what was written");

    lfs_file_close(&lfs, &file);
    lfs_unmount(&lfs);
    return 0;
}
#endif // TEST

// ============================================================================
// Test registry
// ============================================================================
const test_harness_case_t mram_tests[] = {
    {1, test_mram_write_read, "Write and Read"},
    {2, test_mram_write_disable_enable, "Write Disable/Enable"},
    {3, test_mram_preserve_data_on_sleep, "Preserve Data on Sleep"},
    {4, test_mram_clear, "Clear"},
    {5, test_mram_read_status, "Read Status"},
    {6, test_mram_write_max_length_boundary, "Write Max Length Boundary"},
    {7, test_mram_write_exceeds_max_length, "Write Exceeds Max Length"},
    {8, test_mram_clear_exceeds_max_length, "Clear Exceeds Max Length"},
    {9, test_mram_write_overwrite, "Write Overwrite"},
    {10, test_mram_multiple_independent_regions, "Multiple Independent Regions"},
    {11, test_mram_adjacent_regions_no_bleed, "Adjacent Regions No Bleed"},
    {12, test_mram_full_byte_range, "Full Byte Range"},
    {13, test_mram_single_byte_write_read, "Single Byte Write/Read"},
    {14, test_mram_large_write_read, "Large Write/Read"},
#ifdef TEST
    {15, test_flash_wrap_prog_read, "Flash Wrap Prog/Read"},
    {16, test_flash_wrap_erase, "Flash Wrap Erase"},
    {17, test_flash_wrap_sync, "Flash Wrap Sync"},
    {18, test_flash_wrap_offset_within_block, "Flash Wrap Offset Within Block"},
    {19, test_flash_wrap_lfs_format_mount, "Flash Wrap LFS Format/Mount"},
    {20, test_flash_wrap_lfs_write_read_file, "Flash Wrap LFS Write/Read File"},
#endif
};

const size_t mram_tests_len = sizeof(mram_tests) / sizeof(mram_tests[0]);

// ============================================================================
// Main test runner
// ============================================================================
int main()
{
    mram_init();

    return test_harness_run("MRAM", mram_tests, mram_tests_len,
                            mram_test_setup);
}
