/**
 * @file test_filesys.c
 * @brief Unit tests for the filesystem module
 * @author Test Suite
 * @date 2025-10-29
 */

#define TEST

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "filesys.h"
#include "lfs.h"
#include "logger.h"
#include "slate.h"

/* Test helper macros */
#define TEST_ASSERT(condition, message)                                        \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
        {                                                                      \
            LOG_ERROR("FAIL: %s - %s", __func__, message);                     \
            return -1;                                                         \
        }                                                                      \
    } while (0)

#define TEST_PASS()                                                            \
    do                                                                         \
    {                                                                          \
        LOG_INFO("PASS: %s", __func__);                                        \
        return 0;                                                              \
    } while (0)

/* Mock data for testing */
static slate_t test_slate;
static uint8_t test_buffer[1024];

/* Test fixtures */
void setup_test_slate(void)
{
    memset(&test_slate, 0, sizeof(slate_t));
    memset(test_buffer, 0, sizeof(test_buffer));
}

void teardown_test_slate(void)
{
    /* Clean up any resources */
    filesys_cancel_file_write(&test_slate);
}

/* ============================================================================
 * Test Cases for filesys_initialize
 * ============================================================================
 */

/**
 * Test successful filesystem initialization
 */
int test_filesys_initialize_success(void)
{
    setup_test_slate();

    lfs_ssize_t result = filesys_initialize(&test_slate);
    TEST_ASSERT(result >= 0, "Initialization should succeed");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test filesystem initialization with NULL pointer
 */
int test_filesys_initialize_null_slate(void)
{
    lfs_ssize_t result = filesys_initialize(NULL);
    TEST_ASSERT(result < 0, "Should fail with NULL slate pointer");
    TEST_PASS();
}

/**
 * Test double initialization
 */
int test_filesys_initialize_twice(void)
{
    setup_test_slate();

    lfs_ssize_t result1 = filesys_initialize(&test_slate);
    TEST_ASSERT(result1 >= 0, "First initialization should succeed");

    lfs_ssize_t result2 = filesys_initialize(&test_slate);
    TEST_ASSERT(result2 >= 0, "Second initialization should handle gracefully");

    teardown_test_slate();
    TEST_PASS();
}

/* ============================================================================
 * Test Cases for filesys_start_file_write
 * ============================================================================
 */

/**
 * Test starting a file write with valid parameters
 */
int test_filesys_start_file_write_success(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    FILESYS_BUFFERED_FNAME_T fname = 0x1234;
    FILESYS_BUFFERED_FILE_LEN_T file_size = 512;
    FILESYS_BUFFERED_FILE_CRC_T file_crc = 0xABCD;
    lfs_ssize_t blocks_left;

    int8_t result = filesys_start_file_write(&test_slate, fname, file_size,
                                             file_crc, &blocks_left);
    TEST_ASSERT(result == 0, "Should successfully start file write");
    TEST_ASSERT(blocks_left >= 0, "Should have blocks available");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test starting a file write when one is already in progress
 */
int test_filesys_start_file_write_already_writing(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    lfs_ssize_t blocks_left;

    /* Start first write */
    int8_t result1 = filesys_start_file_write(&test_slate, 0x1234, 512, 0xABCD,
                                              &blocks_left);
    TEST_ASSERT(result1 == 0, "First write should succeed");

    /* Try to start second write without completing first */
    int8_t result2 = filesys_start_file_write(&test_slate, 0x5678, 256, 0xDEF0,
                                              &blocks_left);
    TEST_ASSERT(result2 == -1,
                "Should return -1 when file already being written");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test starting a file write with insufficient space
 */
int test_filesys_start_file_write_insufficient_space(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    /* Try to write a very large file */
    FILESYS_BUFFERED_FILE_LEN_T huge_size = 0xFFFFFFFF;
    lfs_ssize_t blocks_left;

    int8_t result = filesys_start_file_write(&test_slate, 0x1234, huge_size,
                                             0xABCD, &blocks_left);
    TEST_ASSERT(result == -3, "Should return -3 for insufficient space");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test starting a file write with NULL blocks_left pointer
 */
int test_filesys_start_file_write_null_blocks(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    int8_t result =
        filesys_start_file_write(&test_slate, 0x1234, 512, 0xABCD, NULL);
    /* Implementation may handle NULL pointer - test expected behavior */
    TEST_ASSERT(result != 0, "Should handle NULL blocks_left appropriately");

    teardown_test_slate();
    TEST_PASS();
}

/* ============================================================================
 * Test Cases for filesys_write_data_to_buffer
 * ============================================================================
 */

/**
 * Test writing data to buffer successfully
 */
int test_filesys_write_data_to_buffer_success(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    lfs_ssize_t blocks_left;
    filesys_start_file_write(&test_slate, 0x1234, 512, 0xABCD, &blocks_left);

    uint8_t data[] = "Hello, World!";
    FILESYS_BUFFER_SIZE_T n_bytes = sizeof(data);
    FILESYS_BUFFER_SIZE_T offset = 0;

    int8_t result =
        filesys_write_data_to_buffer(&test_slate, data, n_bytes, offset);
    TEST_ASSERT(result == 0, "Should successfully write data to buffer");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test writing data with offset
 */
int test_filesys_write_data_to_buffer_with_offset(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    lfs_ssize_t blocks_left;
    filesys_start_file_write(&test_slate, 0x1234, 512, 0xABCD, &blocks_left);

    uint8_t data1[] = "First";
    uint8_t data2[] = "Second";

    int8_t result1 =
        filesys_write_data_to_buffer(&test_slate, data1, sizeof(data1), 0);
    TEST_ASSERT(result1 == 0, "First write should succeed");

    int8_t result2 =
        filesys_write_data_to_buffer(&test_slate, data2, sizeof(data2), 64);
    TEST_ASSERT(result2 == 0, "Second write with offset should succeed");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test writing NULL data
 */
int test_filesys_write_data_to_buffer_null_data(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    lfs_ssize_t blocks_left;
    filesys_start_file_write(&test_slate, 0x1234, 512, 0xABCD, &blocks_left);

    int8_t result = filesys_write_data_to_buffer(&test_slate, NULL, 10, 0);
    TEST_ASSERT(result != 0, "Should fail with NULL data pointer");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test writing with offset exceeding buffer size
 */
int test_filesys_write_data_to_buffer_offset_overflow(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    lfs_ssize_t blocks_left;
    filesys_start_file_write(&test_slate, 0x1234, 512, 0xABCD, &blocks_left);

    uint8_t data[] = "Test";
    FILESYS_BUFFER_SIZE_T large_offset = 0xFFFF; /* Exceeds buffer size */

    int8_t result = filesys_write_data_to_buffer(&test_slate, data,
                                                 sizeof(data), large_offset);
    TEST_ASSERT(result != 0, "Should fail when offset exceeds buffer size");

    teardown_test_slate();
    TEST_PASS();
}

/* ============================================================================
 * Test Cases for filesys_write_buffer_to_mram
 * ============================================================================
 */

/**
 * Test writing buffer to MRAM successfully
 */
int test_filesys_write_buffer_to_mram_success(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_T fname = 0x1234;
    filesys_start_file_write(&test_slate, fname, 512, 0xABCD, &blocks_left);

    uint8_t data[256];
    memset(data, 0xAA, sizeof(data));
    filesys_write_data_to_buffer(&test_slate, data, sizeof(data), 0);

    lfs_ssize_t bytes_written = filesys_write_buffer_to_mram(&test_slate, 256);
    TEST_ASSERT(bytes_written == 256, "Should write correct number of bytes");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test writing full buffer to MRAM
 */
int test_filesys_write_buffer_to_mram_full_buffer(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_T fname = 0x5678;
    filesys_start_file_write(&test_slate, fname, 1024, 0xDEF0, &blocks_left);

    /* Fill entire buffer */
    uint8_t data[512];
    memset(data, 0xBB, sizeof(data));
    filesys_write_data_to_buffer(&test_slate, data, sizeof(data), 0);

    /* Write using FILESYS_BUFFER_SIZE constant */
    lfs_ssize_t bytes_written = filesys_write_buffer_to_mram(&test_slate, 512);
    TEST_ASSERT(bytes_written > 0, "Should successfully write full buffer");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test multiple buffer writes to MRAM
 */
int test_filesys_write_buffer_to_mram_multiple(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_T fname = 0xABCD;
    filesys_start_file_write(&test_slate, fname, 1024, 0x1234, &blocks_left);

    /* Write first block */
    uint8_t data1[256];
    memset(data1, 0x11, sizeof(data1));
    filesys_write_data_to_buffer(&test_slate, data1, sizeof(data1), 0);
    lfs_ssize_t written1 = filesys_write_buffer_to_mram(&test_slate, 256);
    TEST_ASSERT(written1 == 256, "First write should succeed");

    /* Write second block */
    uint8_t data2[256];
    memset(data2, 0x22, sizeof(data2));
    filesys_write_data_to_buffer(&test_slate, data2, sizeof(data2), 0);
    lfs_ssize_t written2 = filesys_write_buffer_to_mram(&test_slate, 256);
    TEST_ASSERT(written2 == 256, "Second write should succeed");

    teardown_test_slate();
    TEST_PASS();
}

/* ============================================================================
 * Test Cases for filesys_complete_file_write
 * ============================================================================
 */

/**
 * Test completing a file write successfully
 */
int test_filesys_complete_file_write_success(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    lfs_ssize_t blocks_left;
    filesys_start_file_write(&test_slate, 0x1234, 512, 0xABCD, &blocks_left);

    /* Write and flush all data */
    uint8_t data[256];
    memset(data, 0xCC, sizeof(data));
    filesys_write_data_to_buffer(&test_slate, data, sizeof(data), 0);
    filesys_write_buffer_to_mram(&test_slate, 256);

    int8_t result = filesys_complete_file_write(&test_slate);
    TEST_ASSERT(result == 0, "Should successfully complete file write");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test completing with dirty buffer
 */
int test_filesys_complete_file_write_dirty_buffer(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    lfs_ssize_t blocks_left;
    filesys_start_file_write(&test_slate, 0x1234, 512, 0xABCD, &blocks_left);

    /* Write data but don't flush to MRAM */
    uint8_t data[256];
    memset(data, 0xDD, sizeof(data));
    filesys_write_data_to_buffer(&test_slate, data, sizeof(data), 0);

    int8_t result = filesys_complete_file_write(&test_slate);
    TEST_ASSERT(result == -1, "Should return -1 with dirty buffer");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test completing when no file is being written
 */
int test_filesys_complete_file_write_no_active_write(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    int8_t result = filesys_complete_file_write(&test_slate);
    TEST_ASSERT(result != 0, "Should handle no active write appropriately");

    teardown_test_slate();
    TEST_PASS();
}

/* ============================================================================
 * Test Cases for filesys_clear_buffer
 * ============================================================================
 */

/**
 * Test clearing buffer
 */
int test_filesys_clear_buffer(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    lfs_ssize_t blocks_left;
    filesys_start_file_write(&test_slate, 0x1234, 512, 0xABCD, &blocks_left);

    /* Write some data */
    uint8_t data[256];
    memset(data, 0xEE, sizeof(data));
    filesys_write_data_to_buffer(&test_slate, data, sizeof(data), 0);

    /* Clear buffer */
    filesys_clear_buffer(&test_slate);

    /* Now completing should succeed since buffer is clean */
    int8_t result = filesys_complete_file_write(&test_slate);
    TEST_ASSERT(result == 0, "Should complete after clearing buffer");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test clearing buffer with NULL slate
 */
int test_filesys_clear_buffer_null(void)
{
    /* This is a void function, so we just ensure it doesn't crash */
    filesys_clear_buffer(NULL);
    TEST_PASS();
}

/* ============================================================================
 * Test Cases for filesys_cancel_file_write
 * ============================================================================
 */

/**
 * Test canceling a file write
 */
int test_filesys_cancel_file_write_success(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    lfs_ssize_t blocks_left;
    filesys_start_file_write(&test_slate, 0x1234, 512, 0xABCD, &blocks_left);

    /* Write some data */
    uint8_t data[256];
    memset(data, 0xFF, sizeof(data));
    filesys_write_data_to_buffer(&test_slate, data, sizeof(data), 0);

    int8_t result = filesys_cancel_file_write(&test_slate);
    TEST_ASSERT(result == 0, "Should successfully cancel file write");

    /* Should be able to start a new write after canceling */
    int8_t result2 = filesys_start_file_write(&test_slate, 0x5678, 256, 0xDEF0,
                                              &blocks_left);
    TEST_ASSERT(result2 == 0, "Should start new write after canceling");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test canceling when no file is being written
 */
int test_filesys_cancel_file_write_no_active_write(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    int8_t result = filesys_cancel_file_write(&test_slate);
    TEST_ASSERT(result == -1, "Should return -1 when no file being written");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test canceling with partial data written to MRAM
 */
int test_filesys_cancel_file_write_with_mram_data(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_T fname = 0x1234;
    filesys_start_file_write(&test_slate, fname, 1024, 0xABCD, &blocks_left);

    /* Write and flush one block */
    uint8_t data[256];
    memset(data, 0x12, sizeof(data));
    filesys_write_data_to_buffer(&test_slate, data, sizeof(data), 0);
    filesys_write_buffer_to_mram(&test_slate, 256);

    /* Cancel should delete the partial file */
    int8_t result = filesys_cancel_file_write(&test_slate);
    TEST_ASSERT(result == 0, "Should cancel and delete partial MRAM data");

    teardown_test_slate();
    TEST_PASS();
}

/* ============================================================================
 * Integration Tests
 * ============================================================================
 */

/**
 * Test complete write workflow
 */
int test_complete_write_workflow(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    FILESYS_BUFFERED_FNAME_T fname = 0xBEEF;
    lfs_ssize_t blocks_left;

    /* Start write */
    int8_t result =
        filesys_start_file_write(&test_slate, fname, 768, 0xCAFE, &blocks_left);
    TEST_ASSERT(result == 0, "Should start file write");

    /* Write three blocks */
    for (int i = 0; i < 3; i++)
    {
        uint8_t data[256];
        memset(data, i, sizeof(data));
        filesys_write_data_to_buffer(&test_slate, data, sizeof(data), 0);
        lfs_ssize_t written = filesys_write_buffer_to_mram(&test_slate, 256);
        TEST_ASSERT(written == 256, "Block write should succeed");
    }

    /* Complete write */
    result = filesys_complete_file_write(&test_slate);
    TEST_ASSERT(result == 0, "Should complete file write");

    teardown_test_slate();
    TEST_PASS();
}

/**
 * Test write, cancel, and restart workflow
 */
int test_cancel_and_restart_workflow(void)
{
    setup_test_slate();
    filesys_initialize(&test_slate);

    lfs_ssize_t blocks_left;

    /* Start and cancel first write */
    filesys_start_file_write(&test_slate, 0x1111, 512, 0x2222, &blocks_left);
    uint8_t data1[128];
    memset(data1, 0xAA, sizeof(data1));
    filesys_write_data_to_buffer(&test_slate, data1, sizeof(data1), 0);
    int8_t result1 = filesys_cancel_file_write(&test_slate);
    TEST_ASSERT(result1 == 0, "Should cancel first write");

    /* Start second write */
    int8_t result2 = filesys_start_file_write(&test_slate, 0x3333, 256, 0x4444,
                                              &blocks_left);
    TEST_ASSERT(result2 == 0, "Should start second write after cancel");

    /* Complete second write */
    uint8_t data2[256];
    memset(data2, 0xBB, sizeof(data2));
    filesys_write_data_to_buffer(&test_slate, data2, sizeof(data2), 0);
    filesys_write_buffer_to_mram(&test_slate, 256);
    int8_t result3 = filesys_complete_file_write(&test_slate);
    TEST_ASSERT(result3 == 0, "Should complete second write");

    teardown_test_slate();
    TEST_PASS();
}

/* ============================================================================
 * Test Runner
 * ============================================================================
 */

typedef int (*test_func_t)(void);

typedef struct
{
    const char *name;
    test_func_t func;
} test_case_t;

static test_case_t test_cases[] = {
    /* Initialize tests */
    {"Initialize success", test_filesys_initialize_success},
    {"Initialize null slate", test_filesys_initialize_null_slate},
    {"Initialize twice", test_filesys_initialize_twice},

    /* Start file write tests */
    {"Start file write success", test_filesys_start_file_write_success},
    {"Start file write already writing",
     test_filesys_start_file_write_already_writing},
    {"Start file write insufficient space",
     test_filesys_start_file_write_insufficient_space},
    {"Start file write null blocks", test_filesys_start_file_write_null_blocks},

    /* Write data to buffer tests */
    {"Write data to buffer success", test_filesys_write_data_to_buffer_success},
    {"Write data with offset", test_filesys_write_data_to_buffer_with_offset},
    {"Write null data", test_filesys_write_data_to_buffer_null_data},
    {"Write with offset overflow",
     test_filesys_write_data_to_buffer_offset_overflow},

    /* Write buffer to MRAM tests */
    {"Write buffer to MRAM success", test_filesys_write_buffer_to_mram_success},
    {"Write full buffer to MRAM",
     test_filesys_write_buffer_to_mram_full_buffer},
    {"Write multiple buffers to MRAM",
     test_filesys_write_buffer_to_mram_multiple},

    /* Complete file write tests */
    {"Complete file write success", test_filesys_complete_file_write_success},
    {"Complete with dirty buffer",
     test_filesys_complete_file_write_dirty_buffer},
    {"Complete no active write",
     test_filesys_complete_file_write_no_active_write},

    /* Clear buffer tests */
    {"Clear buffer", test_filesys_clear_buffer},
    {"Clear buffer null", test_filesys_clear_buffer_null},

    /* Cancel file write tests */
    {"Cancel file write success", test_filesys_cancel_file_write_success},
    {"Cancel no active write", test_filesys_cancel_file_write_no_active_write},
    {"Cancel with MRAM data", test_filesys_cancel_file_write_with_mram_data},

    /* Integration tests */
    {"Complete write workflow", test_complete_write_workflow},
    {"Cancel and restart workflow", test_cancel_and_restart_workflow},
};

int main(void)
{
    int total = sizeof(test_cases) / sizeof(test_cases[0]);
    int passed = 0;
    int failed = 0;

    printf("Running %d tests...\n\n", total);

    for (int i = 0; i < total; i++)
    {
        printf("Test %d/%d: %s\n", i + 1, total, test_cases[i].name);
        int result = test_cases[i].func();

        if (result == 0)
        {
            passed++;
        }
        else
        {
            failed++;
        }
        printf("\n");
    }

    printf("======================\n");
    printf("Test Results:\n");
    printf("Total:  %d\n", total);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("======================\n");

    return (failed == 0) ? 0 : 1;
}
