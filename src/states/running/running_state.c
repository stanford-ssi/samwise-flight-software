#include "running_state.h"
#include "neopixel.h"

state_id_t running_get_next_state(slate_t *slate)
{
    if (slate->shutdown_triggered)
    {
        return STATE_SHUTDOWN;
    }
    return STATE_RUNNING;
}

#ifdef BRINGUP
// Diagnostics task is only included in the bringup build
sched_state_t running_state = {.name = "running",
                               .id = STATE_RUNNING,
                               .num_tasks = 4,
                               .task_list = {&print_task, &watchdog_task,
                                             &diagnostics_task,
                                             &hardware_test_task},
                               .get_next_state = &running_get_next_state};
#elif defined(PICOHAT)
sched_state_t running_state = {.name = "running",
                               .id = STATE_RUNNING,
                               .num_tasks = 6,
                               .task_list = {&print_task, &blink_task,
                                             &beacon_task, &radio_task,
                                             &command_task, &watchdog_task},
                               .get_next_state = &running_get_next_state};
#elif defined(PICO)
sched_state_t running_state = {
    .name = "running",
    .id = STATE_RUNNING,
    .num_tasks = 3,
    .task_list = {&print_task, &blink_task, &hardware_test_task},
    .get_next_state = &running_get_next_state};
#else
sched_state_t running_state = {
    .name = "running",
    .id = STATE_RUNNING,
    .num_tasks = 8,
    .task_list = {&print_task, &watchdog_task, &beacon_task, &adcs_task,
                  &payload_task, &telemetry_task, &radio_task, &command_task},
    .get_next_state = &running_get_next_state};
#endif
