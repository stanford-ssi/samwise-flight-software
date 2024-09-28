/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 *
 * This file defines the types and global declarations for the satellite state
 * machine.
 */

#pragma once

#include "../slate.h"
#include "pico/types.h"

#define MAX_TASKS_PER_STATE 10

/**
 * A list of all states the state machine can be in. Each state dispatches a set
 * of tasks at regular intervals.
 */
typedef enum sm_state
{
    state_init, /* Entered by default at boot */
    state_running,

    /* Auto-updates to the number of sates */
    num_states

} sm_state_t;

/**
 * Holds the info for a single task. A single can belong to multiple states.
 */
typedef struct sm_task
{
    const char *name;
    const uint32_t dispatch_period_ms;

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

} sm_task_t;

/**
 * Holds the info for defining a state.
 */
typedef struct sm_state_info
{
    const char *name;

    size_t num_tasks;
    sm_task_t *task_list[MAX_TASKS_PER_STATE];

    /**
     * Called each time the state dispatches.
     * @param slate     Pointer to the current satellite slate
     * @return The next state to transition to
     */
    sm_state_t (*get_next_state)(slate_t *slate);
} sm_state_info_t;

void sm_init(slate_t *slate);
void sm_dispatch(slate_t *slate);