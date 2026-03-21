#ifdef BRINGUP
#include "bringup_state.h"

/*
 * Code for the "bringup" state, which we enter from init, never to return if
 * the BRINGUP symbol is defined.
 */

state_id_t bringup_get_next_state(slate_t *slate)
{
    return STATE_BRINGUP;
}

sched_state_t bringup_state = {
    .name = "bringup",
    .id = STATE_BRINGUP,
    .num_tasks = 3,
    .task_list = {&diagnostics_task, &hardware_test_task, &watchdog_task},
    .get_next_state = &bringup_get_next_state};

#endif
