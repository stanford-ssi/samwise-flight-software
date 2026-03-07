#include "shutdown_state.h"
#include "logger.h"
#include "neopixel.h"

#define SHUTDOWN_STATE_COLOR 75, 0, 130 // Indigo: shutdown indicator

state_id_t shutdown_get_next_state(slate_t *slate)
{
    (void)slate;
    neopixel_set_color_rgb(SHUTDOWN_STATE_COLOR);
    return STATE_SHUTDOWN;
}

sched_state_t shutdown_state = {
    .name = "shutdown",
    .id = STATE_SHUTDOWN,
    .num_tasks = 1,
    .task_list = {&watchdog_task},
    .get_next_state = &shutdown_get_next_state};
