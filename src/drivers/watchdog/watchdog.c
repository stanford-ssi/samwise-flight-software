#include "watchdog.h"

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
    gpio_init(wd->pin);
    gpio_set_dir(wd->pin, GPIO_OUT);

    gpio_put(wd->pin, 0);
    wd->last_transition = get_absolute_time();
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
    uint64_t delta =
        absolute_time_diff_us(get_absolute_time(), wd->last_transition);
    if (wd->set && delta > wd->us_high)
    {
        gpio_put(wd->pin, 0);
        wd->set = false;
        wd->last_transition = get_absolute_time();
    }
    else if (!wd->set && delta > wd->us_low)
    {
        gpio_put(wd->pin, 1);
        wd->set = true;
        wd->last_transition = get_absolute_time();
    }
}
