/**
 * @author  Niklas Vainio
 * @date    2024-08-27
 *
 * Task to blink the onboard LED.
 */

#include "blink_task.h"
#include "neopixel.h"

void blink_task_init(slate_t *slate)
{
    slate->neopixel_is_on = false;
}

void blink_task_dispatch(slate_t *slate)
{
#ifdef PICO
    onboard_led_toggle(&slate->onboard_led);
#else
    neopixel_set_color_rgb(0, slate->neopixel_is_on ? 0xff : 0, 0,
                           SAMWISE_NEOPIXEL_PIN);
    slate->neopixel_is_on = !slate->neopixel_is_on;
#endif
}

sched_task_t blink_task = {.name = "blink",
                           .dispatch_period_ms = 1000,
                           .task_init = &blink_task_init,
                           .task_dispatch = &blink_task_dispatch,

                           /* Set to an actual value on init */
                           .next_dispatch = 0};
