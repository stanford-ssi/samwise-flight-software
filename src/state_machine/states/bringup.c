#include "macros.h"
#ifdef BRINGUP

#include "state_machine/states/bringup.h"
#include "state_machine/states/states.h"

sched_state_t *bringup_get_next_state(slate_t *slate)
{
    return &bringup_state;
}

sched_state_t bringup_state = {.name = "bringup",
                              .num_tasks = 0,
                              .task_list = {},
                              .get_next_state = &bringup_get_next_state};
                              
#endif
