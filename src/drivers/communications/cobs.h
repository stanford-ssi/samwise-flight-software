#pragma once
/* Author:      @Carson Lauer
 * Date:        April 26, 2026
 * Description: COBS encoding library.
 * This includes utilities to encode messages as COBS zero-terminated
 * sequences.
 *
 * Code copied straight from here:
 * (https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing)
 */
#include <stdint.h>

uint32_t cobs_encode(const void *data, uint32_t length, uint8_t *buffer);

uint32_t cobs_decode(const uint8_t *buffer, uint32_t length, void *data);
