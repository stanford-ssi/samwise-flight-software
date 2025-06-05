#pragma once

#include "common/config.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pins.h"

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
