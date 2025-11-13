#pragma once

#include "packet.h"

#define I2C_TIMEOUT_MS 100
#define MIN_WATCHDOG_INTERVAL_MS 200

/**
 * Command task configuration
 */
#define COMMAND_MNEMONIC_SIZE 1 // number of bytes used to identify command

/**
 * File transfer protocol configuration
 */
// The number of packets to require before moving on to the next n packets
#define FTP_NUM_PACKETS_PER_CYCLE 5

// Note: FTP_NUM_PACKETS_PER_CYCLE must be <= number of bits in
// FTP_PACKET_TRACKER_T
#define FTP_PACKET_TRACKER_T uint8_t

// Automatically calculated size of maximum data payload in bytes per packet
#define FTP_DATA_PAYLOAD_SIZE                                                  \
    (PACKET_DATA_SIZE - COMMAND_MNEMONIC_SIZE -                                \
     sizeof(FILESYS_BUFFERED_FNAME_T) - sizeof(FTP_PACKET_SEQUENCE_T))

// Maximum length of a file, as determined by the number of bytes for sequence
// id and the maximum data payload size
#define FTP_MAX_FILE_LEN                                                       \
    (1 << sizeof(FTP_PACKET_SEQUENCE_T)) * FTP_DATA_PAYLOAD_SIZE

// Type used to represent packet sequence IDs
#define FTP_PACKET_SEQUENCE_T uint16_t

/**
 * Filesystem configuration
 */
// Size of buffer used for filesystem writes
#define FILESYS_BUFFER_SIZE (FTP_DATA_PAYLOAD_SIZE * FTP_NUM_PACKETS_PER_CYCLE)
#define FILESYS_BUFFER_SIZE_T                                                  \
    uint16_t // Must be able to hold FILESYS_BUFFER_SIZE

// Size of buffer used for filesystem reads (specifically for computing CRC)
#define FILESYS_READ_BUFFER_SIZE 256
#define FILESYS_READ_BUFFER_SIZE_T                                             \
    uint16_t // Must be able to hold
             // FILESYS_READ_BUFFER_SIZE

// Note: Only one file can be buffered at a time, so there is no configuration
// for FILESYS_MAX_BUFFERED_FILES.

// Number of bytes to use for storing the filename
#define FILESYS_BUFFERED_FNAME_T uint16_t

// Number of bytes to use for storing the length of a file
#define FILESYS_BUFFERED_FILE_LEN_T uint32_t

// Number of bytes to use for storing the CRC of a file
#define FILESYS_BUFFERED_FILE_CRC_T uint32_t
