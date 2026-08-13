#include "init_state.h"
#include "device_status.h"
#include "flash.h"
#include "logger.h"
#include "neopixel.h"
#include "slate.h"

state_id_t init_get_next_state(slate_t *slate)
{
    // If shutdown was active before a reboot, re-enter shutdown immediately.
    // This keeps the communication blackout persistent across reboots and
    // skips normal init / burn wire, which must not run again.
    if (get_shutdown_active())
    {
        LOG_INFO("Persistent shutdown flag set. Re-entering shutdown state.");
        slate->shutdown_triggered = true;
        return STATE_SHUTDOWN;
    }

#ifdef BRINGUP
    return STATE_BRINGUP;
#elif defined(PICO)
    return STATE_RUNNING;
#else
    // Check if RBF pin is detected
    if (is_rbf_pin_detected())
    {
        // If RBF pin is detected, block and stay in init state
        LOG_INFO("RBF pin detected, staying in init state.");
        neopixel_set_color_rgb(0xff, 0, 0);
        return STATE_INIT;
    }
#ifdef FLIGHT
    return STATE_BURN_WIRE;
#else
    return STATE_RUNNING;
#endif
#endif
}

sched_state_t init_state = {.name = "init",
                            .id = STATE_INIT,
                            .num_tasks = 0,
                            .task_list = {},
                            .get_next_state = &init_get_next_state};
