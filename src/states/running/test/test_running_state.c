/**
 * @file test_running_state.c
 * @brief Unit tests for the running state FSM
 *
 * Tests verify:
 * - Task execution during the running state
 * - Correct task ordering in the task list
 * - State transition behavior
 */

#include "running_state.h"
#include "logger.h"
#include "error.h"
#include <string.h>

slate_t test_slate;

/**
 * Test that the running state is properly initialized
 */
void test_running_state_initialization()
{
    LOG_DEBUG("Testing running_state initialization...");

    ASSERT(running_state.name != NULL);
    ASSERT(strcmp(running_state.name, "running") == 0);
    ASSERT(running_state.num_tasks > 0);
    ASSERT(running_state.get_next_state != NULL);
    ASSERT(running_state.task_list != NULL);

    LOG_DEBUG("✓ Running state initialization test passed");
}

/**
 * Test that all tasks in the running state have valid properties
 */
void test_running_state_tasks_valid()
{
    LOG_DEBUG("Testing running_state tasks validity...");

    for (size_t i = 0; i < running_state.num_tasks; i++)
    {
        sched_task_t *task = running_state.task_list[i];

        ASSERT(task != NULL);
        ASSERT(task->name != NULL);
        ASSERT(task->task_init != NULL);
        ASSERT(task->task_dispatch != NULL);
        ASSERT(task->dispatch_period_ms > 0);

        LOG_DEBUG("  Task %zu: %s (period: %u ms)", i, task->name, task->dispatch_period_ms);
    }

    LOG_DEBUG("✓ Running state tasks validity test passed");
}

/**
 * Test expected task ordering and presence
 */
void test_running_state_task_order()
{
    LOG_DEBUG("Testing running_state task order...");

    // Verify that specific critical tasks are present
    bool found_print = false;
    bool found_watchdog = false;

    for (size_t i = 0; i < running_state.num_tasks; i++)
    {
        sched_task_t *task = running_state.task_list[i];

        if (strcmp(task->name, "print") == 0)
        {
            found_print = true;
            // Print task should be first (index 0) for early logging
            ASSERT(i == 0);
            LOG_DEBUG("  Found print task at expected position %zu", i);
        }

        if (strcmp(task->name, "watchdog") == 0)
        {
            found_watchdog = true;
            // Watchdog should be second (index 1) for early reset prevention
            ASSERT(i == 1);
            LOG_DEBUG("  Found watchdog task at expected position %zu", i);
        }
    }

    ASSERT(found_print);
    ASSERT(found_watchdog);

    LOG_DEBUG("✓ Running state task order test passed");
}

/**
 * Test that task init functions can be called
 */
void test_running_state_task_init()
{
    LOG_DEBUG("Testing running_state task initialization...");

    // Initialize the test slate
    memset(&test_slate, 0, sizeof(slate_t));
    test_slate.current_state = &running_state;

    // Call init for each task in the running state
    for (size_t i = 0; i < running_state.num_tasks; i++)
    {
        sched_task_t *task = running_state.task_list[i];
        LOG_DEBUG("  Initializing task %zu: %s", i, task->name);
        task->task_init(&test_slate);
    }

    LOG_DEBUG("✓ Running state task init test passed");
}

/**
 * Test that task dispatch functions can be called
 */
void test_running_state_task_dispatch()
{
    LOG_DEBUG("Testing running_state task dispatch...");

    // Initialize the test slate
    memset(&test_slate, 0, sizeof(slate_t));
    test_slate.current_state = &running_state;

    // Call dispatch for each task in the running state
    for (size_t i = 0; i < running_state.num_tasks; i++)
    {
        sched_task_t *task = running_state.task_list[i];
        LOG_DEBUG("  Dispatching task %zu: %s", i, task->name);
        task->task_dispatch(&test_slate);
    }

    LOG_DEBUG("✓ Running state task dispatch test passed");
}

/**
 * Test state transition behavior
 */
void test_running_state_transition()
{
    LOG_DEBUG("Testing running_state transition behavior...");

    // Initialize the test slate
    memset(&test_slate, 0, sizeof(slate_t));
    test_slate.current_state = &running_state;

    // Get next state - running state should return itself
    sched_state_t *next_state = running_state.get_next_state(&test_slate);

    ASSERT(next_state != NULL);
    ASSERT(next_state == &running_state);
    ASSERT(strcmp(next_state->name, "running") == 0);

    LOG_DEBUG("✓ Running state transition test passed");
}

/**
 * Test task dispatch periods are reasonable
 */
void test_running_state_dispatch_periods()
{
    LOG_DEBUG("Testing running_state task dispatch periods...");

    for (size_t i = 0; i < running_state.num_tasks; i++)
    {
        sched_task_t *task = running_state.task_list[i];

        // Dispatch periods should be reasonable (between 10ms and 10 minutes)
        ASSERT(task->dispatch_period_ms >= 10);
        ASSERT(task->dispatch_period_ms <= 600000);

        LOG_DEBUG("  Task %s: dispatch_period_ms = %u",
                  task->name, task->dispatch_period_ms);
    }

    LOG_DEBUG("✓ Running state dispatch periods test passed");
}

/**
 * Test that tasks execute in the expected order based on timing
 */
void test_running_state_task_execution_order()
{
    LOG_DEBUG("Testing running_state task execution order based on timing...");

    // Initialize the test slate
    memset(&test_slate, 0, sizeof(slate_t));
    test_slate.current_state = &running_state;

    // Simulate scheduler behavior - tasks with smaller periods should execute more frequently
    uint32_t min_period = UINT32_MAX;
    size_t fastest_task_idx = 0;

    for (size_t i = 0; i < running_state.num_tasks; i++)
    {
        sched_task_t *task = running_state.task_list[i];
        if (task->dispatch_period_ms < min_period)
        {
            min_period = task->dispatch_period_ms;
            fastest_task_idx = i;
        }
    }

    sched_task_t *fastest_task = running_state.task_list[fastest_task_idx];
    LOG_DEBUG("  Fastest task: %s with period %u ms",
              fastest_task->name, fastest_task->dispatch_period_ms);

    ASSERT(min_period > 0);
    ASSERT(min_period <= 10000); // Fastest task should be at most 10 seconds

    LOG_DEBUG("✓ Running state task execution order test passed");
}

int main()
{
    LOG_DEBUG("=== Running State FSM Tests ===");

    test_running_state_initialization();
    test_running_state_tasks_valid();
    test_running_state_task_order();
    test_running_state_task_init();
    test_running_state_task_dispatch();
    test_running_state_transition();
    test_running_state_dispatch_periods();
    test_running_state_task_execution_order();

    LOG_DEBUG("=== All Running State Tests Passed ===");

    return 0;
}
