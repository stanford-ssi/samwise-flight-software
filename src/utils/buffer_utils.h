/**
 * Author: Ayush Garg
 *
 * @brief Utilities for encoding/decoding structured data to/from flat byte
 * buffers.
 *
 * These helpers serialize and deserialize fields sequentially into a uint8_t
 * buffer, avoiding the need for __attribute__((packed)) structs. All sizes are
 * resolved at compile time — each ARG/RAW macro expands to a direct memcpy,
 * with no varargs loop at runtime.
 *
 * Encoding (struct fields -> byte buffer):
 *   uint8_t buf[64];
 *   size_t len = buffer_encode_data(buf,
 *       BUFFER_ENCODE_ARG(my_uint16),       // scalar: copied by value
 *       BUFFER_ENCODE_ARG(my_uint32),
 *       BUFFER_ENCODE_RAW(N, byte_array));   // array: copied by pointer
 *
 * Decoding (byte buffer -> struct fields):
 *   uint16_t my_uint16;
 *   uint32_t my_uint32;
 *   uint8_t  byte_array[N];
 *   buffer_decode_data(buf,
 *       BUFFER_DECODE_ARG(my_uint16),
 *       BUFFER_DECODE_ARG(my_uint32),
 *       BUFFER_DECODE_RAW(N, byte_array));
 */

#pragma once

#include <stdint.h>
#include <string.h>

/* ===== Encode: write fields into a byte buffer ===== */

/**
 * Encodes a scalar value. Creates a compound literal so the value's address
 * can be passed to memcpy.
 */
#define BUFFER_ENCODE_ARG(x)                                                   \
    (memcpy(_enc_buf + _enc_off, &(__typeof__(x)){(x)}, sizeof(x)),            \
     _enc_off += sizeof(x))

/**
 * Encodes a raw byte array of known size (e.g. uint8_t[], tracker bitfields).
 */
#define BUFFER_ENCODE_RAW(size, arr)                                           \
    (memcpy(_enc_buf + _enc_off, (arr), (size)), _enc_off += (size))

/**
 * Sequentially encodes fields into a byte buffer.
 *
 * @param out  Destination buffer (must be large enough for all fields).
 * @param ...  One or more BUFFER_ENCODE_ARG / BUFFER_ENCODE_RAW expressions.
 * @return     Total number of bytes written.
 */
#define buffer_encode_data(out, ...)                                           \
    ({                                                                         \
        uint8_t *_enc_buf = (out);                                             \
        size_t _enc_off = 0;                                                   \
        __VA_ARGS__;                                                           \
        _enc_off;                                                              \
    })

/* ===== Decode: read fields from a byte buffer ===== */

/**
 * Decodes a scalar value from the buffer into a variable.
 */
#define BUFFER_DECODE_ARG(x)                                                   \
    (memcpy(&(x), _dec_buf + _dec_off, sizeof(x)), _dec_off += sizeof(x))

/**
 * Decodes a raw byte array of known size from the buffer.
 */
#define BUFFER_DECODE_RAW(size, arr)                                           \
    (memcpy((arr), _dec_buf + _dec_off, (size)), _dec_off += (size))

/**
 * Sequentially decodes fields from a byte buffer.
 *
 * @param in   Source buffer to read from.
 * @param ...  One or more BUFFER_DECODE_ARG / BUFFER_DECODE_RAW expressions.
 * @return     Total number of bytes read.
 */
#define buffer_decode_data(in, ...)                                            \
    ({                                                                         \
        const uint8_t *_dec_buf = (in);                                        \
        size_t _dec_off = 0;                                                   \
        __VA_ARGS__;                                                           \
        _dec_off;                                                              \
    })
