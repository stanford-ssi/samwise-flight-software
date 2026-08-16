
/**
 * @author  Yao Yiheng
 * @date    2025-07-06
 */

#include "burn_wire_task.h"
#include "device_status.h"
#include "flash.h"
#include "logger.h"
#include "neopixel.h"
#include "safe_sleep.h"

void burn_wire_task_init(slate_t *slate)
{
    LOG_INFO("Burn wire task is initializing...");
}

void burn_wire_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(BURN_WIRE_TASK_COLOR);

    // Only burn the sides which have not released yet. This also covers
    // rebooting into this state on an already-deployed satellite.
    bool panel_A_deployed = is_flex_panel_A_deployed();
    bool panel_B_deployed = is_flex_panel_B_deployed();
    if (panel_A_deployed && panel_B_deployed)
    {
        LOG_INFO("Both panels are deployed. Not activating burn wire.");
        neopixel_set_color_rgb(0, 0, 0);
        return;
    }

    // Check if burn wire is already activated
    uint32_t burn_wire_attempts = get_burn_wire_attempts();
    LOG_INFO("Burn wire task is dispatching... %d", burn_wire_attempts);
    if (burn_wire_attempts >= MAX_BURN_WIRE_ATTEMPTS)
    {
        LOG_ERROR(
            "Maximum burn wire attempts reached. Not activating burn wire.");
        neopixel_set_color_rgb(0, 0, 0);
        return;
    }
    safe_sleep_ms(1000); // Sleep for 1 second
    // Activate burn wire for a maximum duration
    // of MAX_BURN_DURATION_MS milliseconds at max power.
    // Activate A and B channels one after another.
    if (!panel_A_deployed)
    {
        burn_wire_activate(slate, BURN_DURATION_MS, true,
                           false); // Activate A channel
    }
    if (!panel_B_deployed)
    {
        burn_wire_activate(slate, BURN_DURATION_MS, false,
                           true); // Activate B channel
    }
    // Give the panels time to swing clear before the detect pins are sampled
    // by burn_wire_get_next_state.
    safe_sleep_ms(4000);
    increment_burn_wire_attempts();
    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t burn_wire_task = {.name = "burn_wire",
                               .dispatch_period_ms = 0, // Dispatch immediately
                               .task_init = &burn_wire_task_init,
                               .task_dispatch = &burn_wire_task_dispatch,

                               /* Set to an actual value on init */
                               .next_dispatch = 0};
