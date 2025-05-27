#include "running_state.h"

sched_state_t *running_get_next_state(slate_t *slate)
{
    return &running_state;
}

#ifdef BRINGUP
// Diagnostics task is only included in the bringup build
sched_state_t running_state = {
    .name = "running",
    .num_tasks = 3,
    .task_list = {&print_task, &watchdog_task, &diagnostics_task},
    .get_next_state = &running_get_next_state};
#else
sched_state_t running_state = {
    .name = "running",
    .num_tasks = 8,
    .task_list = {&print_task, &watchdog_task, &blink_task, &telemetry_task,
                  &beacon_task, &radio_task, &command_task, &payload_task},
    .get_next_state = &running_get_next_state};
#endif
