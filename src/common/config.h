#pragma once

#include "packet.h"

#define I2C_TIMEOUT_MS 100
#define MIN_WATCHDOG_INTERVAL_MS 200

/**
 * Command task configuration
 */
#define COMMAND_MNEMONIC_SIZE 1 // number of bytes used to identify command

/**
 * FTP Configuration
 */
// The number of packets to require before moving on to the next n packets
#define FTP_NUM_PACKETS_PER_CYCLE 5

// Automatically calculated size of maximum data payload in bytes per packet
#define FTP_DATA_PAYLOAD_SIZE                                                  \
    (PACKET_DATA_SIZE - COMMAND_MNEMONIC_SIZE -                                \
     sizeof(FILESYS_BUFFERED_FNAME_T) - sizeof(FTP_PACKET_SEQUENCE_T))

// Type used to represent packet sequence IDs
typedef uint16_t FTP_PACKET_SEQUENCE_T;

/**
 * Filesystem configuration
 */
// Size of buffer used for filesystem writes
#define FILESYS_BUFFER_SIZE (FTP_DATA_PAYLOAD_SIZE * FTP_NUM_PACKETS_PER_CYCLE)
typedef uint16_t
    FILESYS_BUFFER_SIZE_T; // Must be able to hold FILESYS_BUFFER_SIZE

// Size of buffer used for filesystem reads (specifically for computing CRC)
#define FILESYS_READ_BUFFER_SIZE 256
typedef uint16_t FILESYS_READ_BUFFER_SIZE_T; // Must be able to hold
                                             // FILESYS_READ_BUFFER_SIZE

// Note: Only one file can be buffered at a time, so there is no configuration
// for FILESYS_MAX_BUFFERED_FILES.

// Number of bytes to use for storing the filename
typedef uint16_t FILESYS_BUFFERED_FNAME_T;

// Type for storing filename str, based on FILESYS_BUFFERED_FNAME_T
typedef char FILESYS_BUFFERED_FNAME_STR_T[sizeof(FILESYS_BUFFERED_FNAME_T) + 1];

// Number of bytes to use for storing the length of a file
typedef uint32_t FILESYS_BUFFERED_FILE_LEN_T;

// Number of bytes to use for storing the CRC of a file
typedef uint32_t FILESYS_BUFFERED_FILE_CRC_T;

// TODO: What is our average file size?
// This is currently set to 1KB blocks, which is approx. what we buffer
// in RAM during FTP.
#define FILESYS_BLOCK_SIZE 1024
#define FILESYS_BLOCK_COUNT 512 // 512KB MRAM total
