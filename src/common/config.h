#pragma once

#include "packet.h"
#include <limits.h>

#define I2C_TIMEOUT_MS 100
#define MIN_WATCHDOG_INTERVAL_MS 200

/**
 * Command task configuration
 */
#define COMMAND_MNEMONIC_SIZE 1 // number of bytes used to identify command

/**
 * FTP Configuration
 */
// Time to wait between dispatches when idle
#define FTP_IDLE_DISPATCH_MS 1000

// Time to wait between dispatches when active
#define FTP_ACTIVE_DISPATCH_MS 100

// The number of packets to require before moving on to the next n packets
#define FTP_NUM_PACKETS_PER_CYCLE 5

// Note: FTP_NUM_PACKETS_PER_CYCLE must be <= number of bits in
// FTP_PACKET_TRACKER_T
// A bit set means the corresponding packet in this cycle was received.
typedef uint8_t FTP_PACKET_TRACKER_T;

_Static_assert(FTP_NUM_PACKETS_PER_CYCLE <=
                   (sizeof(FTP_PACKET_TRACKER_T) * __CHAR_BIT__),
               "FTP_NUM_PACKETS_PER_CYCLE must be less than or equal to the "
               "number of bits in FTP_PACKET_TRACKER_T");

// Automatically calculated size of maximum data payload in bytes per packet
#define FTP_DATA_PAYLOAD_SIZE                                                  \
    (PACKET_DATA_SIZE - COMMAND_MNEMONIC_SIZE -                                \
     sizeof(FILESYS_BUFFERED_FNAME_T) - sizeof(FTP_PACKET_SEQUENCE_T))

// Type used to represent packet sequence IDs
typedef uint16_t FTP_PACKET_SEQUENCE_T;

// Maximum length of a file, as determined by the number of bytes for sequence
// id and the maximum data payload size
#define FTP_MAX_FILE_LEN                                                       \
    (1ULL << (sizeof(FTP_PACKET_SEQUENCE_T) * __CHAR_BIT__)) *                 \
        FTP_DATA_PAYLOAD_SIZE

/**
 * Filesystem configuration
 */
// Number of bytes to use for storing the filename
typedef uint16_t FILESYS_BUFFERED_FNAME_T;

// Helper definition for maximum number of files that can be written based on
// size of FILESYS_BUFFERED_FNAME_T.
// Note that \0 cannot be in either of the two bytes used for the filename,
// since we use null-terminated strings to store filenames. This means the
// maximum number of files is actually 2^16 - 2 = 65534.
#define FILESYS_MAX_FILES                                                      \
    (1 << (sizeof(FILESYS_BUFFERED_FNAME_T) * CHAR_BIT)) - 2

// Size of buffer used for filesystem writes
#define FILESYS_BUFFER_SIZE (FTP_DATA_PAYLOAD_SIZE * FTP_NUM_PACKETS_PER_CYCLE)

_Static_assert(
    FILESYS_BUFFER_SIZE >= FTP_DATA_PAYLOAD_SIZE * FTP_NUM_PACKETS_PER_CYCLE,
    "FILESYS_BUFFER_SIZE must be at least as large as the maximum "
    "data payload size for the number of packets per cycle for FTP support");

typedef uint16_t
    FILESYS_BUFFER_SIZE_T; // Must be able to hold FILESYS_BUFFER_SIZE

_Static_assert(
    FILESYS_BUFFER_SIZE <= (1ULL << (sizeof(FILESYS_BUFFER_SIZE_T) * CHAR_BIT)),
    "FILESYS_BUFFER_SIZE_T must be able to hold FILESYS_BUFFER_SIZE");

_Static_assert(
    FILESYS_BUFFER_SIZE <=
        100000, // Maximum of 100KB buffer size just as an upper bound
    "FILESYS_BUFFER_SIZE must be a multiple of FILESYS_BLOCK_SIZE");

// Size of buffer used for filesystem reads (specifically for computing CRC)
#define FILESYS_READ_BUFFER_SIZE 256
typedef uint16_t FILESYS_READ_BUFFER_SIZE_T; // Must be able to hold
                                             // FILESYS_READ_BUFFER_SIZE

_Static_assert(FILESYS_READ_BUFFER_SIZE <=
                   (1ULL << (sizeof(FILESYS_READ_BUFFER_SIZE_T) * CHAR_BIT)),
               "FILESYS_READ_BUFFER_SIZE_T must be able to hold "
               "FILESYS_READ_BUFFER_SIZE");

// Note: Only one file can be buffered at a time, so there is no configuration
// for FILESYS_MAX_BUFFERED_FILES.

// Type for storing filename str, based on FILESYS_BUFFERED_FNAME_T
typedef char FILESYS_BUFFERED_FNAME_STR_T[sizeof(FILESYS_BUFFERED_FNAME_T) + 1];

// Number of bytes to use for storing the length of a file
typedef uint32_t FILESYS_BUFFERED_FILE_LEN_T;

_Static_assert(FTP_MAX_FILE_LEN <=
                   (1ULL << (sizeof(FILESYS_BUFFERED_FILE_LEN_T) * CHAR_BIT)),
               "FTP_MAX_FILE_LEN exceeds maximum representable file length in "
               "FILESYS_BUFFERED_FILE_LEN_T");

// Number of bytes to use for storing the CRC of a file
typedef uint32_t FILESYS_BUFFERED_FILE_CRC_T;

// Upper bound for while loop when listing files, to prevent infinite loops in
// case of filesystem corruption or any other issues. This should be set high
// enough to allow listing all files on disk, but low enough to prevent infinite
// loops.
// We give a little overhead therefore over FILESYS_MAX_FILES to allow for some
// unexpected extra files on disk or empty directories, but this should still be
// a reasonable upper bound to prevent infinite loops.
#define FILESYS_MAX_LOOP_LIST_FILES FILESYS_MAX_FILES * 2

// Root directory for our filesystem. Note that little-fs does not require this,
// but it is kept here for clarity and in case we want to add support for
// subdirectories in the future. All files are currently stored in the root
// directory.
#define FILESYS_ROOT_DIR "/"

// TODO: What is our average file size?
// This is currently set to 1KB blocks, which is approx. what we buffer
// in RAM during FTP.
#define FILESYS_BLOCK_SIZE 1024
#define FILESYS_BLOCK_COUNT 512 // 512KB MRAM total
