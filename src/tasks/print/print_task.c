
/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 */

#include "print_task.h"

#include "hardware/gpio.h"
#include "pins.h"

static uint32_t count = 0;

void print_task_init(slate_t *slate)
{
    LOG_INFO("Test task is initializing...");
}

void print_task_dispatch(slate_t *slate)
{
    LOG_INFO("Test task is dispatching... %d", count);
    count++;
    gpio_put(SAMWISE_WATCHDOG_FEED_PIN, 1);
    sleep_ms(200);
    gpio_put(SAMWISE_WATCHDOG_FEED_PIN, 0);
}

sched_task_t print_task = {.name = "print",
                           .dispatch_period_ms = 1000,
                           .task_init = &print_task_init,
                           .task_dispatch = &print_task_dispatch,

                           /* Set to an actual value on init */
                           .next_dispatch = 0};
