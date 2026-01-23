/**
 * @author Ayush Garg
 * @date  2026-01-20
 */

#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief Copy memory from source to destination and increment the destination
 *        pointer. Makes it easier to append data into a buffer.
 *
 * @param ptr Pointer to the destination pointer which will be incremented.
 * @param src Source memory to copy from.
 * @param size Number of bytes to copy.
 */
inline static void memcpy_inc(uint8_t **ptr, const void *src, size_t size)
{
    memcpy(*ptr, src, size);
    *ptr += size;
}
