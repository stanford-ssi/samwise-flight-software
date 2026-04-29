#if defined(BRINGUP) || defined(PICO)

#include "hardware_test_task.h"

#ifndef TEST
#include "hardware_tests.h"
#include "pico/stdlib.h"

static const hw_test_entry_t hw_tests[] = HW_TEST_TABLE;
#define NUM_HW_TESTS (sizeof(hw_tests) / sizeof(hw_tests[0]))
#endif

void hardware_test_task_init(slate_t *slate)
{
#ifndef TEST
    LOG_INFO("Hardware test task starting in 30 seconds...");
    for (int i = 30; i > 0; i--)
    {
        LOG_INFO("Hardware test task is dispatching... %d", i);
        sleep_ms(1000);
    }

    LOG_INFO("Hardware test task: running %zu test(s)", NUM_HW_TESTS);

    for (size_t i = 0; i < NUM_HW_TESTS; i++)
    {
        LOG_INFO("=== [%zu/%zu] %s ===", i + 1, NUM_HW_TESTS, hw_tests[i].name);
        hw_tests[i].run();
    }

    LOG_INFO("Hardware tests complete.");
#endif
}

void hardware_test_task_dispatch(slate_t *slate)
{
}

sched_task_t hardware_test_task = {
    .name = "hardware_test",
    .dispatch_period_ms = 10000,
    .task_init = &hardware_test_task_init,
    .task_dispatch = &hardware_test_task_dispatch,
    .next_dispatch = 0,
};

#endif
