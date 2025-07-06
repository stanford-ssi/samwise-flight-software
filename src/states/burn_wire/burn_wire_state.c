#include "burn_wire_state.h"

extern sched_state_t burn_wire_state;

sched_state_t *burn_wire_get_next_state(slate_t *slate)
{
    return &running_state;
}

sched_state_t burn_wire_state = {.name = "burn_wire",
                                 .num_tasks = 1,
                                 .task_list = {&burn_wire_task},
                                 .get_next_state = &burn_wire_get_next_state};
