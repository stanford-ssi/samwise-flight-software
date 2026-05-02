/**
 * @author  Joseph Shetaye
 * @date    2024-11-18
 */

#pragma once

#include <string.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"

#include "macros.h"
#include "pins.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

#include "packet.h"
#include "rfm9x.h"

// LED Color for radio task - Magenta
#define RADIO_TASK_COLOR 255, 0, 255

#define TX_QUEUE_SIZE 16
#define RX_QUEUE_SIZE 16

// First byte of every SAMWISE -> Ground Station packet's data field. Mirrors
// the uplink Command enum / COMMAND_MNEMONIC_SIZE convention so the ground
// station can dispatch on packet type instead of assuming every RX is a beacon.
typedef enum
{
    DOWNLINK_BEACON = 0,
    DOWNLINK_COMMAND_RESPONSE = 1,
    // add more downlink packet types here as needed
} DownlinkPacketId;

#define DOWNLINK_PACKET_ID_SIZE 1

size_t encode_packet(const packet_t *p, uint8_t *buf, size_t bufsize,
                     bool enable_hmac);
void radio_task_init(slate_t *slate);
void radio_task_dispatch(slate_t *slate);

extern sched_task_t radio_task;
