/**
 * @author  Niklas Vainio
 * @date    2024-08-27
 *
 * This file defines all the states in the state machine. Each state is defined
 * along with its transition function.
 *
 * IMPORTANT: Make sure every state here is accounted for in the sm_state enum
 * in state_machine.h
 */

#pragma once

#include "state_machine.h"

/*
 * Tasks are built from the tasks folder, which we cannot depend on to avoid
 * circularity.
 */
extern sm_task_t print_task;
extern sm_task_t blink_task;

/**
 * List of all tasks.
 */
sm_task_t *all_tasks[] = {&print_task, &blink_task};

#define NUM_TASKS sizeof(all_tasks) / sizeof(all_tasks[0])

/**
 * Transition functions for all states
 */
static sm_state_t init_get_next_state(slate_t *slate)
{
    return state_running;
}
static sm_state_t running_get_next_state(slate_t *slate)
{
    return state_running;
}

/**
 * List of all states, in the same order as the sm_state_t enum.
 *
 * Note: For each state, the order of the task list determines priority. Tasks
 * nearer the top have higher priority.
 */
static sm_state_info_t all_states[] = {
    /* state_init */
    {.name = "init",
     .num_tasks = 0,
     .task_list = {},
     .get_next_state = &init_get_next_state},

    /* state_running */
    {.name = "running",
     .num_tasks = 2,
     .task_list = {&print_task, &blink_task},
     .get_next_state = &running_get_next_state}};

static_assert(sizeof(all_states) == sizeof(sm_state_info_t) * num_states);