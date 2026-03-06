/***
 * @author Ayush Garg, Luis Cantoran, Joziah Uribe-Lopez
 * @date   2025-10-18
 *
 *  Test for mram driver
 */
#include "mram_test.h"

// Test harness macro: logs a message and returns -1 on failure
#define TEST_ASSERT(cond, msg)                                                 \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            LOG_ERROR("[mram_test] FAIL: %s\n", msg);                          \
            return -1;                                                         \
        }                                                                      \
    } while (0)

int test_mram_write_read(void)
{
    LOG_DEBUG("Testing MRAM write...\n");
    TEST_ASSERT(mram_write(0x000000, (uint8_t *)"Hello, MRAM!", 13),
                "mram_write should succeed");

    LOG_DEBUG("Testing MRAM read...\n");

    uint8_t buffer[13];
    mram_read(0x000000, buffer, 13);

    LOG_DEBUG("Read from MRAM: %s\n", buffer);
    TEST_ASSERT(memcmp(buffer, "Hello, MRAM!", 13) == 0,
                "Read back should match written data");

    return 0;
}

int test_mram_write_disable_enable(void)
{
    const uint32_t test_addr = 0x000100;
    const uint8_t WEL_BIT = 0x02;

    /* mram_write calls mram_write_enable internally, so write-disable has no
       effect on subsequent writes (matches real hardware behaviour). What
       mram_write_disable does affect is the WEL bit in the status register. */
    LOG_DEBUG("Testing MRAM write disable clears WEL bit...\n");
    mram_write_disable();
    TEST_ASSERT((mram_read_status() & WEL_BIT) == 0,
                "WEL bit should be cleared after write disable");

    /* A write should still succeed because mram_write re-enables internally */
    LOG_DEBUG("Testing MRAM write succeeds despite prior write-disable...\n");
    TEST_ASSERT(
        mram_write(test_addr, (const uint8_t *)"This should be written", 22),
        "Write should succeed despite prior write-disable");
    uint8_t verify_buf[22];
    mram_read(test_addr, verify_buf, 22);
    TEST_ASSERT(memcmp(verify_buf, "This should be written", 22) == 0,
                "Data should match after write despite prior write-disable");

    /* mram_write_enable explicitly sets the WEL bit */
    LOG_DEBUG("Testing MRAM write enable sets WEL bit...\n");
    mram_write_disable();
    mram_write_enable();
    TEST_ASSERT((mram_read_status() & WEL_BIT) != 0,
                "WEL bit should be set after write enable");
    return 0;
}

int test_mram_preserve_data_on_sleep(void)
{
    LOG_DEBUG("Testing MRAM preserve data across sleep/wake...\n");
    const char *test_str = "SleepTest123";
    size_t len = strlen(test_str) + 1; // include null terminator
    /* Clear a read buffer and write the test string */
    uint8_t read_buf[32];
    memset(read_buf, 0x00, sizeof(read_buf));
    TEST_ASSERT(mram_write(0x000200, (const uint8_t *)test_str, len),
                "Write before sleep should succeed");
    /* Read back before sleep to ensure write succeeded */
    mram_read(0x000200, read_buf, len);
    TEST_ASSERT(memcmp(read_buf, test_str, len) == 0,
                "Read before sleep should match written data");
    /* Put MRAM to sleep and then wake it up */
    mram_sleep();
    sleep_us(1000); /* allow some time while sleeping */
    mram_wake();
    /* Read back after wake and verify data preserved */
    memset(read_buf, 0x00, sizeof(read_buf));
    mram_read(0x000200, read_buf, len);
    TEST_ASSERT(memcmp(read_buf, test_str, len) == 0,
                "Data should be preserved after sleep/wake cycle");
    return 0;
}

int test_mram_clear(void)
{
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
    uint8_t read_buf[len];
    memset(read_buf, 0xFF, len);
    mram_read(addr, read_buf, len);
    for (size_t i = 0; i < len; i++)
    {
        TEST_ASSERT(read_buf[i] == 0x00,
                    "Each byte should be zero after clear");
    }
    return 0;
}

int test_mram_read_status(void)
{
    LOG_DEBUG("Testing MRAM read status...\n");
    /* STATUS WEL bit mask (bit 1) */
    const uint8_t WEL_BIT = 0x02;
    /* Ensure known state: disable writes and check WEL cleared */
    mram_write_disable();
    uint8_t status = mram_read_status();
    LOG_DEBUG("MRAM status (after WRDI): 0x%02X\n", status);
    TEST_ASSERT((status & WEL_BIT) == 0,
                "WEL bit should be cleared after write disable");
    /* Enable writes and verify the WEL bit is set */
    mram_write_enable();
    status = mram_read_status();
    LOG_DEBUG("MRAM status (after WREN): 0x%02X\n", status);
    TEST_ASSERT((status & WEL_BIT) != 0,
                "WEL bit should be set after write enable");
    return 0;
}

// Write exactly 256 bytes (maximum allowed) and verify round-trip
int test_mram_write_max_length_boundary(void)
{
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

// Write 257 bytes, verify mram_write returns false
int test_mram_write_exceeds_max_length(void)
{
    LOG_DEBUG("=== Test: Write exceeds max length ===\n");
    uint8_t buf[257];
    memset(buf, 0xAB, sizeof(buf));
    TEST_ASSERT(!mram_write(0x000400, buf, 257),
                "Write of 257 bytes should return false");
    return 0;
}

// Attempt to clear 257 bytes; should be a no-op — existing data must be
// unchanged
int test_mram_clear_exceeds_max_length(void)
{
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

// Write pattern A then overwrite same address with pattern B; verify B is read
// back
int test_mram_write_overwrite(void)
{
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

// Write to three independent non-overlapping addresses, verify no
// cross-contamination
int test_mram_multiple_independent_regions(void)
{
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

// Write to two adjacent regions, verify neither bleeds into the other
int test_mram_adjacent_regions_no_bleed(void)
{
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

// Write all 256 possible byte values (0x00..0xFF) in a single 256-byte payload
int test_mram_full_byte_range(void)
{
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

// Write and read a single byte (minimum payload)
int test_mram_single_byte_write_read(void)
{
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

int test_mram_large_write_read(void)
{
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

// ============================================================================
// Main test runner
// ============================================================================
int main()
{
    LOG_DEBUG("========================================\n");
    LOG_DEBUG("Starting MRAM Test Suite\n");
    LOG_DEBUG("========================================\n");

    mram_init();

    int result;
    int tests_passed = 0;
    int tests_failed = 0;

    struct
    {
        int (*test_func)(void);
        const char *name;
    } tests[] = {
        {test_mram_write_read, "Write and Read"},
        {test_mram_write_disable_enable, "Write Disable/Enable"},
        {test_mram_preserve_data_on_sleep, "Preserve Data on Sleep"},
        {test_mram_clear, "Clear"},
        {test_mram_read_status, "Read Status"},
        {test_mram_write_max_length_boundary, "Write Max Length Boundary"},
        {test_mram_write_exceeds_max_length, "Write Exceeds Max Length"},
        {test_mram_clear_exceeds_max_length, "Clear Exceeds Max Length"},
        {test_mram_write_overwrite, "Write Overwrite"},
        {test_mram_multiple_independent_regions,
         "Multiple Independent Regions"},
        {test_mram_adjacent_regions_no_bleed, "Adjacent Regions No Bleed"},
        {test_mram_full_byte_range, "Full Byte Range"},
        {test_mram_single_byte_write_read, "Single Byte Write/Read"},
        {test_mram_large_write_read, "Large Write/Read"},
    };

    int num_tests = sizeof(tests) / sizeof(tests[0]);
    bool tests_passed_arr[num_tests];

    for (int i = 0; i < num_tests; i++)
    {
        LOG_DEBUG("\n--- Running Test %d/%d: %s ---\n", i + 1, num_tests,
                  tests[i].name);
        result = tests[i].test_func();
        if (result == 0)
        {
            tests_passed++;
            tests_passed_arr[i] = true;
            LOG_DEBUG("--- Test %d PASSED ---\n", i + 1);
        }
        else
        {
            tests_failed++;
            tests_passed_arr[i] = false;
            LOG_ERROR("--- Test %d FAILED ---\n", i + 1);
        }
    }

    LOG_DEBUG("\n========================================\n");
    LOG_DEBUG("Test Suite Complete\n");
    LOG_DEBUG("Passed: %d / %d\n", tests_passed, num_tests);
    LOG_DEBUG("Failed: %d / %d\n", tests_failed, num_tests);
    LOG_DEBUG("========================================\n");

    if (tests_failed > 0)
    {
        LOG_ERROR("SOME TESTS FAILED!\n");
        for (int i = 0; i < num_tests; i++)
        {
            if (!tests_passed_arr[i])
            {
                LOG_ERROR(" - Test %d: %s\n", i + 1, tests[i].name);
            }
        }
        return -1;
    }

    LOG_DEBUG("All MRAM tests passed!\n");
    return 0;
}
