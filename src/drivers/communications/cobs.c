/* Author:      @Carson Lauer
 * Date:        April 26, 2026
 * Description: COBS encoding library.
 * This includes utilities to encode messages as COBS zero-terminated
 * sequences.
 *
 * Code copied straight from here:
 * (https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing)
 */
#include "macros.h"
#include <stdint.h>

#include "cobs.h"

/** COBS encode data to buffer
        @param data Pointer to input data to encode
        @param length Number of bytes to encode
        @param buffer Pointer to encoded output buffer
        @return Encoded buffer length in bytes
        @note Does not output delimiter byte
*/
uint32_t cobs_encode(const void *data, uint32_t length, uint8_t *buffer)
{
    ASSERT(data && buffer);

    uint8_t *encode = buffer;  // Encoded byte pointer
    uint8_t *codep = encode++; // Output code pointer
    uint8_t code = 1;          // Code value

    for (const uint8_t *byte = (const uint8_t *)data; length--; ++byte)
    {
        if (*byte) // Byte not zero, write it
            *encode++ = *byte, ++code;

        if (!*byte || code == 0xff) // Input is zero or block completed, restart
        {
            *codep = code, code = 1, codep = encode;
            if (!*byte || length)
                ++encode;
        }
    }
    *codep = code; // Write final code value

    return (uint32_t)(encode - buffer);
}

/** COBS decode data from buffer
        @param buffer Pointer to encoded input bytes
        @param length Number of bytes to decode
        @param data Pointer to decoded output data
        @return Number of bytes successfully decoded
        @note Stops decoding if delimiter byte is found
*/
uint32_t cobs_decode(const uint8_t *buffer, uint32_t length, void *data)
{
    ASSERT(buffer && data);

    const uint8_t *byte = buffer;      // Encoded input byte pointer
    uint8_t *decode = (uint8_t *)data; // Decoded output byte pointer

    for (uint8_t code = 0xff, block = 0; byte < buffer + length; --block)
    {
        if (block) // Decode block byte
            *decode++ = *byte++;
        else
        {
            block = *byte++; // Fetch the next block length
            if (block &&
                (code != 0xff)) // Encoded zero, write it unless it's delimiter.
                *decode++ = 0;
            code = block;
            if (!code) // Delimiter code found
                break;
        }
    }

    return (uint32_t)(decode - (uint8_t *)data);
}
