#include "state_machine/states/running.h"
#include "state_machine/states/states.h"
#include "state_machine/tasks/tasks.h"

sched_state_t running_state = {.name = "running",
                               .num_tasks = 3,
                               .task_list = {&print_task, &blink_task, &radio_task},
                               .get_next_state = &running_get_next_state};

sched_state_t *running_get_next_state(slate_t *slate)
{
    return &running_state;
}
