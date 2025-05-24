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

bool is_packet_authenticated(packet_t *packet)
{
#ifdef PACKET_HMAC_PSK
    if (packet == NULL)
    {
        LOG_ERROR("is_packet_authenticated: NULL packet pointer");
        return false;
    }

    struct tc_hmac_state_struct hmac;

    tc_hmac_set_key(&hmac, (const uint8_t *)PACKET_HMAC_PSK,
                    PACKET_HMAC_PSK_LEN);
    tc_hmac_init(&hmac);
    tc_hmac_update(&hmac, (const void *)packet, offsetof(packet_t, hmac));

    uint8_t out_hmac[TC_SHA256_DIGEST_SIZE];
    tc_hmac_final(out_hmac, TC_SHA256_DIGEST_SIZE, &hmac);

    return _compare(out_hmac, packet->hmac, TC_SHA256_DIGEST_SIZE) == 0;
#else // If PACKET_HMAC_PSK is not defined, skip authentication
    return true;

#endif // PACKET_HMAC_PSK
}
