#pragma once

#include <string.h>

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
