#include "mram_test.h"

void main(void)
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
        return;
    }

    LOG_DEBUG("All MRAM tests passed!\n");
}
