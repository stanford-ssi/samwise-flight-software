#include "state_machine/states/init.h"
#include "state_machine/states/states.h"

sched_state_info_t* init_get_next_state(slate_t *slate)
{
    return &running_state_info;
}

sched_state_info_t init_state_info = {.name = "init",
     .num_tasks = 0,
     .task_list = {},
     .get_next_state = &init_get_next_state};
