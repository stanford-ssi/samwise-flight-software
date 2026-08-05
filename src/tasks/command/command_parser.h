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

typedef enum
{
    PING,
    PAYLOAD_EXEC,
    PAYLOAD_TURN_ON,
    PAYLOAD_TURN_OFF,
    MANUAL_STATE_OVERRIDE,
    OTA_CMD
    // add more commands here as needed
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
