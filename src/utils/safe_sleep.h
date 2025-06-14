#pragma once

#include "common/config.h"
#include "drivers/watchdog/watchdog.h"
#include "pico/time.h"
#include <slate.h>
#include <stdint.h>

extern slate_t slate;

static void safe_sleep_ms(uint32_t ms)
{
    uint32_t remaining_time = ms;

    while (remaining_time > MIN_WATCHDOG_INTERVAL_MS)
    {
        watchdog_feed(&slate.watchdog);
        // This together with the while loop condition ensures
        // there will be no integer overflow.
        remaining_time -= MIN_WATCHDOG_INTERVAL_MS;
        sleep_ms(MIN_WATCHDOG_INTERVAL_MS);
    }
    sleep_ms(remaining_time);
}
