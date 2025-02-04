#include "state_machine/states/init.h"
#include "state_machine/states/states.h"

extern sched_state_t bringup_state;

sched_state_t *init_get_next_state(slate_t *slate)
{
#ifdef BRINGUP
    return &bringup_state;
#else
    return &running_state;
#endif
}

sched_state_t init_state = {.name = "init",
                            .num_tasks = 0,
                            .task_list = {},
                            .get_next_state = &init_get_next_state};
