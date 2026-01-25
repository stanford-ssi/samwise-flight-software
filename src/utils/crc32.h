#pragma once

#include <stdint.h>

// From https://gist.github.com/xobs/91a84d29152161e973d717b9be84c4d0
// (not using fast version because we want small binary size)
// Adapted such that original CRC can be passed in to continue calculation
// WARNING: Note this does NOT invert after calculation, this must be done by
// the user! To use, initialize crc to 0xFFFFFFFF, then call crc32_continue as
// needed, then invert the final result (use the ~ operator).
inline static unsigned int crc32_continue(const uint8_t *message, uint16_t len,
                                          unsigned int crc)
{
    size_t i;
    unsigned int byte, mask;

    i = 0;
    while (i < len)
    {
        byte = message[i]; // Get next byte.
        crc = crc ^ byte;
        for (int j = 0; j < 8; j++)
        { // Do eight times.
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        i = i + 1;
    }

    return crc; // Don't invert here - must be done by the user!
}

inline static unsigned int crc32(const uint8_t *message, uint16_t len)
{
    return ~crc32_continue(message, len, 0xFFFFFFFF);
}
