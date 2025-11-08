#pragma once

#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/**
 * Converts a uint16_t to a string, where each byte is represented
 * as a character in the string.
 *
 * @param value The uint16_t value to convert.
 * @return A pointer to the string representation of the value.
 */
inline static void toString(const uint16_t value, char *buffer)
{
    buffer[0] = (value >> 8) & 0xFF;
    buffer[1] = value & 0xFF;
    buffer[2] = '\0';
}
