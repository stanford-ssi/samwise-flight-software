/**
 * @file test_running_state.c
 * @brief Unit tests for the running state FSM with real scheduler
 *
 * This test defines its own states and tasks to verify scheduler behavior
 * with different timings and configurations.
 */

#include "error.h"
#include "logger.h"
#include "pico/stdlib.h"
#include "scheduler.h"
#include "slate.h"
#include "state_machine.h"
#include <stdio.h>
#include <string.h>

// External mock time variable
extern uint64_t mock_time_us;

slate_t test_slate;
FILE *viz_log = NULL;

// Track task executions
typedef struct
{
    const char *task_name;
    uint32_t init_count;
    uint32_t dispatch_count;
    uint32_t last_dispatch_time_ms;
} task_execution_stats_t;

#define MAX_TASKS 16
task_execution_stats_t task_stats[MAX_TASKS];
size_t num_tracked_tasks = 0;

/**
 * Helper function to write JSON log events
 */
void log_viz_event(const char *event_type, const char *task_name,
                   const char *details)
{
    if (viz_log != NULL)
    {
        uint32_t time_ms = (uint32_t)(mock_time_us / 1000);
        fprintf(viz_log,
                "{\"time_ms\": %u, \"event\": \"%s\", \"task\": \"%s\", "
                "\"details\": \"%s\"},\n",
                time_ms, event_type, task_name ? task_name : "",
                details ? details : "");
        fflush(viz_log);
    }
}

/**
 * Get or create task stats entry
 */
task_execution_stats_t *get_task_stats(const char *task_name)
{
    for (size_t i = 0; i < num_tracked_tasks; i++)
    {
        if (strcmp(task_stats[i].task_name, task_name) == 0)
        {
            return &task_stats[i];
        }
    }

    if (num_tracked_tasks < MAX_TASKS)
    {
        task_stats[num_tracked_tasks].task_name = task_name;
        task_stats[num_tracked_tasks].init_count = 0;
        task_stats[num_tracked_tasks].dispatch_count = 0;
        task_stats[num_tracked_tasks].last_dispatch_time_ms = 0;
        return &task_stats[num_tracked_tasks++];
    }

    return NULL;
}

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

// Define stub states that scheduler.c expects
sched_state_t init_state = {
    .name = "init",
    .num_tasks = 0,
    .task_list = {},
    .get_next_state = NULL};

sched_state_t running_state = {
    .name = "running",
    .num_tasks = 0,
    .task_list = {},
    .get_next_state = NULL};

sched_state_t burn_wire_state = {
    .name = "burn_wire",
    .num_tasks = 0,
    .task_list = {},
    .get_next_state = NULL};

sched_state_t burn_wire_reset_state = {
    .name = "burn_wire_reset",
    .num_tasks = 0,
    .task_list = {},
    .get_next_state = NULL};

sched_state_t bringup_state = {
    .name = "bringup",
    .num_tasks = 0,
    .task_list = {},
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

    // Reset
    mock_time_us = 0;
    memset(&test_slate, 0, sizeof(slate_t));
    memset(task_stats, 0, sizeof(task_stats));
    num_tracked_tasks = 0;

    // Set up test state
    test_slate.current_state = &test_state_1;
    test_slate.entered_current_state_time = get_absolute_time();

    // Initialize tasks manually (scheduler would normally do this)
    for (size_t i = 0; i < test_state_1.num_tasks; i++)
    {
        sched_task_t *task = test_state_1.task_list[i];
        task->task_init(&test_slate);
        task->next_dispatch = make_timeout_time_ms(task->dispatch_period_ms);
    }

    // Log task info
    for (size_t i = 0; i < test_state_1.num_tasks; i++)
    {
        sched_task_t *task = test_state_1.task_list[i];
        char details[128];
        snprintf(details, sizeof(details), "period=%u ms, index=%zu",
                 task->dispatch_period_ms, i);
        log_viz_event("task_discovered", task->name, details);
    }

    // Simulate 10 seconds
    const uint32_t simulation_duration_ms = 10000;
    const uint32_t dispatch_interval_ms = 5;

    LOG_DEBUG("Simulating %u ms with dispatch interval %u ms",
              simulation_duration_ms, dispatch_interval_ms);

    for (uint32_t elapsed_ms = 0; elapsed_ms < simulation_duration_ms;
         elapsed_ms += dispatch_interval_ms)
    {
        mock_time_us += dispatch_interval_ms * 1000ULL;
        sched_dispatch(&test_slate);

        if (elapsed_ms % 2000 == 0)
        {
            char details[64];
            snprintf(details, sizeof(details), "elapsed=%u ms", elapsed_ms);
            log_viz_event("simulation_milestone", NULL, details);
        }
    }

    // Verify dispatch counts
    LOG_DEBUG("Task dispatch counts:");
    for (size_t i = 0; i < num_tracked_tasks; i++)
    {
        task_execution_stats_t *stats = &task_stats[i];
        LOG_DEBUG("  %s: %u dispatches", stats->task_name,
                  stats->dispatch_count);

        char details[128];
        snprintf(details, sizeof(details), "dispatches=%u",
                 stats->dispatch_count);
        log_viz_event("task_summary", stats->task_name, details);
    }

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

    // Reset
    mock_time_us = 0;
    memset(&test_slate, 0, sizeof(slate_t));
    memset(task_stats, 0, sizeof(task_stats));
    num_tracked_tasks = 0;

    // Set up test state 2
    test_slate.current_state = &test_state_2;
    test_slate.entered_current_state_time = get_absolute_time();

    // Initialize tasks
    for (size_t i = 0; i < test_state_2.num_tasks; i++)
    {
        sched_task_t *task = test_state_2.task_list[i];
        task->task_init(&test_slate);
        task->next_dispatch = make_timeout_time_ms(task->dispatch_period_ms);
    }

    // Simulate 5 seconds
    const uint32_t simulation_duration_ms = 5000;
    const uint32_t dispatch_interval_ms = 5;

    for (uint32_t elapsed_ms = 0; elapsed_ms < simulation_duration_ms;
         elapsed_ms += dispatch_interval_ms)
    {
        mock_time_us += dispatch_interval_ms * 1000ULL;
        sched_dispatch(&test_slate);
    }

    // Verify only fast tasks executed
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

    // Reset
    mock_time_us = 0;
    memset(&test_slate, 0, sizeof(slate_t));
    memset(task_stats, 0, sizeof(task_stats));
    num_tracked_tasks = 0;

    // Set up test state 3
    test_slate.current_state = &test_state_3;
    test_slate.entered_current_state_time = get_absolute_time();

    // Initialize tasks
    for (size_t i = 0; i < test_state_3.num_tasks; i++)
    {
        sched_task_t *task = test_state_3.task_list[i];
        task->task_init(&test_slate);
        task->next_dispatch = make_timeout_time_ms(task->dispatch_period_ms);
    }

    // Simulate 15 seconds
    const uint32_t simulation_duration_ms = 15000;
    const uint32_t dispatch_interval_ms = 10;

    for (uint32_t elapsed_ms = 0; elapsed_ms < simulation_duration_ms;
         elapsed_ms += dispatch_interval_ms)
    {
        mock_time_us += dispatch_interval_ms * 1000ULL;
        sched_dispatch(&test_slate);
    }

    // Verify only slow tasks executed
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

    // Reset
    mock_time_us = 0;
    memset(&test_slate, 0, sizeof(slate_t));
    memset(task_stats, 0, sizeof(task_stats));
    num_tracked_tasks = 0;

    // Use all tasks state
    test_slate.current_state = &test_state_1;
    test_slate.entered_current_state_time = get_absolute_time();

    // Initialize
    for (size_t i = 0; i < test_state_1.num_tasks; i++)
    {
        sched_task_t *task = test_state_1.task_list[i];
        task->task_init(&test_slate);
        task->next_dispatch = make_timeout_time_ms(task->dispatch_period_ms);
    }

    // Simulate exactly 10 seconds with fine-grained dispatch
    const uint32_t test_duration_ms = 10000;
    const uint32_t dispatch_interval_ms = 1; // 1ms precision

    for (uint32_t elapsed_ms = 0; elapsed_ms < test_duration_ms;
         elapsed_ms += dispatch_interval_ms)
    {
        mock_time_us += dispatch_interval_ms * 1000ULL;
        sched_dispatch(&test_slate);
    }

    // Check expected dispatch counts
    for (size_t i = 0; i < test_state_1.num_tasks; i++)
    {
        sched_task_t *task = test_state_1.task_list[i];
        task_execution_stats_t *stats = get_task_stats(task->name);

        uint32_t expected_dispatches = test_duration_ms / task->dispatch_period_ms;
        uint32_t actual_dispatches = stats ? stats->dispatch_count : 0;

        LOG_DEBUG("  %s: expected ~%u, actual %u", task->name,
                  expected_dispatches, actual_dispatches);

        char details[128];
        snprintf(details, sizeof(details),
                 "expected=%u, actual=%u, period=%u ms", expected_dispatches,
                 actual_dispatches, task->dispatch_period_ms);
        log_viz_event("period_accuracy_check", task->name, details);

        // For very low frequency tasks (<=2 dispatches), allow greater tolerance
        uint32_t tolerance = (expected_dispatches <= 2) ? expected_dispatches : 5;
        ASSERT(actual_dispatches >= expected_dispatches - tolerance &&
               actual_dispatches <= expected_dispatches + tolerance);
    }

    log_viz_event("test_pass", NULL, "task_period_accuracy");
    LOG_DEBUG("✓ Test 4 passed");
}

int main()
{
    LOG_DEBUG("=== Running State FSM Tests (Real Scheduler with Custom States) "
              "===");

    // Open visualization log file
    viz_log = fopen("running_state_viz.json", "w");
    if (viz_log != NULL)
    {
        fprintf(viz_log, "{\n\"events\": [\n");
        LOG_DEBUG("Visualization log opened: running_state_viz.json");
    }

    // Initialize mock time
    mock_time_us = 0;

    // Run tests
    test_all_tasks_different_periods();
    test_fast_tasks_only();
    test_slow_tasks_only();
    test_task_period_accuracy();

    LOG_DEBUG("=== All Tests Passed ===");
    LOG_DEBUG("Total simulated time: %lu ms",
              (unsigned long)(mock_time_us / 1000));

    // Close visualization log file
    if (viz_log != NULL)
    {
        // Remove trailing comma and close JSON
        fseek(viz_log, -2, SEEK_CUR);
        fprintf(viz_log, "\n]\n}\n");
        fclose(viz_log);
        LOG_DEBUG("Visualization log closed");
    }

    return 0;
}
