/**
 * @file test_scheduler_helpers.c
 * @brief Implementation of shared test infrastructure for scheduler testing
 */

#include "test_scheduler_helpers.h"
#include "logger.h"
#include "pico/stdlib.h"
#include "state_ids.h"
#include "state_registry.h"
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

int viz_log_open_raw(const char *filename)
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

int viz_log_open_log_dir(const char *basename)
{
    // Build output filename, writing to TEST_UNDECLARED_OUTPUTS_DIR if
    // available so Bazel preserves the JSON after the test run.
    const char *out_dir = getenv("TEST_UNDECLARED_OUTPUTS_DIR");

    // Replace hyphens with underscores for valid filenames
    for (char *p = basename; *p; p++)
    {
        if (*p == '-')
            *p = '_';
    }

    if (out_dir != NULL)
    {
        char filename[strlen(out_dir) + 1 + strlen(basename) + 1];
        snprintf(filename, sizeof(filename), "%s/%s", out_dir, basename);
        return viz_log_open_raw(filename);
    }

    return viz_log_open_raw(basename);
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
    sched_state_t *current_state_info =
        state_registry_get(slate->current_state_id);

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
    state_id_t next_state_id;
    if (slate->manual_override_state_id != STATE_NONE)
    {
        next_state_id = slate->manual_override_state_id;
        slate->manual_override_state_id = STATE_NONE;
    }
    else
    {
        next_state_id = current_state_info->get_next_state(slate);
    }

    if (next_state_id != slate->current_state_id)
    {
        slate->current_state_id = next_state_id;
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
    int in_range =
        (actual >= expected - tolerance && actual <= expected + tolerance);

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

// =============================================================================
// FSM SIMULATION
// =============================================================================

state_id_t run_fsm_simulation(slate_t *slate, uint32_t dispatch_interval_ms,
                              uint32_t log_interval_ms,
                              int stable_count_threshold)
{
    LOG_DEBUG("FSM simulation: dispatch interval %u ms, stable threshold %d",
              dispatch_interval_ms, stable_count_threshold);

    int consecutive_same_state = 0;
    state_id_t prev_state_id = slate->current_state_id;
    uint32_t elapsed_ms = 0;

    // Log initial state entry
    sched_state_t *initial_state = state_registry_get(slate->current_state_id);
    if (initial_state != NULL)
    {
        char details[128];
        snprintf(details, sizeof(details), "state=%s, from=none",
                 initial_state->name);
        log_viz_event("state_enter", NULL, details);
        log_discovered_tasks(initial_state);
    }

    while (consecutive_same_state < stable_count_threshold)
    {
        mock_time_us += dispatch_interval_ms * 1000ULL;
        elapsed_ms += dispatch_interval_ms;

        // Use test version of dispatcher that logs task start/end
        test_sched_dispatch(slate);

        // Check for state transition
        if (slate->current_state_id != prev_state_id)
        {
            sched_state_t *old_state = state_registry_get(prev_state_id);
            sched_state_t *new_state =
                state_registry_get(slate->current_state_id);

            // Log state exit
            char details[128];
            snprintf(details, sizeof(details), "state=%s, to=%s",
                     old_state ? old_state->name : "unknown",
                     new_state ? new_state->name : "unknown");
            log_viz_event("state_exit", NULL, details);

            // Log state enter
            snprintf(details, sizeof(details), "state=%s, from=%s",
                     new_state ? new_state->name : "unknown",
                     old_state ? old_state->name : "unknown");
            log_viz_event("state_enter", NULL, details);

            // Log tasks for the new state
            if (new_state != NULL)
            {
                log_discovered_tasks(new_state);
            }

            LOG_DEBUG("FSM: %s -> %s at %u ms",
                      old_state ? old_state->name : "?",
                      new_state ? new_state->name : "?", elapsed_ms);

            prev_state_id = slate->current_state_id;
            consecutive_same_state = 0;
        }
        else
        {
            consecutive_same_state++;
        }

        if (log_interval_ms > 0 && elapsed_ms % log_interval_ms == 0)
        {
            char details[64];
            snprintf(details, sizeof(details), "elapsed=%u ms", elapsed_ms);
            log_viz_event("simulation_milestone", NULL, details);
        }

        // Safety limit: 5 minutes of simulated time
        if (elapsed_ms > 300000)
        {
            LOG_DEBUG("FSM simulation: safety limit reached at %u ms",
                      elapsed_ms);
            break;
        }
    }

    sched_state_t *final_state = state_registry_get(slate->current_state_id);
    LOG_DEBUG("FSM simulation: stable in state %s after %u ms",
              final_state ? final_state->name : "unknown", elapsed_ms);

    return slate->current_state_id;
}
