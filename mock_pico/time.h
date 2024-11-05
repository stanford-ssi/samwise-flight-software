#ifndef MOCK_PICO_TIME_H
#define MOCK_PICO_TIME_H

#include <stdint.h>
#include "types.h"

// Time functions from pico SDK that need to be mocked
absolute_time_t get_absolute_time(void);
void sleep_ms(uint32_t ms);
void sleep_us(uint64_t us);

// Helper functions for time calculations
static inline absolute_time_t delayed_by_ms(absolute_time_t t, uint32_t ms) {
    absolute_time_t result;
    update_us_since_boot(&result, to_us_since_boot(t) + ms * 1000ull);
    return result;
}

static inline absolute_time_t make_timeout_time_ms(uint32_t ms) {
    return delayed_by_ms(get_absolute_time(), ms);
}

static inline int64_t absolute_time_diff_us(absolute_time_t from, absolute_time_t to) {
    return (int64_t)(to_us_since_boot(to) - to_us_since_boot(from));
}

// Helper functions for converting between time formats
static inline uint64_t to_us_since_boot(absolute_time_t t) {
#ifdef PICO_DEBUG_ABSOLUTE_TIME_T
    return t._private_us_since_boot;
#else
    return t;
#endif
}

static inline void update_us_since_boot(absolute_time_t *t, uint64_t us_since_boot) {
#ifdef PICO_DEBUG_ABSOLUTE_TIME_T
    t->_private_us_since_boot = us_since_boot;
#else
    *t = us_since_boot;
#endif
}

#endif // MOCK_PICO_TIME_H