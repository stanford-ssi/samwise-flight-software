#include "test_harness.h"

int test_harness_run(const char *suite_name, const test_harness_case_t *tests,
                     size_t num_tests,
                     const test_harness_init_slate_func_t init_func)
{
    LOG_DEBUG("========================================\n");
    LOG_DEBUG("Starting %s Test Suite\n", suite_name);
    LOG_DEBUG("========================================\n");

    if (num_tests == 0)
    {
        LOG_DEBUG("No tests to run in suite %s.\n", suite_name);
        return 0;
    }

    int result;
    size_t tests_passed = 0;
    size_t tests_failed = 0;
    bool test_results[num_tests];

    for (size_t i = 0; i < num_tests; i++)
    {
        LOG_DEBUG("\n--- Running Test %zu/%zu: %s (id=%d) ---\n", i + 1,
                  num_tests, tests[i].name, tests[i].test_id);

        slate_t slate;
        test_harness_init_slate_func_t init_function_to_use =
            init_func != NULL && tests[i].requires_custom_init
                ? init_func
                : clear_and_init_slate;

        if (init_function_to_use(&slate) < 0)
        {
            LOG_ERROR("Failed to set up slate for test %d: %s\n", i + 1,
                      tests[i].name);
            result = -1;
        }
        else
        {
            result = tests[i].test_func(&slate);
        }

        free_slate(&slate);

        if (result == 0)
        {
            tests_passed++;
            test_results[i] = true;
            LOG_DEBUG("--- Test %zu/%zu PASSED: %s (id=%d) ---\n", i + 1,
                      num_tests, tests[i].name, tests[i].test_id);
        }
        else
        {
            tests_failed++;
            test_results[i] = false;
            LOG_ERROR("--- Test %zu/%zu FAILED: %s (id=%d) ---\n", i + 1,
                      num_tests, tests[i].name, tests[i].test_id);
        }
    }

    LOG_DEBUG("========================================\n");
    LOG_DEBUG("Test Suite Complete\n");
    LOG_DEBUG("Passed: %zu / %zu\n", tests_passed, num_tests);
    LOG_DEBUG("Failed: %zu / %zu\n", tests_failed, num_tests);
    if (tests_failed > 0)
    {
        LOG_DEBUG("Failed Tests:");
        for (size_t i = 0; i < num_tests; i++)
        {
            if (!test_results[i])
                LOG_DEBUG("    - %s (id=%d)\n", tests[i].name,
                          tests[i].test_id);
        }
    }
    LOG_DEBUG("========================================\n");

    if (tests_failed > 0)
    {
        LOG_ERROR("SOME TESTS FAILED!\n");
        return -1;
    }

    LOG_DEBUG("All %s tests passed!\n", suite_name);
    return 0;
}

int test_harness_include_run(const char *suite_name,
                             const test_harness_case_t *cases, size_t num_tests,
                             const test_harness_init_slate_func_t init_func,
                             const uint16_t *ids, size_t num_ids)
{
    test_harness_case_t picked_cases[num_ids];
    for (size_t i = 0; i < num_ids; i++)
    {
        bool found = false;
        for (size_t j = 0; j < num_tests; j++)
        {
            if (cases[j].test_id == ids[i])
            {
                picked_cases[i] = cases[j];
                found = true;
                break;
            }
        }

        if (!found)
        {
            LOG_ERROR("ID %u not found for test cases!\n", ids[i]);
            return -1;
        }
    }

    return test_harness_run(suite_name, picked_cases, num_ids, init_func);
}

int test_harness_exclude_run(const char *suite_name,
                             const test_harness_case_t *cases, size_t num_tests,
                             const test_harness_init_slate_func_t init_func,
                             const uint16_t *exclude_ids,
                             size_t num_exclude_ids)
{
    ASSERT(num_exclude_ids < num_tests);
    test_harness_case_t included_cases[num_tests - num_exclude_ids];
    size_t include_idx = 0;
    for (size_t i = 0; i < num_tests; i++)
    {
        bool excluded = false;
        for (size_t j = 0; j < num_exclude_ids; j++)
            if (cases[i].test_id == exclude_ids[j])
            {
                excluded = true;
                break;
            }

        if (!excluded)
            included_cases[include_idx++] = cases[i];
    }

    return test_harness_run(suite_name, included_cases, include_idx, init_func);
}
