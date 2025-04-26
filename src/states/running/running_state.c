#include "running_state.h"

sched_state_t *running_get_next_state(slate_t *slate)
{
    return &running_state;
}

sched_state_t running_state = {.name = "running",
                               .num_tasks = 1,
                               .task_list = {
                                             &radio_task},
                               .get_next_state = &running_get_next_state};
