/**
 * @author  Thomas Haile
 * @date    2025-05-24
 *
 * Command parsing and data structure definitions
 */

#pragma once

#include "macros.h"
#include "packet.h"
#include "payload_uart.h"
#include "slate.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*
 * Command IDs are part of the ground <-> satellite wire protocol: the value
 * is transmitted verbatim as data[0] of an uplinked packet.
 * ground_station/config.py hardcodes the same numbers, so the two lists must
 * agree exactly.
 *
 * Number every entry explicitly. Never reorder, renumber, or reuse a value.
 * Appending to an implicitly-numbered enum silently claims the next ID, which
 * is how OTA_CMD once collided with the ground station's PAYLOAD_SHUTDOWN.
 */
typedef enum
{
    PING = 0,
    PAYLOAD_EXEC = 1,
    PAYLOAD_TURN_ON = 2,
    PAYLOAD_TURN_OFF = 3,
    MANUAL_STATE_OVERRIDE = 4,
    // 5 is reserved for PAYLOAD_SHUTDOWN. The ground station already sends
    // it (radio_commands.py:send_payload_shutdown) but there is no handler
    // here yet, so it falls through to `default`. Do not reuse this value.
    OTA_CMD = 6
    // add more commands here as needed: new IDs only, never renumber
} Command;

// Packet configuration
#define COMMAND_MNEMONIC_SIZE 1 // number of bytes used to identify command

/**
 * Command data structures
 *
 * How to add new command:
 * 1. Define command ID
 * 2. Define data structure (e.g., typedef struct { ... } TASK3_DATA;)
 * 3. Add queue initialization in command_task_init()
 * 4. Add case in dispatch_command()
 */

typedef struct
{
    char serialized_command[sizeof(((packet_t *)0)->data) -
                            COMMAND_MNEMONIC_SIZE];
    uint16_t seq_num;     // Sequence number for command execution
    Command command_type; // Command type
} PAYLOAD_COMMAND_DATA;

// data[0]   = OTA_CMD mnemonic (consumed by dispatch_command before this)
// data[1-3] = null-terminated 2-char filename (FILESYS_BUFFERED_FNAME_STR_T)
typedef struct
{
    FILESYS_BUFFERED_FNAME_STR_T fname;
} OTA_COMMAND_DATA;

void dispatch_command(slate_t *slate, packet_t *packet);
