/**
 * @author  Joseph Shetaye
 * @date    2024-09-30
 *
 * This file lists all states of the satellite state machine. It should only be
 * included in scheduler.c
 */

#pragma once

#include "scheduler/scheduler.h"

/*
 * Declare all states as extern.
 */
extern sched_state_t init_state;
extern sched_state_t running_state;

/**
 * List of all states. We need this because we cannot enumerate all states at
 * runtime.
 *
 * Note: For each state, the order of the task list determines priority. Tasks
 * nearer the top have higher priority.
 */
static const sched_state_t *all_states[] = {
    &init_state,
    &running_state};
static const sched_state_t *initial_state = &init_state;

/*
 * Must be a macro because it is used to initialize an array
 */
#define num_states (sizeof(all_states) / sizeof(sched_state_t *))
