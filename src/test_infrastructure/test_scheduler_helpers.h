/**
 * @file test_scheduler_helpers.h
 * @brief Shared test infrastructure for scheduler state testing
 * 
 * This provides common utilities for testing different states with the
 * real scheduler, including time simulation, task execution tracking,
 * and visualization logging.
 */

#pragma once

#include "scheduler.h"
#include "slate.h"
#include "state_machine.h"
#include <stdint.h>
#include <stdio.h>

// External mock time variable (defined in test_mocks)
extern uint64_t mock_time_us;

// =============================================================================
// TASK EXECUTION TRACKING
// =============================================================================

/**
 * Structure to track task execution statistics during tests
 */
typedef struct
{
    const char *task_name;
    uint32_t init_count;
    uint32_t dispatch_count;
    uint32_t last_dispatch_time_ms;
} task_execution_stats_t;

#define MAX_TRACKED_TASKS 16

/**
 * Global task statistics tracking
 */
extern task_execution_stats_t task_stats[MAX_TRACKED_TASKS];
extern size_t num_tracked_tasks;

/**
 * Get or create task stats entry
 * @param task_name Name of the task to track
 * @return Pointer to task stats, or NULL if limit reached
 */
task_execution_stats_t *get_task_stats(const char *task_name);

/**
 * Reset task statistics tracking
 */
void reset_task_stats(void);

// =============================================================================
// VISUALIZATION LOGGING
// =============================================================================

/**
 * Global visualization log file handle
 */
extern FILE *viz_log;

/**
 * Track currently executing task for log capture (defined in test_mocks/logger.c)
 */
extern const char *current_executing_task;

/**
 * Open visualization log file for writing
 * @param filename Path to the log file
 * @return 0 on success, -1 on error
 */
int viz_log_open(const char *filename);

/**
 * Close visualization log file
 */
void viz_log_close(void);

/**
 * Write an event to the visualization log
 * @param event_type Type of event (e.g., "test_start", "task_dispatch")
 * @param task_name Name of task involved (can be NULL)
 * @param details Additional details string (can be NULL)
 */
void log_viz_event(const char *event_type, const char *task_name,
                   const char *details);

// =============================================================================
// TEST STATE HELPERS
// =============================================================================

/**
 * Initialize a test state with tasks
 * @param state State to initialize
 * @param slate Test slate
 */
void test_state_init_tasks(sched_state_t *state, slate_t *slate);

/**
 * Test version of sched_dispatch that logs task execution
 * @param slate Test slate
 */
void test_sched_dispatch(slate_t *slate);

/**
 * Run scheduler dispatch loop for a duration
 * @param slate Test slate
 * @param duration_ms Simulation duration in milliseconds
 * @param dispatch_interval_ms How often to call sched_dispatch
 * @param log_interval_ms How often to log milestones (0 = no milestones)
 */
void run_scheduler_simulation(slate_t *slate, uint32_t duration_ms,
                               uint32_t dispatch_interval_ms,
                               uint32_t log_interval_ms);

/**
 * Verify task dispatch counts are within expected range
 * @param task_name Name of task to check
 * @param expected Expected number of dispatches
 * @param tolerance Allowed deviation (+/-)
 * @return 1 if within range, 0 if not
 */
int verify_dispatch_count(const char *task_name, uint32_t expected,
                          uint32_t tolerance);

/**
 * Log all discovered tasks to visualization log
 * @param state State containing tasks to log
 */
void log_discovered_tasks(sched_state_t *state);

/**
 * Log task execution summary
 */
void log_task_summary(void);

// =============================================================================
// STUB STATE DEFINITIONS
// =============================================================================

/**
 * Define stub states that scheduler.c expects but aren't used in tests.
 * These can be referenced in test files or defined there.
 */
extern sched_state_t init_state;
extern sched_state_t running_state;
extern sched_state_t burn_wire_state;
extern sched_state_t burn_wire_reset_state;
extern sched_state_t bringup_state;

