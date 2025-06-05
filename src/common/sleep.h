#pragma once

#include <inttypes.h>
#include <slate.h>

static void safe_sleep(uint32_t ms)
{
    const uint32_t safe_sleep_interval_ms = 10000; // 10 sec
    uint32_t remaining_time = ms;
    watchdog_feed(&slate->watchdog);

    while (remaining_time > 0)
    {
        watchdog_feed(&slate->watchdog);
        uint32_t time_to_sleep = remaining_time > safe_sleep_interval_ms
                                     ? safe_sleep_interval_ms
                                     : remaining_time;

        remaining_time -= time_to_sleep;
        sleep_ms(time_to_sleep);
    }
}