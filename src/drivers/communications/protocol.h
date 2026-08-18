#pragma once
/*
 * Author:      @Carson Lauer
 * Date:        April 26, 2026
 * Description: Communication protocol header.
 * Includes a protocol to communicate
 */

#include <stdint.h>

#include "adcs_packet.h"

enum
{
    MSG_PING, // No-op, but returns a pong
    MSG_PONG,
    MSG_STRING,
    MSG_COMMAND,    // Execute command
    MSG_ADCS_PACKET // ADCS
};

typedef struct protocol_msg
{
    uint8_t src;
    uint8_t dst;
    uint8_t seq;
    uint8_t flags;
    uint8_t type;
    uint8_t len;
    uint8_t *payload;
    uint8_t crc8;
} msg_t;

/*
 * Creates a protocol ping message
 */
void protocol_message_ping(msg_t *msg);

void protocol_message_pong(msg_t *msg);

void protocol_message_command(msg_t *msg, uint8_t command);

void protocol_message_string(msg_t *msg, uint8_t *s);

void protocol_message_adcs(msg_t *msg, adcs_packet_t *adcs);

/*
 * Takes a message and formats it into a buffer
 */
void protocol_message_encode(msg_t *msg, uint8_t *buf);
void protocol_message_decode(msg_t *msg, uint32_t len, uint8_t *buf);
