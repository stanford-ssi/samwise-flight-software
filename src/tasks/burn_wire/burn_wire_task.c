
/**
 * @author  Yao Yiheng
 * @date    2025-07-06
 */

#include "burn_wire_task.h"
#include "neopixel.h"

static uint32_t count = 0;

void burn_wire_task_init(slate_t *slate)
{
    LOG_INFO("Test task is initializing...");
}

void burn_wire_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(BURN_WIRE_TASK_COLOR);
    LOG_INFO("Test task is dispatching... %d", count);
    count++;
    // Activate burn wire for 100ms at 50% duty cycle
    // 0 -> off
    // 1 -> 25% duty cycle
    // 2 -> 50% duty cycle
    // 3 -> 75% duty cycle
    // 4 -> 100% duty cycle
    burn_wire_activate(slate, MAX_BURN_DURATION_MS, 2, true, true);
    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t burn_wire_task = {.name = "burn_wire",
                               .dispatch_period_ms = 0, // Dispatch immediately
                               .task_init = &burn_wire_task_init,
                               .task_dispatch = &burn_wire_task_dispatch,

                               /* Set to an actual value on init */
                               .next_dispatch = 0};
