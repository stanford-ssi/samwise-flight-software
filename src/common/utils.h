#pragma once

#include <inttypes.h>

static uint16_t safe_mult(uint16_t val, int m)
{
    if (m < 0)
        return 0;
    // First compute the maximum value which can be multiplied
    // by m and fit within uint16_t.
    // If value is larger than the above, we will overflow, so
    // just return the maximum uint16_t value instead.
    if (m != 0 && val > (uint16_t)(val / m))
        return 0xFFFF;
    return val * m;
}
