#include "running_state.h"
#include "neopixel.h"

sched_state_t *running_get_next_state(slate_t *slate)
{
    neopixel_set_color_rgb(0, 0xf, 0xf);
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
    .num_tasks = 4,
    .task_list = {&print_task, &blink_task, &watchdog_task, &payload_task},
    .get_next_state = &running_get_next_state};
#endif
