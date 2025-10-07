#pragma once

#include <stdint.h>
#include <stdbool.h>

// Mock pico time types - use from stdlib.h
#include "pico/stdlib.h"

// Additional time functions
#define PICO_TIME_DEFAULT_ALARM_POOL_HARDWARE_ALARM_NUM 3

typedef void (*alarm_callback_t)(uint alarm_num);

static inline bool alarm_pool_add_alarm_at(void *pool, absolute_time_t time,
                                           alarm_callback_t callback, void *user_data,
                                           bool fire_if_past) {
    return true;
}
