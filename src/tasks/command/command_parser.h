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

    /* FTP Commands
       All docs are in src/tasks/ftp/README.md. See this doc for specifics on
       how to use these commands. */

    // Reformats and initializes the Little-FS portion of the MRAM, which should
    // only be done when it seems corrupted (will lose all data in the
    // partition)!
    FTP_REFORMAT,

    // Starts a file write with a certain file name, file length, and file crc.
    FTP_START_FILE_WRITE,

    // Writes a packet's worth of data to the file, which can happen out of
    // order. Specific packet format outlined in src/tasks/ftp/README.md. Note
    // this automatically completes the file write if all packets are recieved.
    FTP_WRITE_TO_FILE,

    // Cancels the current file write, thereby clearing the SRAM cache. Note
    // that it doesn't explicitly delete the file on MRAM.
    FTP_CANCEL_FILE_WRITE,

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
    FILESYS_BUFFERED_FNAME_STR_T fname_str;
    FILESYS_BUFFERED_FILE_LEN_T file_len;
    FILESYS_BUFFERED_FILE_CRC_T file_crc;
} FTP_START_FILE_WRITE_DATA;

typedef struct
{
    FILESYS_BUFFERED_FNAME_STR_T fname_str;
    FTP_PACKET_SEQUENCE_T packet_id;
    PACKET_SIZE_T data_len; // Length of the data payload - might be less than
                            // FTP_DATA_PAYLOAD_SIZE for last packet
    uint8_t data[FTP_DATA_PAYLOAD_SIZE];
} FTP_WRITE_TO_FILE_DATA;

typedef struct
{
    FILESYS_BUFFERED_FNAME_STR_T fname_str;
} FTP_CANCEL_FILE_WRITE_DATA;

void dispatch_command(slate_t *slate, packet_t *packet);
