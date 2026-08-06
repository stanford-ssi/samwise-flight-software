#pragma once

#ifndef TEST
#include "config.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "pico/time.h"
#include "pins.h"
#else
#include "typedefs.h"
#include <stdbool.h>
#endif

struct watchdog
{
    uint pin;
    absolute_time_t last_transition;
    bool set;
    uint64_t us_low;
    uint64_t us_high;
    bool is_initialized;
};

struct watchdog watchdog_mk();
void watchdog_init(struct watchdog *wd);
void watchdog_feed(struct watchdog *wd);
