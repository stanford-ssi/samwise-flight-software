#define I2C_TIMEOUT_MS 100
#define MIN_WATCHDOG_INTERVAL_MS 200

/**
 * File transfer protocol configuration
 */
// The number of packets to require before moving on to the next n packets
#define FTP_NUM_PACKETS_PER_CYCLE 5

// Note: FTP_NUM_PACKETS_PER_CYCLE must be <= number of bits in
// FTP_PACKET_TRACKER_T
#define FTP_PACKET_TRACKER_T uint8_t

// Maximum length of a file, as determined by the number of bytes for sequence
// id (2, so 2^(8 * 2) possibilities) and the number of bytes uploaded per
// packet (205)
#define FTP_MAX_FILE_LEN (1 << (8 * 2)) * 205

/**
 * Filesystem configuration
 */
// Each packet has a max of 205 bytes of data + 1 byte for the command ID +
// 2 bytes for fname + 2 bytes for sequence id (sum = 210)
#define FILESYS_BUFFER_SIZE (205 * FTP_NUM_PACKETS_PER_CYCLE)
#define FILESYS_BUFFER_SIZE_T                                                  \
    uint16_t // Must be able to hold FILESYS_BUFFER_SIZE

// Note: Only one file can be buffered at a time, so there is no configuration
// for FILESYS_MAX_BUFFERED_FILES.

// Number of bytes to use for storing the filename
#define FILESYS_BUFFERED_FNAME_T uint16_t

// Number of bytes to use for storing the length of a file
#define FILESYS_BUFFERED_FILE_LEN_T uint32_t

// Number of bytes to use for storing the CRC of a file
#define FILESYS_BUFFERED_FILE_CRC_T uint32_t
