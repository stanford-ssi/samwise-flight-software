#pragma once

#include "pico/types.h"
#include "tinycrypt/sha256.h"

typedef struct
{
    uint8_t src;
    uint8_t dst;
    uint8_t flags;
    uint8_t seq;
    uint8_t len; // this should be the length of the packet structure being sent
                 // over
    uint8_t data[256 - 5 - TC_SHA256_DIGEST_SIZE];
    uint8_t hmac[TC_SHA256_DIGEST_SIZE]; // This MUST be the last field in the
                                         // struct for is_packet_authenticated
                                         // to work correctly
} packet_t;

/**
 * Verifies the authenticity of a packet by computing its HMAC and comparing it
 * to the provided HMAC field. Assumes the packet struct is contiguous in
 * memory with the HMAC as the last field and no padding before it.
 *
 * @param packet Pointer to the packet to authenticate.
 * @return true if the HMAC matches (authenticated), false otherwise.
 */
bool is_packet_authenticated(packet_t *packet);

/*

Here is where we will define all of the structs that hold arguments for calling
certain tasks.

*/
typedef struct
{

} TASK1_DATA;

typedef struct
{
    bool yes_no;
    uint16_t number;
} TASK2_DATA;
