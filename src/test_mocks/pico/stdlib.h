#pragma once

#include <stdbool.h>
#include <stdint.h>

// Mock pico stdlib types and functions
typedef uint64_t absolute_time_t;

// Mock time functions
static inline absolute_time_t get_absolute_time(void)
{
    return 0;
}
static inline uint64_t absolute_time_diff_us(absolute_time_t from,
                                             absolute_time_t to)
{
    return 0;
}
static inline bool time_reached(absolute_time_t t)
{
    return false;
}
static inline absolute_time_t make_timeout_time_ms(uint32_t ms)
{
    return ms;
}
static inline absolute_time_t delayed_by_ms(absolute_time_t t, uint32_t ms)
{
    return t + ms;
}
static inline absolute_time_t delayed_by_us(absolute_time_t t, uint64_t us)
{
    return t + us;
}
static inline uint32_t to_ms_since_boot(absolute_time_t t)
{
    return (uint32_t)t;
}

// Mock sleep functions
static inline void sleep_ms(uint32_t ms)
{
}
static inline void sleep_us(uint64_t us)
{
}
