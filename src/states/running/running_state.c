#include "running_state.h"

sched_state_t *running_get_next_state(slate_t *slate)
{
    return &running_state;
}

sched_state_t running_state = {.name = "running",
                               .num_tasks = 7,
                               .task_list = {&print_task, &blink_task,
                                             &radio_task, &command_task,
                                             &beacon_task, &watchdog_task,
                                            &payload_task},
                               .get_next_state = &running_get_next_state};
