#pragma once

#include <stdint.h>

// Mock PWM types and functions
typedef unsigned int uint;

static inline void pwm_set_gpio_level(uint gpio, uint16_t level) {}
static inline void pwm_set_wrap(uint slice_num, uint16_t wrap) {}
static inline void pwm_set_chan_level(uint slice_num, uint chan, uint16_t level) {}
static inline void pwm_set_enabled(uint slice_num, bool enabled) {}
static inline uint pwm_gpio_to_slice_num(uint gpio) { return 0; }
static inline uint pwm_gpio_to_channel(uint gpio) { return 0; }
