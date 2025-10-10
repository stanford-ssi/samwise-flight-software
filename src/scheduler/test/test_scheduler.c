/**
 * @file test_scheduler.c
 * @brief Unit tests for the scheduler with custom test tasks
 *
 * Tests scheduler behavior with different task timings and configurations.
 * This is independent of any specific state - it tests the core scheduler
 * dispatch logic.
 */

#include "error.h"
#include "logger.h"
#include "pico/stdlib.h"
#include "scheduler.h"
#include "test_scheduler_helpers.h"
#include <stdio.h>
#include <string.h>

slate_t test_slate;

// =============================================================================
// TEST TASK DEFINITIONS
// =============================================================================

// Fast task - 50ms period
void fast_task_init(slate_t *slate)
{
    task_execution_stats_t *stats = get_task_stats("fast_task");
    if (stats)
        stats->init_count++;
    log_viz_event("task_init", "fast_task", "50ms period");
    LOG_DEBUG("fast_task initialized");
}

void fast_task_dispatch(slate_t *slate)
{
    task_execution_stats_t *stats = get_task_stats("fast_task");
    if (stats)
    {
        stats->dispatch_count++;
        stats->last_dispatch_time_ms = (uint32_t)(mock_time_us / 1000);
    }

    char details[64];
    snprintf(details, sizeof(details), "dispatch_count=%u",
             stats ? stats->dispatch_count : 0);
    log_viz_event("task_dispatch", "fast_task", details);
}

sched_task_t fast_task = {
    .name = "fast_task",
    .dispatch_period_ms = 50,
    .next_dispatch = 0,
    .task_init = fast_task_init,
    .task_dispatch = fast_task_dispatch};

// Medium task - 200ms period
void medium_task_init(slate_t *slate)
{
    task_execution_stats_t *stats = get_task_stats("medium_task");
    if (stats)
        stats->init_count++;
    log_viz_event("task_init", "medium_task", "200ms period");
    LOG_DEBUG("medium_task initialized");
}

void medium_task_dispatch(slate_t *slate)
{
    task_execution_stats_t *stats = get_task_stats("medium_task");
    if (stats)
    {
        stats->dispatch_count++;
        stats->last_dispatch_time_ms = (uint32_t)(mock_time_us / 1000);
    }

    char details[64];
    snprintf(details, sizeof(details), "dispatch_count=%u",
             stats ? stats->dispatch_count : 0);
    log_viz_event("task_dispatch", "medium_task", details);
}

sched_task_t medium_task = {
    .name = "medium_task",
    .dispatch_period_ms = 200,
    .next_dispatch = 0,
    .task_init = medium_task_init,
    .task_dispatch = medium_task_dispatch};

// Slow task - 1000ms period
void slow_task_init(slate_t *slate)
{
    task_execution_stats_t *stats = get_task_stats("slow_task");
    if (stats)
        stats->init_count++;
    log_viz_event("task_init", "slow_task", "1000ms period");
    LOG_DEBUG("slow_task initialized");
}

void slow_task_dispatch(slate_t *slate)
{
    task_execution_stats_t *stats = get_task_stats("slow_task");
    if (stats)
    {
        stats->dispatch_count++;
        stats->last_dispatch_time_ms = (uint32_t)(mock_time_us / 1000);
    }

    char details[64];
    snprintf(details, sizeof(details), "dispatch_count=%u",
             stats ? stats->dispatch_count : 0);
    log_viz_event("task_dispatch", "slow_task", details);
}

sched_task_t slow_task = {
    .name = "slow_task",
    .dispatch_period_ms = 1000,
    .next_dispatch = 0,
    .task_init = slow_task_init,
    .task_dispatch = slow_task_dispatch};

// Very slow task - 5000ms period
void very_slow_task_init(slate_t *slate)
{
    task_execution_stats_t *stats = get_task_stats("very_slow_task");
    if (stats)
        stats->init_count++;
    log_viz_event("task_init", "very_slow_task", "5000ms period");
    LOG_DEBUG("very_slow_task initialized");
}

void very_slow_task_dispatch(slate_t *slate)
{
    task_execution_stats_t *stats = get_task_stats("very_slow_task");
    if (stats)
    {
        stats->dispatch_count++;
        stats->last_dispatch_time_ms = (uint32_t)(mock_time_us / 1000);
    }

    char details[64];
    snprintf(details, sizeof(details), "dispatch_count=%u",
             stats ? stats->dispatch_count : 0);
    log_viz_event("task_dispatch", "very_slow_task", details);
}

sched_task_t very_slow_task = {
    .name = "very_slow_task",
    .dispatch_period_ms = 5000,
    .next_dispatch = 0,
    .task_init = very_slow_task_init,
    .task_dispatch = very_slow_task_dispatch};

// =============================================================================
// TEST STATE DEFINITIONS
// =============================================================================

// Forward declarations
extern sched_state_t test_state_1;
extern sched_state_t test_state_2;
extern sched_state_t test_state_3;

// Test state 1: All tasks active
sched_state_t *test_state_1_get_next_state(slate_t *slate)
{
    return &test_state_1;
}

sched_state_t test_state_1 = {
    .name = "test_state_1_all_tasks",
    .num_tasks = 4,
    .task_list = {&fast_task, &medium_task, &slow_task, &very_slow_task},
    .get_next_state = test_state_1_get_next_state};

// Test state 2: Only fast tasks
sched_state_t *test_state_2_get_next_state(slate_t *slate)
{
    return &test_state_2;
}

sched_state_t test_state_2 = {
    .name = "test_state_2_fast_only",
    .num_tasks = 2,
    .task_list = {&fast_task, &medium_task},
    .get_next_state = test_state_2_get_next_state};

// Test state 3: Only slow tasks
sched_state_t *test_state_3_get_next_state(slate_t *slate)
{
    return &test_state_3;
}

sched_state_t test_state_3 = {
    .name = "test_state_3_slow_only",
    .num_tasks = 2,
    .task_list = {&slow_task, &very_slow_task},
    .get_next_state = test_state_3_get_next_state};

// Stub running_state for scheduler.c
sched_state_t running_state = {
    .name = "running",
    .num_tasks = 0,
    .task_list = {NULL},
    .get_next_state = NULL};

// =============================================================================
// TESTS
// =============================================================================

/**
 * Test 1: All tasks with different periods
 */
void test_all_tasks_different_periods()
{
    LOG_DEBUG("=== Test 1: All tasks with different periods ===");
    log_viz_event("test_start", NULL, "all_tasks_different_periods");

    mock_time_us = 0;
    memset(&test_slate, 0, sizeof(slate_t));
    reset_task_stats();

    test_slate.current_state = &test_state_1;
    test_slate.entered_current_state_time = get_absolute_time();

    test_state_init_tasks(&test_state_1, &test_slate);
    log_discovered_tasks(&test_state_1);

    // Simulate 10 seconds with 5ms dispatch interval, log every 2 seconds
    run_scheduler_simulation(&test_slate, 10000, 5, 2000);

    log_task_summary();

    // Verify expected counts (with tolerance)
    task_execution_stats_t *fast = get_task_stats("fast_task");
    task_execution_stats_t *medium = get_task_stats("medium_task");
    task_execution_stats_t *slow = get_task_stats("slow_task");
    task_execution_stats_t *vslow = get_task_stats("very_slow_task");

    ASSERT(fast->dispatch_count >= 180 && fast->dispatch_count <= 220);   // ~200
    ASSERT(medium->dispatch_count >= 45 && medium->dispatch_count <= 55); // ~50
    ASSERT(slow->dispatch_count >= 8 && slow->dispatch_count <= 12);      // ~10
    ASSERT(vslow->dispatch_count >= 1 && vslow->dispatch_count <= 3);     // ~2

    log_viz_event("test_pass", NULL, "all_tasks_different_periods");
    LOG_DEBUG("✓ Test 1 passed");
}

/**
 * Test 2: Only fast tasks
 */
void test_fast_tasks_only()
{
    LOG_DEBUG("=== Test 2: Only fast tasks ===");
    log_viz_event("test_start", NULL, "fast_tasks_only");

    mock_time_us = 0;
    memset(&test_slate, 0, sizeof(slate_t));
    reset_task_stats();

    test_slate.current_state = &test_state_2;
    test_slate.entered_current_state_time = get_absolute_time();

    test_state_init_tasks(&test_state_2, &test_slate);

    run_scheduler_simulation(&test_slate, 5000, 5, 0);

    task_execution_stats_t *fast = get_task_stats("fast_task");
    task_execution_stats_t *medium = get_task_stats("medium_task");
    task_execution_stats_t *slow = get_task_stats("slow_task");
    task_execution_stats_t *vslow = get_task_stats("very_slow_task");

    ASSERT(fast != NULL && fast->dispatch_count >= 90);
    ASSERT(medium != NULL && medium->dispatch_count >= 20);
    ASSERT(slow == NULL || slow->dispatch_count == 0);
    ASSERT(vslow == NULL || vslow->dispatch_count == 0);

    LOG_DEBUG("  fast_task: %u dispatches", fast->dispatch_count);
    LOG_DEBUG("  medium_task: %u dispatches", medium->dispatch_count);

    log_viz_event("test_pass", NULL, "fast_tasks_only");
    LOG_DEBUG("✓ Test 2 passed");
}

/**
 * Test 3: Only slow tasks
 */
void test_slow_tasks_only()
{
    LOG_DEBUG("=== Test 3: Only slow tasks ===");
    log_viz_event("test_start", NULL, "slow_tasks_only");

    mock_time_us = 0;
    memset(&test_slate, 0, sizeof(slate_t));
    reset_task_stats();

    test_slate.current_state = &test_state_3;
    test_slate.entered_current_state_time = get_absolute_time();

    test_state_init_tasks(&test_state_3, &test_slate);

    run_scheduler_simulation(&test_slate, 15000, 10, 0);

    task_execution_stats_t *fast = get_task_stats("fast_task");
    task_execution_stats_t *medium = get_task_stats("medium_task");
    task_execution_stats_t *slow = get_task_stats("slow_task");
    task_execution_stats_t *vslow = get_task_stats("very_slow_task");

    ASSERT(fast == NULL || fast->dispatch_count == 0);
    ASSERT(medium == NULL || medium->dispatch_count == 0);
    ASSERT(slow != NULL && slow->dispatch_count >= 14);
    ASSERT(vslow != NULL && vslow->dispatch_count >= 2);

    LOG_DEBUG("  slow_task: %u dispatches", slow->dispatch_count);
    LOG_DEBUG("  very_slow_task: %u dispatches", vslow->dispatch_count);

    log_viz_event("test_pass", NULL, "slow_tasks_only");
    LOG_DEBUG("✓ Test 3 passed");
}

/**
 * Test 4: Verify task periods are accurate
 */
void test_task_period_accuracy()
{
    LOG_DEBUG("=== Test 4: Task period accuracy ===");
    log_viz_event("test_start", NULL, "task_period_accuracy");

    mock_time_us = 0;
    memset(&test_slate, 0, sizeof(slate_t));
    reset_task_stats();

    test_slate.current_state = &test_state_1;
    test_slate.entered_current_state_time = get_absolute_time();

    test_state_init_tasks(&test_state_1, &test_slate);

    // Simulate exactly 10 seconds with fine-grained dispatch
    run_scheduler_simulation(&test_slate, 10000, 1, 0);

    for (size_t i = 0; i < test_state_1.num_tasks; i++)
    {
        sched_task_t *task = test_state_1.task_list[i];
        task_execution_stats_t *stats = get_task_stats(task->name);

        uint32_t expected_dispatches = 10000 / task->dispatch_period_ms;
        uint32_t actual_dispatches = stats ? stats->dispatch_count : 0;

        // For very low frequency tasks (<=2 dispatches), allow greater tolerance
        uint32_t tolerance = (expected_dispatches <= 2) ? expected_dispatches : 5;

        char details[128];
        snprintf(details, sizeof(details),
                 "expected=%u, actual=%u, period=%u ms", expected_dispatches,
                 actual_dispatches, task->dispatch_period_ms);
        log_viz_event("period_accuracy_check", task->name, details);

        ASSERT(verify_dispatch_count(task->name, expected_dispatches, tolerance));
    }

    log_viz_event("test_pass", NULL, "task_period_accuracy");
    LOG_DEBUG("✓ Test 4 passed");
}

int main()
{
    LOG_DEBUG("=== Scheduler Tests (Custom Tasks) ===");

    viz_log_open("scheduler_test_viz.json");

    mock_time_us = 0;

    test_all_tasks_different_periods();
    test_fast_tasks_only();
    test_slow_tasks_only();
    test_task_period_accuracy();

    LOG_DEBUG("=== All Scheduler Tests Passed ===");
    LOG_DEBUG("Total simulated time: %lu ms",
              (unsigned long)(mock_time_us / 1000));

    viz_log_close();

    return 0;
}
