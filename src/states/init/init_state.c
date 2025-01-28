#include "init_state.h"

sched_state_t *init_get_next_state(slate_t *slate)
{
    return &running_state;
}

sched_state_t init_state = {.name = "init",
                            .num_tasks = 0,
                            .task_list = {},
                            .get_next_state = &init_get_next_state};
