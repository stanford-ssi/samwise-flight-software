/**
 * @author  @developStorm
 * @date    2025-05-17
 *
 * Implementation of packet authentication methods.
 */

#include "packet.h"
#include "logger.h"

#include <tinycrypt/hmac.h>
#include <tinycrypt/sha256.h>
#include <tinycrypt/utils.h>

#define PACKET_HMAC_PSK_LEN 32

_Static_assert(offsetof(packet_t, hmac) ==
                   sizeof(packet_t) - TC_SHA256_DIGEST_SIZE,
               "hmac must be the last field and no padding before hmac");
_Static_assert(sizeof(packet_t) == 256, "packet_t size must be 256 bytes");

// Track last seen message ID for replay protection
static uint32_t last_seen_msg_id = 0;

/*
 * Packet Authentication Requirements & Expectations
 *
 * 1. HMAC Authentication:
 *    - Each packet must be authenticated using HMAC-SHA256.
 *    - The HMAC is computed over all packet fields except the HMAC field
 * itself.
 *    - The pre-shared key (PSK) used for HMAC must be 32 bytes
 * (PACKET_HMAC_PSK_LEN).
 *    - The HMAC field must be the last field in the packet_t struct, with no
 * padding before it.
 *    - The total size of packet_t must be exactly 256 bytes.
 *
 * 2. Replay Protection:
 *    - Each packet includes a boot_count and msg_id.
 *    - The boot_count must match the current system boot count.
 *    - The msg_id must be strictly greater than the last seen msg_id for the
 * current boot_count.
 *    - If either check fails, the packet is considered a replay and is
 * rejected.
 *
 * 3. Verification Process:
 *    - If PACKET_HMAC_PSK is defined, authentication is enforced as above.
 *    - If PACKET_HMAC_PSK is not defined, authentication is bypassed (all
 * packets are accepted).
 *    - On successful authentication, last_seen_msg_id is updated.
 *    - On failure, an error is logged and the packet is rejected.
 */

bool is_packet_authenticated(packet_t *packet, uint32_t current_boot_count)
{
#ifdef PACKET_HMAC_PSK
    if (packet == NULL)
    {
        LOG_ERROR("is_packet_authenticated: NULL packet pointer");
        return false;
    }

    // Replay protection: check boot_count and msg_id
    if (packet->boot_count != current_boot_count)
    {
        LOG_ERROR("Replay detected: packet boot_count %u != current %u",
                  packet->boot_count, current_boot_count);
        return false;
    }
    if (packet->msg_id <= last_seen_msg_id)
    {
        LOG_ERROR("Replay detected: packet msg_id %u <= last seen %u for "
                  "boot_count %u",
                  packet->msg_id, last_seen_msg_id, packet->boot_count);
        return false;
    }

    struct tc_hmac_state_struct hmac;
    tc_hmac_set_key(&hmac, (const uint8_t *)PACKET_HMAC_PSK,
                    PACKET_HMAC_PSK_LEN);
    tc_hmac_init(&hmac);
    tc_hmac_update(&hmac, (const void *)packet, offsetof(packet_t, hmac));

    uint8_t out_hmac[TC_SHA256_DIGEST_SIZE];
    tc_hmac_final(out_hmac, TC_SHA256_DIGEST_SIZE, &hmac);

    if (_compare(out_hmac, packet->hmac, TC_SHA256_DIGEST_SIZE) != 0)
    {
        LOG_ERROR("HMAC mismatch: computed and provided HMACs do not match");
        return false;
    }

    // Update last seen msg_id since this packet is authenticated
    last_seen_msg_id = packet->msg_id;

    return true;
#else  // If PACKET_HMAC_PSK is not defined, skip authentication
    return true;
#endif // PACKET_HMAC_PSK
}
