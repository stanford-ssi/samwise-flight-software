#include "shutdown_state.h"
#include "flash.h"
#include "logger.h"
#include "neopixel.h"

#define SHUTDOWN_STATE_COLOR 75, 0, 130 // Indigo: shutdown indicator

// Fallback auto-exit timeout. Ground reactivation (shutdown_listen_task) is
// the intended way out of shutdown; this timer only exists as a safety net in
// case ground contact is impossible. Its exact value is not important. 3
// months: 90 days * 24h * 60m * 60s * 1000ms.
// NOTE: time_in_current_state_ms resets on reboot, so this is a per-boot
// timer, not a persistent 3-month deadline.
#define SHUTDOWN_TIMEOUT_MS (90ULL * 24 * 60 * 60 * 1000)

state_id_t shutdown_get_next_state(slate_t *slate)
{
    neopixel_set_color_rgb(SHUTDOWN_STATE_COLOR);

    // Fallback safety net: auto-reactivate after the timeout even without a
    // ground command.
    if (slate->shutdown_triggered &&
        slate->time_in_current_state_ms >= SHUTDOWN_TIMEOUT_MS)
    {
        LOG_INFO("Shutdown fallback timeout reached. Reactivating.");
        clear_shutdown_active();
        slate->shutdown_triggered = false;
        slate->shutdown_cmd_counter = 0;
        slate->shutdown_reactivate_counter = 0;
    }

    // shutdown_listen_task clears shutdown_triggered on ground reactivation.
    // Exit to STATE_RUNNING (not STATE_INIT): initialization and burn wire have
    // already happened, so re-running them would be wrong.
    if (!slate->shutdown_triggered)
    {
        return STATE_RUNNING;
    }

    return STATE_SHUTDOWN;
}

sched_state_t shutdown_state = {
    .name = "shutdown",
    .id = STATE_SHUTDOWN,
    .num_tasks = 2,
    .task_list = {&watchdog_task, &shutdown_listen_task},
    .get_next_state = &shutdown_get_next_state};
