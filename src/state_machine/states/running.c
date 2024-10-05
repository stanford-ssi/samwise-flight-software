#include "state_machine/states/running.h"

sm_state_t running_get_next_state(slate_t *slate)
{
    return state_running;
}

sched_state_info_t running_state_info = {.name = "running",
     .num_tasks = 2,
     .task_list = {&print_task, &blink_task},
     .get_next_state = &running_get_next_state}};
