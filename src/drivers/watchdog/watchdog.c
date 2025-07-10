#include "watchdog.h"
#include "hal_interface.h"

struct watchdog watchdog_mk()
{
    return (struct watchdog){
        .pin = SAMWISE_WATCHDOG_FEED_PIN,
        .last_transition = nil_time,
        .set = false,
        .us_low = 1000 * 1000,                      // 5 second
        .us_high = MIN_WATCHDOG_INTERVAL_MS * 1000, // 200ms
        .is_initialized = false,
    };
}

void watchdog_init(struct watchdog *wd)
{
    hal.gpio_init(wd->pin);
    hal.gpio_set_dir(wd->pin, HAL_GPIO_OUT);

    hal.gpio_put(wd->pin, 0);
    wd->last_transition = hal.get_absolute_time_us();
    wd->set = false;
    wd->is_initialized = true;
}

void watchdog_feed(struct watchdog *wd)
{
    // Ensure watchdog GPIO pins are initialized before feeding
    if (!wd->is_initialized)
    {
        watchdog_init(wd);
    }
    uint64_t current_time = hal.get_absolute_time_us();
    uint64_t delta =
        hal.absolute_time_diff_us(wd->last_transition, current_time);
    if (wd->set && delta > wd->us_high)
    {
        hal.gpio_put(wd->pin, 0);
        wd->set = false;
        wd->last_transition = current_time;
    }
    else if (!wd->set && delta > wd->us_low)
    {
        hal.gpio_put(wd->pin, 1);
        wd->set = true;
        wd->last_transition = current_time;
    }
}
