/**
 * @author  Niklas Vainio
 * @date    2024-08-27
 *
 * Task to blink the onboard LED.
 */

#include "state_machine/tasks/blink_task.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "slate.h"

void blink_task_init(slate_t *slate)
{
    slate->led_state = false;
}

void blink_task_dispatch(slate_t *slate)
{
    slate->led_state = !slate->led_state;
    gpio_put(PICO_DEFAULT_LED_PIN, slate->led_state);
}

sched_task_t blink_task = {.name = "blink",
                           .dispatch_period_ms = 1000,
                           .task_init = &blink_task_init,
                           .task_dispatch = &blink_task_dispatch,

                           /* Set to an actual value on init */
                           .next_dispatch = 0};
