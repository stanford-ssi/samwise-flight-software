/**
 * @file test_scheduler_helpers.c
 * @brief Implementation of shared test infrastructure for scheduler testing
 */

#include "test_scheduler_helpers.h"
#include "logger.h"
#include "pico/stdlib.h"
#include <string.h>

// =============================================================================
// TASK EXECUTION TRACKING
// =============================================================================

task_execution_stats_t task_stats[MAX_TRACKED_TASKS];
size_t num_tracked_tasks = 0;

task_execution_stats_t *get_task_stats(const char *task_name)
{
    // Find existing
    for (size_t i = 0; i < num_tracked_tasks; i++)
    {
        if (strcmp(task_stats[i].task_name, task_name) == 0)
        {
            return &task_stats[i];
        }
    }

    // Create new
    if (num_tracked_tasks < MAX_TRACKED_TASKS)
    {
        task_stats[num_tracked_tasks].task_name = task_name;
        task_stats[num_tracked_tasks].init_count = 0;
        task_stats[num_tracked_tasks].dispatch_count = 0;
        task_stats[num_tracked_tasks].last_dispatch_time_ms = 0;
        return &task_stats[num_tracked_tasks++];
    }

    return NULL;
}

void reset_task_stats(void)
{
    memset(task_stats, 0, sizeof(task_stats));
    num_tracked_tasks = 0;
}

// =============================================================================
// VISUALIZATION LOGGING
// =============================================================================

// External references (defined in test_mocks/logger.c)
extern FILE *viz_log;
extern const char *current_executing_task;

int viz_log_open(const char *filename)
{
    viz_log = fopen(filename, "w");
    if (viz_log != NULL)
    {
        fprintf(viz_log, "{\n\"events\": [\n");
        LOG_DEBUG("Visualization log opened: %s", filename);
        return 0;
    }
    return -1;
}

void viz_log_close(void)
{
    if (viz_log != NULL)
    {
        // Remove trailing comma and close JSON
        fseek(viz_log, -2, SEEK_CUR);
        fprintf(viz_log, "\n]\n}\n");
        fclose(viz_log);
        viz_log = NULL;
        LOG_DEBUG("Visualization log closed");
    }
}

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

// =============================================================================
// TEST STATE HELPERS
// =============================================================================

void test_state_init_tasks(sched_state_t *state, slate_t *slate)
{
    for (size_t i = 0; i < state->num_tasks; i++)
    {
        sched_task_t *task = state->task_list[i];
        task->task_init(slate);
        task->next_dispatch = make_timeout_time_ms(task->dispatch_period_ms);

        // Log task initialization for visualization
        log_viz_event("task_init", task->name, "initialized");
    }
}

void test_sched_dispatch(slate_t *slate)
{
    sched_state_t *current_state_info = slate->current_state;

    // Loop through all of this state's tasks
    for (size_t i = 0; i < current_state_info->num_tasks; i++)
    {
        sched_task_t *task = current_state_info->task_list[i];

        // Check if this task is due and if so, dispatch it
        if (absolute_time_diff_us(task->next_dispatch, get_absolute_time()) > 0)
        {
            task->next_dispatch =
                make_timeout_time_ms(task->dispatch_period_ms);

            // Log task start BEFORE dispatching
            char details[64];
            snprintf(details, sizeof(details), "time=%u ms",
                     (uint32_t)(mock_time_us / 1000));
            log_viz_event("task_start", task->name, details);

            // Set current task context for log capture
            current_executing_task = task->name;

            // Actually dispatch the task
            task->task_dispatch(slate);

            // Clear task context
            current_executing_task = NULL;

            // Log task end AFTER dispatching
            log_viz_event("task_end", task->name, details);
        }
    }

    slate->time_in_current_state_ms =
        absolute_time_diff_us(slate->entered_current_state_time,
                              get_absolute_time()) /
        1000;

    // Handle state transitions (simplified from scheduler.c)
    sched_state_t *next_state;
    if (slate->manual_override_state != NULL)
    {
        next_state = slate->manual_override_state;
        slate->manual_override_state = NULL;
    }
    else
    {
        next_state = current_state_info->get_next_state(slate);
    }

    if (next_state != current_state_info)
    {
        slate->current_state = next_state;
        slate->entered_current_state_time = get_absolute_time();
        slate->time_in_current_state_ms = 0;
    }
}

void run_scheduler_simulation(slate_t *slate, uint32_t duration_ms,
                               uint32_t dispatch_interval_ms,
                               uint32_t log_interval_ms)
{
    LOG_DEBUG("Simulating %u ms with dispatch interval %u ms", duration_ms,
              dispatch_interval_ms);

    for (uint32_t elapsed_ms = 0; elapsed_ms < duration_ms;
         elapsed_ms += dispatch_interval_ms)
    {
        mock_time_us += dispatch_interval_ms * 1000ULL;

        // Use test version of dispatcher that logs task start/end
        test_sched_dispatch(slate);

        if (log_interval_ms > 0 && elapsed_ms % log_interval_ms == 0)
        {
            char details[64];
            snprintf(details, sizeof(details), "elapsed=%u ms", elapsed_ms);
            log_viz_event("simulation_milestone", NULL, details);
        }
    }
}

int verify_dispatch_count(const char *task_name, uint32_t expected,
                          uint32_t tolerance)
{
    task_execution_stats_t *stats = get_task_stats(task_name);
    if (stats == NULL)
    {
        LOG_DEBUG("Task %s not found in stats", task_name);
        return 0;
    }

    uint32_t actual = stats->dispatch_count;
    int in_range = (actual >= expected - tolerance &&
                    actual <= expected + tolerance);

    LOG_DEBUG("  %s: expected ~%u, actual %u [%s]", task_name, expected, actual,
              in_range ? "PASS" : "FAIL");

    return in_range;
}

void log_discovered_tasks(sched_state_t *state)
{
    for (size_t i = 0; i < state->num_tasks; i++)
    {
        sched_task_t *task = state->task_list[i];
        char details[128];
        snprintf(details, sizeof(details), "period=%u ms, index=%zu",
                 task->dispatch_period_ms, i);
        log_viz_event("task_discovered", task->name, details);
    }
}

void log_task_summary(void)
{
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
}
