/**
 * @author  Thomas Haile
 * @date    2025-05-24
 *
 * Command parsing and data structure definitions
 */

#pragma once

#include "config.h"
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

    /* FTP Commands */
    FTP_START_FILE_WRITE,
    FTP_WRITE_TO_FILE,
    FTP_CANCEL_FILE_WRITE,
    FTP_FORMAT_FILESYSTEM,

    // add more commands here as needed
} Command;

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
    char serialized_command[PACKET_DATA_SIZE - COMMAND_MNEMONIC_SIZE];
    uint16_t seq_num;     // Sequence number for command execution
    Command command_type; // Command type
} PAYLOAD_COMMAND_DATA;

typedef struct
{
    FILESYS_BUFFERED_FNAME_T fname;
    FILESYS_BUFFERED_FILE_LEN_T file_len;
    FILESYS_BUFFERED_FILE_CRC_T file_crc;
} FTP_START_FILE_WRITE_DATA;

typedef struct
{
    FILESYS_BUFFERED_FNAME_T fname;
    FTP_PACKET_SEQUENCE_T packet_id;
    PACKET_SIZE_T data_len; // Length of the data payload - might be less than
                            // FTP_DATA_PAYLOAD_SIZE for last packet
    uint8_t data[FTP_DATA_PAYLOAD_SIZE];
} FTP_WRITE_TO_FILE_DATA;

typedef struct
{
    FILESYS_BUFFERED_FNAME_T fname;
} FTP_CANCEL_FILE_WRITE_DATA;

void dispatch_command(slate_t *slate, packet_t *packet);
