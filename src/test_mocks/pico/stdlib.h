#pragma once

#include <stdbool.h>
#include <stdint.h>

// Mock pico stdlib types and functions
typedef uint64_t absolute_time_t;

// Global simulated time for testing (in microseconds)
extern uint64_t mock_time_us;

// Mock time functions
static inline absolute_time_t get_absolute_time(void)
{
    return mock_time_us;
}
static inline uint64_t absolute_time_diff_us(absolute_time_t from,
                                             absolute_time_t to)
{
    if (to >= from)
    {
        return to - from;
    }
    else
    {
        return 0;
    }
}
static inline bool time_reached(absolute_time_t t)
{
    return mock_time_us >= t;
}
static inline absolute_time_t make_timeout_time_ms(uint32_t ms)
{
    return mock_time_us + (ms * 1000ULL);
}
static inline absolute_time_t delayed_by_ms(absolute_time_t t, uint32_t ms)
{
    return t + (ms * 1000ULL);
}
static inline absolute_time_t delayed_by_us(absolute_time_t t, uint64_t us)
{
    return t + us;
}
static inline uint32_t to_ms_since_boot(absolute_time_t t)
{
    return (uint32_t)(t / 1000ULL);
}

// Mock sleep functions - advance simulated time
static inline void sleep_ms(uint32_t ms)
{
    mock_time_us += ms * 1000ULL;
}
static inline void sleep_us(uint64_t us)
{
    mock_time_us += us;
}
