/**
 * @author  Joseph Shetaye
 * @date    2024-09-30
 *
 * This file lists all states of the satellite state machine. It should only be
 * included in scheduler.c
 */

#pragma once

#include "scheduler/scheduler.h"

enum sm_state
{
    state_init, /* Entered by default at boot */
    state_running,

    /* Auto-updates to the number of sates */
    num_states
};

/*
 * Declare all states as extern.
 */
extern sched_state_info_t init_state_info;
extern sched_state_info_t running_state_info;

/**
 * List of all states, in the same order as the sm_state_t enum.
 *
 * Note: For each state, the order of the task list determines priority. Tasks
 * nearer the top have higher priority.
 */
static sched_state_info_t* all_states[] = {
    /* state_init */
    &init_state_info,
    /* state_running */
    &running_state_info};

/*
 * Static sanity check.
 */
static_assert(sizeof(all_states) == sizeof(sched_state_info_t*) * num_states);