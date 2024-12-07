#include "macros.h"
#ifdef BRINGUP

#include "state_machine/states/bringup.h"
#include "state_machine/states/states.h"
#include "state_machine/tasks/tasks.h"

sched_state_t *bringup_get_next_state(slate_t *slate)
{
    return &bringup_state;
}

sched_state_t bringup_state = {.name = "bringup",
                               .num_tasks = 1,
                               .task_list = {&scan_task},
                               .get_next_state = &bringup_get_next_state};

#endif
