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
    uint8_t data[256 - 5 - 4 - 4 - TC_SHA256_DIGEST_SIZE];
    uint32_t boot_count;
    uint32_t msg_id;
    uint8_t hmac[TC_SHA256_DIGEST_SIZE]; // This MUST be the last field in the
                                         // struct for is_packet_authenticated
                                         // to work correctly
} packet_t;

/**
 * Verifies the authenticity of a packet by computing its HMAC and comparing it
 * to the provided HMAC field. Also checks for replay attacks using boot_count
 * and msg_id.
 *
 * @param packet Pointer to the packet to authenticate.
 * @param current_boot_count The current reboot counter for replay protection.
 * @return true if the HMAC matches and not a replay, false otherwise.
 */
bool is_packet_authenticated(packet_t *packet, uint32_t current_boot_count);
