/**
 * @author  Thomas Haile
 * @date    2025-05-24
 *
 * Command parsing and data structure definitions
 */

#pragma once

#include "macros.h"
#include "packet.h"
#include "slate.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef enum
{
    NO_OP,
    PAYLOAD_EXEC,
    PAYLOAD_TURN_ON,
    PAYLOAD_TURN_OFF
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
    uint16_t seq_num; // Sequence number for command execution
} PAYLOAD_COMMAND_DATA;

void dispatch_command(slate_t *slate, packet_t *packet);
