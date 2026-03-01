#include "filesys_test.h"

void main(void)
{
    LOG_DEBUG("========================================\n");
    LOG_DEBUG("Starting Filesys Test Suite\n");
    LOG_DEBUG("========================================\n");

    int result;
    int tests_passed = 0;
    int tests_failed = 0;

    // Define test functions
    struct
    {
        int (*test_func)(void);
        const char *name;
    } tests[] = {
        {filesys_test_write_readback_success, "Write and Readback"},
        {filesys_test_initialize_reformat_success, "Initialize and Reformat"},
        {filesys_test_start_file_write_already_writing_should_fail,
         "Start File Write - Already Writing"},
        {filesys_test_write_data_to_buffer_bounds_should_fail,
         "Write Data to Buffer - Bounds Checking"},
        {filesys_test_write_data_to_buffer_when_no_file_started_should_fail,
         "Write Data to Buffer - No File"},
        {filesys_test_write_buffer_to_mram_when_no_file_started_should_fail,
         "Write Buffer to MRAM - No File"},
        {filesys_test_write_buffer_to_mram_clean_buffer_success,
         "Write Buffer to MRAM - Clean Buffer"},
        {filesys_test_complete_file_write_dirty_buffer_success,
         "Complete File Write - Dirty Buffer"},
        {filesys_test_cancel_file_write_success, "Cancel File Write"},
        {filesys_test_cancel_file_write_no_file_should_fail,
         "Cancel File Write - No File"},
        {filesys_test_clear_buffer_success, "Clear Buffer"},
        {filesys_test_crc_correct_success, "CRC Verification - Correct"},
        {filesys_test_crc_incorrect_should_fail,
         "CRC Verification - Incorrect"},
        {filesys_test_crc_no_file_should_fail, "CRC Check - No File"},
        {filesys_test_multiple_files_success, "Multiple File Writes"},
        {filesys_test_blocks_left_calculation_success,
         "Blocks Left Calculation"},
        {filesys_test_multi_chunk_write_success, "Multi-Chunk Write"},
        {filesys_test_write_at_offset_success, "Write at Offset"},
        {filesys_test_multiple_files_commit_success,
         "Multiple File Writes Committed"},
        {filesys_test_write_long_file_crc32_success,
         "Write Really Long File with CRC32"},
        {filesys_test_file_too_large_should_fail,
         "File Too Large for Filesystem"},
        {filesys_test_second_file_out_of_space_should_fail,
         "Second File Runs Out of Space"},
        {filesys_test_raw_lfs_write_large_file_success,
         "Raw LFS Write Large File (510KB)"},
        {filesys_test_list_files_empty_filesystem_success,
         "List Files - Empty Filesystem"},
        {filesys_test_list_files_multiple_files_success,
         "List Files - Multiple Files"},
        {filesys_test_list_files_max_files_limit_success,
         "List Files - Max Files Limit"},
        {filesys_test_list_files_after_cancel_success,
         "List Files - After Cancel"},
        {filesys_test_list_files_crc_mismatch_success,
         "List Files - CRC Mismatch Detection"},
    };

    int num_tests = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < num_tests; i++)
    {
        LOG_DEBUG("\n--- Running Test %d/%d: %s ---\n", i + 1, num_tests,
                  tests[i].name);
        result = tests[i].test_func();
        if (result == 0)
        {
            tests_passed++;
            LOG_DEBUG("--- Test %d PASSED ---\n", i + 1);
        }
        else
        {
            tests_failed++;
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
        return;
    }

    LOG_DEBUG("All Filesys tests passed!\n");
}
