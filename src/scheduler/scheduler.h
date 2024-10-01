/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 *
 * This file defines the types and global declarations for the satellite state
 * machine.
 */

#pragma once

#include "slate.h"
#include "pico/types.h"

#define MAX_TASKS_PER_STATE 10

enum sm_state;
typedef enum sm_state sm_state_t;

/**
 * Holds the info for a single task. A single task can belong to multiple states.
 */
typedef struct sched_task
{
    /**
     * Friendly name for the task.
     */
    const char *name;

    /**
     * Minimum number of milliseconds between dispatches of this task.
     */
    const uint32_t dispatch_period_ms;

    /**
     * Earliest time this task can be dispatched.
     */
    absolute_time_t next_dispatch;

    /**
     * Called once when the task initializes.
     * @param slate     Pointer to the current satellite slate
     */
    void (*task_init)(slate_t *slate);

    /**
     * Called each time the task dispatches.
     * @param slate     Pointer to the current satellite slate
     */
    void (*task_dispatch)(slate_t *slate);

} sched_task_t;

/**
 * Holds the info for defining a state.
 */
typedef struct sched_state_info
{
    /**
     * Friendly name for the state.
     */
    const char *name;

    size_t num_tasks;
    sched_task_t *task_list[MAX_TASKS_PER_STATE];

    /**
     * Called each time the state dispatches.
     * @param slate     Pointer to the current satellite slate
     * @return The next state to transition to
     */
    sm_state_t (*get_next_state)(slate_t *slate);
} sched_state_info_t;

void sched_init(slate_t *slate);
void sched_dispatch(slate_t *slate);
