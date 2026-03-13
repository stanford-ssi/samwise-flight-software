#include "shutdown_state.h"
#include "logger.h"
#include "neopixel.h"

#define SHUTDOWN_STATE_COLOR 75, 0, 130 // Indigo: shutdown indicator

// 3 months in milliseconds: 90 days * 24h * 60m * 60s * 1000ms
#define SHUTDOWN_TIMEOUT_MS (90ULL * 24 * 60 * 60 * 1000)

state_id_t shutdown_get_next_state(slate_t *slate)
{
    neopixel_set_color_rgb(SHUTDOWN_STATE_COLOR);

    if (slate->time_in_current_state_ms >= SHUTDOWN_TIMEOUT_MS)
    {
        LOG_INFO("Shutdown timeout reached (3 months). Reinitializing.");
        slate->shutdown_triggered = false;
        slate->shutdown_cmd_counter = 0;
        return STATE_INIT;
    }

    return STATE_SHUTDOWN;
}

sched_state_t shutdown_state = {.name = "shutdown",
                                .id = STATE_SHUTDOWN,
                                .num_tasks = 1,
                                .task_list = {&watchdog_task},
                                .get_next_state = &shutdown_get_next_state};
