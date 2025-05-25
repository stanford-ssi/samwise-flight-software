/**
 * @author  Thomas Haile 
 * @date    2025-05-24
 *
 * Command parsing and data structure definitions
 */

#pragma once

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "packet.h"
#include "slate.h"

// Command IDs (must start from 1, as 0 indicates "not uploading")
typedef enum {
  TAKE_PHOTO = 1,
  DOWNLOAD_PHOTO,
  TAKE_AND_SEND_PHOTO,
  NO_OP
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
    char serialized_command[sizeof(((packet_t *)0)->data) - COMMAND_MNEMONIC_SIZE];
} PAYLOAD_COMMAND_DATA;

void dispatch_command(slate_t *slate, packet_t *packet);