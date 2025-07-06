
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
    // Activate burn wire for 100ms at 50% duty cycle
    // Duty cycle is 31 out of 63 for 5-bit PWM
    // The maximum value of 63 is set as the WRAP for the PWM slice here
    // src/drivers/burn_wire/burn_wire.c
    burn_wire_activate(slate, MAX_BURN_DURATION_MS, 31, true, true);
    safe_sleep_ms(
        30000); // Sleep for 30 seconds to simulate wait before retrying
    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t burn_wire_task = {.name = "burn_wire",
                               .dispatch_period_ms = 0, // Dispatch immediately
                               .task_init = &burn_wire_task_init,
                               .task_dispatch = &burn_wire_task_dispatch,

                               /* Set to an actual value on init */
                               .next_dispatch = 0};
