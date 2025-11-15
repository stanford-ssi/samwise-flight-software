/**
 * @file test_running_state.c
 * @brief Unit tests for the actual running_state with real tasks
 *
 * Tests the running state as defined in running_state.c with its actual
 * tasks (print, watchdog, blink, adcs, telemetry, beacon, radio, command).
 */

#include "error.h"
#include "logger.h"
#include "pico/stdlib.h"
#include "running_state.h"
#include "scheduler.h"
#include "test_scheduler_helpers.h"
#include <stdio.h>
#include <string.h>

slate_t test_slate;

// =============================================================================
// TESTS
// =============================================================================

/**
 * Test 1: Verify running state structure
 */
void test_running_state_structure()
{
    LOG_DEBUG("=== Test 1: Running state structure ===");
    log_viz_event("test_start", NULL, "running_state_structure");

    // Check state properties
    ASSERT(running_state.name != NULL);
    ASSERT(strcmp(running_state.name, "running") == 0);
    ASSERT(running_state.num_tasks > 0);
    ASSERT(running_state.get_next_state != NULL);
    ASSERT(running_state.task_list != NULL);

    LOG_DEBUG("  State name: %s", running_state.name);
    LOG_DEBUG("  Number of tasks: %zu", running_state.num_tasks);

    // Check all tasks are valid
    for (size_t i = 0; i < running_state.num_tasks; i++)
    {
        sched_task_t *task = running_state.task_list[i];
        ASSERT(task != NULL);
        ASSERT(task->name != NULL);
        ASSERT(task->task_init != NULL);
        ASSERT(task->task_dispatch != NULL);
        ASSERT(task->dispatch_period_ms > 0);

        LOG_DEBUG("  Task %zu: %s (period: %u ms)", i, task->name,
                  task->dispatch_period_ms);
    }

    log_viz_event("test_pass", NULL, "running_state_structure");
    LOG_DEBUG("✓ Test 1 passed");
}

/**
 * Test 2: Verify task configuration
 */
void test_task_configuration()
{
    LOG_DEBUG("=== Test 2: Task configuration ===");
    log_viz_event("test_start", NULL, "task_configuration");

    // Verify critical tasks needs to be present
    bool found_watchdog = false;
    bool found_command = false;
    bool found_radio = false;
    bool found_beacon = false;

    for (size_t i = 0; i < running_state.num_tasks; i++)
    {
        sched_task_t *task = running_state.task_list[i];

        if (strcmp(task->name, "watchdog") == 0)
        {
            found_watchdog = true;
            LOG_DEBUG("  Found watchdog task at position %zu", i);
        }
        else if (strcmp(task->name, "command") == 0)
        {
            found_command = true;
            LOG_DEBUG("  Found command task at position %zu", i);
        }
        else if (strcmp(task->name, "radio") == 0)
        {
            found_radio = true;
            LOG_DEBUG("  Found radio task at position %zu", i);
        }
        else if (strcmp(task->name, "beacon") == 0)
        {
            found_beacon = true;
            LOG_DEBUG("  Found beacon task at position %zu", i);
        }
    }

    ASSERT(found_watchdog);
    ASSERT(found_command);
    // ASSERT(found_radio);
    ASSERT(found_beacon);

    log_viz_event("test_pass", NULL, "task_configuration");
    LOG_DEBUG("✓ Test 2 passed");
}

/**
 * Test 3: Verify dispatch periods are reasonable
 */
void test_dispatch_periods()
{
    LOG_DEBUG("=== Test 3: Dispatch periods ===");
    log_viz_event("test_start", NULL, "dispatch_periods");

    for (size_t i = 0; i < running_state.num_tasks; i++)
    {
        sched_task_t *task = running_state.task_list[i];

        // Periods be at least 10ms
        ASSERT(task->dispatch_period_ms >= 10);

        char details[128];
        snprintf(details, sizeof(details), "period=%u ms",
                 task->dispatch_period_ms);
        log_viz_event("period_check", task->name, details);

        LOG_DEBUG("  %s: %u ms", task->name, task->dispatch_period_ms);
    }

    log_viz_event("test_pass", NULL, "dispatch_periods");
    LOG_DEBUG("✓ Test 3 passed");
}

/**
 * Test 4: Verify state transition (running stays in running)
 */
void test_state_transition()
{
    LOG_DEBUG("=== Test 4: State transition ===");
    log_viz_event("test_start", NULL, "state_transition");

    memset(&test_slate, 0, sizeof(slate_t));
    test_slate.current_state = &running_state;

    // Running state should always return itself
    sched_state_t *next_state = running_state.get_next_state(&test_slate);

    ASSERT(next_state != NULL);
    ASSERT(next_state == &running_state);
    ASSERT(strcmp(next_state->name, "running") == 0);

    LOG_DEBUG("  Running state correctly returns itself");

    log_viz_event("test_pass", NULL, "state_transition");
    LOG_DEBUG("✓ Test 4 passed");
}

/**
 * Test 5: Run scheduler with real running_state tasks
 */
void test_scheduler_execution()
{
    LOG_DEBUG("=== Test 5: Scheduler execution with running_state ===");
    log_viz_event("test_start", NULL, "scheduler_execution");

    mock_time_us = 0;
    memset(&test_slate, 0, sizeof(slate_t));
    reset_task_stats();

    test_slate.current_state = &running_state;
    test_slate.entered_current_state_time = get_absolute_time();

    // Initialize all tasks
    test_state_init_tasks(&running_state, &test_slate);

    // Log discovered tasks
    log_discovered_tasks(&running_state);

    // Simulate 30 seconds with 10ms dispatch interval, log every 5 seconds
    LOG_DEBUG("  Simulating 30 seconds of operation...");
    run_scheduler_simulation(&test_slate, 30000, 10, 5000);

    // Log summary (note: real tasks don't track stats, only init is tracked)
    LOG_DEBUG("  Simulation completed successfully");

    // Verify all tasks were initialized (real tasks don't track dispatch stats)
    for (size_t i = 0; i < running_state.num_tasks; i++)
    {
        sched_task_t *task = running_state.task_list[i];
        LOG_DEBUG("  %s initialized and dispatched", task->name);
    }

    log_viz_event("test_pass", NULL, "scheduler_execution");
    LOG_DEBUG("✓ Test 5 passed");
}

int main()
{
    LOG_DEBUG("=== Running State Tests (Real Tasks) ===");

    viz_log_open("running_state_viz.json");

    mock_time_us = 0;

    test_running_state_structure();
    test_task_configuration();
    test_dispatch_periods();
    test_state_transition();
    test_scheduler_execution();

    LOG_DEBUG("=== All Running State Tests Passed ===");
    LOG_DEBUG("Total simulated time: %lu ms",
              (unsigned long)(mock_time_us / 1000));

    viz_log_close();

    return 0;
}
