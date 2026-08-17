/*
 * Author:      @Carson Lauer
 * Date:        April 26, 2026
 * Description: Communication protocol header.
 * Includes a protocol to communicate
 */

#include <string.h>

#include "protocol.h"

/*
enum {
    MSG_PING,           // No-op, but returns a pong
    MSG_PONG,
    MSG_STRING,
    MSG_COMMAND,        // Execute command
    MSG_ADCS_PACKET     // ADCS
};

typedef struct protocol_msg {
    uint8_t src;
    uint8_t dst;
    uint8_t seq;
    uint8_t flags;
    uint8_t type;
    uint8_t len;
    uint8_t *payload;
    uint8_t crc8;
} msg_t;
*/

// TODO: Implement these values
void protocol_message_ping(msg_t *msg)
{
    msg->src = 1;
    msg->dst = 0;
    msg->seq = 0;
    msg->flags = 0;
    msg->type = MSG_PING;
    msg->len = 1;
    msg->payload = (uint8_t *)0; // null ptr
    msg->crc8 = 0;
}

void protocol_message_pong(msg_t *msg)
{
    msg->src = 1;
    msg->dst = 0;
    msg->seq = 0;
    msg->flags = 0;
    msg->type = MSG_PONG;
    msg->len = 1;
    msg->payload = (uint8_t *)0; // null ptr
    msg->crc8 = 0;
}

void protocol_message_command(msg_t *msg, const uint8_t *command)
{
    msg->src = 1;
    msg->dst = 0;
    msg->seq = 0;
    msg->flags = 0;
    msg->type = MSG_COMMAND;
    msg->len = 1;
    msg->payload = command;
    msg->crc8 = 0;
}

void protocol_message_string(msg_t *msg, const uint8_t *s)
{
    uint8_t len = strlen((const char *)s);
    msg->src = 1;
    msg->dst = 0;
    msg->seq = 0;
    msg->flags = 0;
    msg->type = MSG_STRING;
    msg->len = len;
    msg->payload = s;
    msg->crc8 = 0;
}

void protocol_message_adcs(msg_t *msg, const adcs_packet_t *adcs)
{
    uint8_t len = sizeof(adcs_packet_t);
    msg->src = 1;
    msg->dst = 0;
    msg->seq = 0;
    msg->flags = 0;
    msg->type = MSG_ADCS_PACKET;
    msg->len = len;
    msg->payload = (const uint8_t *)adcs;
    msg->crc8 = 0;
}

/*
 * Removes payload pointer and inserts payload between
 */
void protocol_message_encode(msg_t *msg, uint8_t *buf)
{
    memcpy(buf, (uint8_t *)msg, 6); // Copy first 6 bytes
    buf += 6;
    if (msg->payload == 0 || msg->len == 0)
    {
        memcpy(buf, &msg->payload, 1);
        buf += 1;
    }
    else
    {
        memcpy(buf, msg->payload, msg->len);
        buf += msg->len;
    }
    memcpy(buf, &msg->crc8, 1); // copy crc byte
}

void protocol_message_decode(msg_t *msg, uint32_t len, uint8_t *buf)
{
    uint8_t *msg_buf = (uint8_t *)msg;
    for (int i = 0; i < 6; i++)
    {
        msg_buf[i] = buf[i];
    }
    msg->payload = buf + 6;
    msg->crc8 = buf[len - 1];
}
