/**
 * @author  Niklas Vainio
 * @date    2024-08-27
 *
 * Task to blink the onboard LED.
 */

#include "blink_task.h"

void blink_task_init(slate_t *slate)
{
}

void blink_task_dispatch(slate_t *slate)
{
    onboard_led_toggle(&slate->onboard_led);
}

sched_task_t blink_task = {.name = "blink",
                           .dispatch_period_ms = 1000,
                           .task_init = &blink_task_init,
                           .task_dispatch = &blink_task_dispatch,

                           /* Set to an actual value on init */
                           .next_dispatch = 0};
