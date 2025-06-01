#pragma once

#include "pico/types.h"
#include "tinycrypt/sha256.h"
#include <stddef.h>

typedef struct
{
    uint8_t dst;
    uint8_t src;
    uint8_t flags;
    uint8_t seq;
    uint8_t len; // this should be the length of the packet structure being sent
                 // over
    uint8_t data[256 - (sizeof(uint8_t) * 5) - (sizeof(uint32_t) * 2) -
                 TC_SHA256_DIGEST_SIZE];
    uint32_t boot_count;
    uint32_t msg_id;
    uint8_t hmac[TC_SHA256_DIGEST_SIZE]; // This MUST be the last field in the
                                         // struct for is_packet_authenticated
                                         // to work correctly
} packet_t;

// --- Packet layout constants (derived from struct) ---
#define PACKET_TOTAL_SIZE (sizeof(packet_t))
#define PACKET_HEADER_SIZE (offsetof(packet_t, data))
#define PACKET_DATA_SIZE (sizeof(((packet_t *)0)->data))
#define PACKET_FOOTER_SIZE                                                     \
    (PACKET_TOTAL_SIZE - PACKET_HEADER_SIZE - PACKET_DATA_SIZE)
#define PACKET_HMAC_SIZE (TC_SHA256_DIGEST_SIZE)
#define PACKET_MIN_SIZE (PACKET_HEADER_SIZE + PACKET_FOOTER_SIZE)

_Static_assert(PACKET_TOTAL_SIZE == 256, "packet_t size must be 256 bytes");
_Static_assert(offsetof(packet_t, hmac) ==
                   sizeof(packet_t) - TC_SHA256_DIGEST_SIZE,
               "hmac must be last");

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
