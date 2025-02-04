#ifdef BRINGUP
#include "bringup_state.h"

/*
 * Code for the "bringup" state, which we enter from init, never to return if
 * the BRINGUP symbol is defined.
 */

sched_state_t *bringup_get_next_state(slate_t *slate)
{
    return &bringup_state;
}

sched_state_t bringup_state = {.name = "bringup",
                               .num_tasks = 1,
                               .task_list = {&diagnostics_task},
                               .get_next_state = &bringup_get_next_state};

#endif
