#if defined(BRINGUP) || defined(PICO)

#include "hardware_test_task.h"
#include "hardware_tests.h"

static const hw_test_entry_t hw_tests[] = HW_TEST_TABLE;
#define NUM_HW_TESTS (sizeof(hw_tests) / sizeof(hw_tests[0]))

void hardware_test_task_init(slate_t *slate)
{
    /* All work is done on dispatch. */
}

void hardware_test_task_dispatch(slate_t *slate)
{
    LOG_INFO("Hardware test task: running %zu test(s)", NUM_HW_TESTS);

    for (size_t i = 0; i < NUM_HW_TESTS; i++)
    {
        LOG_INFO("=== [%zu/%zu] %s ===", i + 1, NUM_HW_TESTS, hw_tests[i].name);
        hw_tests[i].run();
    }

    LOG_INFO("Hardware tests complete.");
}

sched_task_t hardware_test_task = {
    .name = "hardware_test",
    // To make sure we see the test results, keep testing every 10 seconds.
    .dispatch_period_ms = 10000,
    .task_init = &hardware_test_task_init,
    .task_dispatch = &hardware_test_task_dispatch,
    .next_dispatch = 0,
};

#endif
