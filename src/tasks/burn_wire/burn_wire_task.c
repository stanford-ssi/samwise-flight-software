
/**
 * @author  Yao Yiheng
 * @date    2025-07-06
 */

#include "burn_wire_task.h"
#include "flash.h"
#include "safe_sleep.h"

void burn_wire_task_init(slate_t *slate)
{
    LOG_INFO("Burn wire task is initializing...");
}

void burn_wire_task_dispatch(slate_t *slate)
{
    // Check if burn wire is already activated
    uint32_t burn_wire_attempts = get_burn_wire_attempts();
    LOG_INFO("Burn wire task is dispatching... %d", burn_wire_attempts);
    if (burn_wire_attempts >= MAX_BURN_WIRE_ATTEMPTS)
    {
        LOG_ERROR("Maximum burn wire attempts reached. Not activating burn wire.");
        return;
    }
    // Activate burn wire for 100ms at 50% duty cycle
    // Duty cycle is 31 out of 63 for 5-bit PWM
    // The maximum value of 63 is set as the WRAP for the PWM slice here
    // src/drivers/burn_wire/burn_wire.c
    burn_wire_activate(slate, MAX_BURN_DURATION_MS, 31, true, true);
    safe_sleep_ms(
        5000); // Sleep for 5 seconds to simulate burn wire task execution
    increment_burn_wire_attempts();
}

sched_task_t burn_wire_task = {.name = "burn_wire",
                               .dispatch_period_ms = 0, // Dispatch immediately
                               .task_init = &burn_wire_task_init,
                               .task_dispatch = &burn_wire_task_dispatch,

                               /* Set to an actual value on init */
                               .next_dispatch = 0};
