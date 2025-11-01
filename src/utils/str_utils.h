#pragma once

#include <string.h>

/**
 * Replacement for strlcpy that works for static-sized buffers (dst) that
 * don't care about truncation and just want to ensure null-termination.
 * Adapted from https://nrk.neocities.org/articles/not-a-fan-of-strlcpy.
 *
 * @param dst Destination buffer.
 * @param src Source string.
 * @param size Size of the source buffer.
 */
void strcpy_trunc(char *dst, const char *src, size_t size)
{
    if (memccpy(dst, src, '\0', size) == NULL)
        dst[size - 1] = '\0'; /* truncation occured, null-terminate manually. */
}
