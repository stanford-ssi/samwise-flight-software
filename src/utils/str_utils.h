#pragma once

#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Calculate the length of data that would be written by snprintf,
 *       ensuring it does not exceed buffer size.
 *
 * @param dest Pointer to the destination buffer.
 * @param data_size Size of the destination buffer.
 * @param fmt Format string.
 * @param ... Additional arguments for the format string.
 * @return Length of data that would be written, including null terminator,
 *         or negative value on encoding error.
 */
inline static int snprintf_len(char *dest, size_t data_size, const char *fmt,
                               ...)
{
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(dest, data_size, fmt, args);
    va_end(args);

    if (n < 0)
    {
        // encoding error, send nothing
        LOG_ERROR("The snprintf function returned an encoding error.");
        return n;
    }
    else if (n < (int)data_size)
    {
        // snprintf fit the buffer; n is number of bytes written excluding NUL
        // include the trailing NUL in the length
        return n + 1;
    }
    else
    {
        // output was truncated; buffer contains (data_size-1) bytes + NUL
        // include that NUL in the length
        return (int)data_size;
    }
}

/**
 * Replacement for strlcpy that works for static-sized buffers (dst) that
 * don't care about truncation and just want to ensure null-termination.
 * Adapted from https://nrk.neocities.org/articles/not-a-fan-of-strlcpy.
 *
 * @param dst Destination buffer.
 * @param src Source string.
 * @param size Maximum size of the destination buffer.
 */
inline static void strcpy_trunc(char *dst, const char *src, size_t max_size)
{
    if (memccpy(dst, src, '\0', max_size) == NULL)
        dst[max_size - 1] =
            '\0'; /* truncation occured, null-terminate manually. */
}
