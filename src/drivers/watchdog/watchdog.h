#pragma once

#include "pins.h"
#include "hardware/gpio.h"

struct watchdog {
  uint pin;
  absolute_time_t last_transition;
  bool set;
  uint64_t us_low;
  uint64_t us_high;
};

struct watchdog watchdog_mk();
void watchdog_init(struct watchdog* wd);
void watchdog_feed(struct watchdog* wd);
