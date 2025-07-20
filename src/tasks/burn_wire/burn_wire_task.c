
/**
 * @author  Yao Yiheng
 * @date    2025-07-06
 */

#include "burn_wire_task.h"
#include "neopixel.h"
#include "safe_sleep.h"

static uint32_t count = 0;

void burn_wire_task_init(slate_t *slate)
{
    LOG_INFO("Test task is initializing...");
}

void burn_wire_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(0xf, 0xf, 0xf);
    // Stop dispatching after 5 attempts
    if (count >= 5)
    {
        LOG_INFO("Burn wire has completed its execution. 5 Attempts made.");
        return;
    }
    LOG_INFO("Burn wire task is dispatching... %d", count);
    count++;
    safe_sleep_ms(1000); // Sleep for 1 second
    // Activate burn wire for a maximum duration
    // of MAX_BURN_DURATION_MS milliseconds at max power.
    // Activate A and B channels one after another.
    burn_wire_activate(slate, 800, true,
                       false); // Activate A channel
    burn_wire_activate(slate, 800, false,
                       true); // Activate B channel
    safe_sleep_ms(4000);
    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t burn_wire_task = {.name = "burn_wire",
                               .dispatch_period_ms = 0, // Dispatch immediately
                               .task_init = &burn_wire_task_init,
                               .task_dispatch = &burn_wire_task_dispatch,

                               /* Set to an actual value on init */
                               .next_dispatch = 0};
