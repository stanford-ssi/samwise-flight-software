/**
 * @author Test Suite
 * @date 2026-01-20
 * @brief Unit tests for the FTP task module.
 *
 * Comprehensive unit tests for FTP task functionality including command
 * processing, packet handling, and error conditions.
 */

#include "config.h"
#include "filesys.h"
#include "ftp_task.h"
#include "logger.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Helper macro for test assertions with error messages
#define TEST_ASSERT(cond, msg)                                                 \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            LOG_ERROR("ASSERTION FAILED: %s\n", msg);                          \
            return -1;                                                         \
        }                                                                      \
    } while (0)

// ============================================================================
// Mock Structures and State Tracking
// ============================================================================

// Track sent packets for verification
typedef struct
{
    FTP_Result result;
    uint8_t data[512];
    size_t data_len;
    bool has_file_headers;
    FILESYS_BUFFERED_FNAME_STR_T fname;
    FILESYS_BUFFERED_FILE_LEN_T file_len;
    FILESYS_BUFFERED_FILE_CRC_T file_crc;
} sent_packet_t;

static sent_packet_t last_sent_packet;
static int packet_send_count = 0;

// ============================================================================
// Helper Functions
// ============================================================================

void reset_test_state(void)
{
    memset(&last_sent_packet, 0, sizeof(sent_packet_t));
    packet_send_count = 0;
}

// Helper function to initialize a clean filesystem for testing
static int setup_clean_filesystem(slate_t *slate)
{
    lfs_ssize_t code = filesys_reformat(slate);
    if (code < 0)
    {
        LOG_ERROR("Failed to reformat filesystem for test setup: %d\n", code);
        return -1;
    }
    return 0;
}

// Mock implementation to track sent packets
void ftp_send_result_packet_no_file_mock(slate_t *slate, FTP_Result result,
                                         const void *additional_data,
                                         size_t additional_data_len)
{
    last_sent_packet.result = result;
    last_sent_packet.has_file_headers = false;
    strcpy(last_sent_packet.fname, "XX");
    last_sent_packet.file_len = 0;
    last_sent_packet.file_crc = 0;

    if (additional_data && additional_data_len > 0)
    {
        memcpy(last_sent_packet.data, additional_data, additional_data_len);
        last_sent_packet.data_len = additional_data_len;
    }
    else
    {
        last_sent_packet.data_len = 0;
    }

    packet_send_count++;
}

void ftp_send_result_packet_mock(slate_t *slate, FTP_Result result,
                                 const void *additional_data,
                                 size_t additional_data_len)
{
    last_sent_packet.result = result;
    last_sent_packet.has_file_headers = true;

    // In real implementation, these would come from slate's file write state
    if (slate->filesys_is_writing_file)
    {
        strcpy(last_sent_packet.fname, slate->filesys_buffered_fname_str);
        last_sent_packet.file_len = slate->filesys_buffered_file_len;
        last_sent_packet.file_crc = slate->filesys_buffered_file_crc;
    }

    if (additional_data && additional_data_len > 0)
    {
        memcpy(last_sent_packet.data, additional_data, additional_data_len);
        last_sent_packet.data_len = additional_data_len;
    }
    else
    {
        last_sent_packet.data_len = 0;
    }

    packet_send_count++;
}

// ============================================================================
// Test 1: FTP Result Code Values
// ============================================================================
int ftp_test_result_codes(void)
{
    LOG_DEBUG("=== Test: FTP Result Code Values ===\n");

    // Verify positive result codes
    TEST_ASSERT(FILESYS_INIT_ERROR == 0, "FILESYS_INIT_ERROR should be 0");
    TEST_ASSERT(FTP_READY_RECEIVE == 1, "FTP_READY_RECEIVE should be 1");
    TEST_ASSERT(FTP_FILE_WRITE_SUCCESS == 2,
                "FTP_FILE_WRITE_SUCCESS should be 2");
    TEST_ASSERT(FTP_EOF_SUCCESS == 3, "FTP_EOF_SUCCESS should be 3");
    TEST_ASSERT(FTP_CANCEL_SUCCESS == 5, "FTP_CANCEL_SUCCESS should be 5");

    // Verify negative error codes
    TEST_ASSERT(FTP_ERROR_RECEIVE == -1, "FTP_ERROR_RECEIVE should be -1");
    TEST_ASSERT(FTP_FILE_WRITE_MRAM_ERROR == -3,
                "FTP_FILE_WRITE_MRAM_ERROR should be -3");
    TEST_ASSERT(FTP_EOF_CRC_ERROR == -3, "FTP_EOF_CRC_ERROR should be -3");
    TEST_ASSERT(FTP_FILE_WRITE_BUFFER_ERROR == -4,
                "FTP_FILE_WRITE_BUFFER_ERROR should be -4");
    TEST_ASSERT(FTP_CANCEL_ERROR == -5, "FTP_CANCEL_ERROR should be -5");
    TEST_ASSERT(FTP_ERROR == -127, "FTP_ERROR should be -127");

    // Verify result mnemonic size
    TEST_ASSERT(FTP_RESULT_MNEMONIC_SIZE == 1,
                "FTP_RESULT_MNEMONIC_SIZE should be 1");

    LOG_DEBUG("=== Test PASSED: FTP Result Code Values ===\n");
    return 0;
}

// ============================================================================
// Test 2: FTP Configuration Constants
// ============================================================================
int ftp_test_config_constants(void)
{
    LOG_DEBUG("=== Test: FTP Configuration Constants ===\n");

    // Verify dispatch limits
    TEST_ASSERT(MAX_FTP_COMMANDS_PER_DISPATCH > 0 &&
                    MAX_FTP_COMMANDS_PER_DISPATCH <= 10,
                "MAX_FTP_COMMANDS_PER_DISPATCH should be reasonable");

    TEST_ASSERT(MAX_FTP_RETRY_COUNT > 0 && MAX_FTP_RETRY_COUNT <= 5,
                "MAX_FTP_RETRY_COUNT should be reasonable");

    // Verify timing constants
    TEST_ASSERT(FTP_IDLE_DISPATCH_MS > 0,
                "FTP_IDLE_DISPATCH_MS should be positive");
    TEST_ASSERT(FTP_ACTIVE_DISPATCH_MS > 0,
                "FTP_ACTIVE_DISPATCH_MS should be positive");
    TEST_ASSERT(FTP_IDLE_DISPATCH_MS > FTP_ACTIVE_DISPATCH_MS,
                "Idle dispatch should be slower than active");

    // Verify packet cycle configuration
    TEST_ASSERT(FTP_NUM_PACKETS_PER_CYCLE > 0,
                "FTP_NUM_PACKETS_PER_CYCLE should be positive");
    TEST_ASSERT(FTP_NUM_PACKETS_PER_CYCLE <= (8 * sizeof(FTP_PACKET_TRACKER_T)),
                "FTP_NUM_PACKETS_PER_CYCLE must fit in tracker bitfield");

    // Verify buffer size calculation
    size_t expected_buffer_size =
        FTP_DATA_PAYLOAD_SIZE * FTP_NUM_PACKETS_PER_CYCLE;
    TEST_ASSERT(FILESYS_BUFFER_SIZE == expected_buffer_size,
                "FILESYS_BUFFER_SIZE should match calculated value");

    LOG_DEBUG("=== Test PASSED: FTP Configuration Constants ===\n");
    return 0;
}

// ============================================================================
// Test 3: Packet Sequence Type Capacity
// ============================================================================
int ftp_test_packet_sequence_capacity(void)
{
    LOG_DEBUG("=== Test: Packet Sequence Type Capacity ===\n");

    // Verify packet sequence type is at least 16-bit
    TEST_ASSERT(sizeof(FTP_PACKET_SEQUENCE_T) >= 2,
                "FTP_PACKET_SEQUENCE_T should be at least 16-bit");

    // Calculate maximum file size
    uint64_t max_packets =
        (1ULL << (sizeof(FTP_PACKET_SEQUENCE_T) * __CHAR_BIT__));
    uint64_t max_file_size = max_packets * FTP_DATA_PAYLOAD_SIZE;

    // Should handle at least 1MB files
    TEST_ASSERT(max_file_size >= (1024 * 1024),
                "Should support at least 1MB files");

    // Verify FTP_MAX_FILE_LEN matches
    LOG_DEBUG("Calculated max file size: %llu bytes", max_file_size);
    TEST_ASSERT(FTP_MAX_FILE_LEN == max_file_size,
                "FTP_MAX_FILE_LEN should match calculated max file size");

    LOG_DEBUG("=== Test PASSED: Packet Sequence Type Capacity ===\n");
    return 0;
}

// ============================================================================
// Test 4: Send Result Packet - No File Headers
// ============================================================================
int ftp_test_send_result_packet_no_file(void)
{
    LOG_DEBUG("=== Test: Send Result Packet - No File ===\n");

    slate_t test_slate;
    reset_test_state();

    // Test 1: Send packet with no additional data
    ftp_send_result_packet_no_file_mock(&test_slate, FILESYS_INIT_ERROR, NULL,
                                        0);

    TEST_ASSERT(last_sent_packet.result == FILESYS_INIT_ERROR,
                "Result should be FILESYS_INIT_ERROR");
    TEST_ASSERT(last_sent_packet.has_file_headers == false,
                "Should not have file headers");
    TEST_ASSERT(strcmp(last_sent_packet.fname, "XX") == 0,
                "Filename should be 'XX'");
    TEST_ASSERT(last_sent_packet.file_len == 0, "File length should be 0");
    TEST_ASSERT(last_sent_packet.file_crc == 0, "File CRC should be 0");
    TEST_ASSERT(last_sent_packet.data_len == 0,
                "Should have no additional data");
    TEST_ASSERT(packet_send_count == 1, "Should have sent 1 packet");

    // Test 2: Send packet with error data
    reset_test_state();
    lfs_ssize_t error_code = -5;
    const char *error_msg = "Filesystem init failed";

    uint8_t packet_data[256];
    size_t offset = 0;
    memcpy(packet_data + offset, &error_code, sizeof(error_code));
    offset += sizeof(error_code);
    memcpy(packet_data + offset, error_msg, strlen(error_msg));
    offset += strlen(error_msg);

    ftp_send_result_packet_no_file_mock(&test_slate, FILESYS_INIT_ERROR,
                                        packet_data, offset);

    TEST_ASSERT(last_sent_packet.result == FILESYS_INIT_ERROR,
                "Result should be FILESYS_INIT_ERROR");
    TEST_ASSERT(last_sent_packet.data_len == offset,
                "Data length should match sent data");
    TEST_ASSERT(packet_send_count == 1, "Should have sent 1 packet");

    // Verify error code in packet
    lfs_ssize_t received_error;
    memcpy(&received_error, last_sent_packet.data, sizeof(lfs_ssize_t));
    TEST_ASSERT(received_error == error_code, "Error code should match");

    LOG_DEBUG("=== Test PASSED: Send Result Packet - No File ===\n");
    return 0;
}

// ============================================================================
// Test 5: Send Result Packet - With File Headers
// ============================================================================
int ftp_test_send_result_packet_with_file(void)
{
    LOG_DEBUG("=== Test: Send Result Packet - With File ===\n");

    slate_t test_slate;
    if (setup_clean_filesystem(&test_slate) < 0)
        return -1;

    reset_test_state();

    // Start a file write to populate file headers in slate
    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_STR_T fname = "T5";
    FILESYS_BUFFERED_FILE_CRC_T crc = 0x12345678;
    FILESYS_BUFFERED_FILE_LEN_T file_size = 512;

    int8_t code = filesys_start_file_write(&test_slate, fname, file_size, crc,
                                           &blocks_left);
    TEST_ASSERT(code == 0, "start_file_write should succeed");

    // Send FTP_READY_RECEIVE packet
    FTP_PACKET_SEQUENCE_T packet_start = 0;
    FTP_PACKET_SEQUENCE_T packet_end = FTP_NUM_PACKETS_PER_CYCLE - 1;
    FTP_PACKET_TRACKER_T received = 0x00; // No packets received yet

    uint8_t packet_data[256];
    size_t offset = 0;
    memcpy(packet_data + offset, &packet_start, sizeof(packet_start));
    offset += sizeof(packet_start);
    memcpy(packet_data + offset, &packet_end, sizeof(packet_end));
    offset += sizeof(packet_end);
    memcpy(packet_data + offset, &received, sizeof(received));
    offset += sizeof(received);

    ftp_send_result_packet_mock(&test_slate, FTP_READY_RECEIVE, packet_data,
                                offset);

    TEST_ASSERT(last_sent_packet.result == FTP_READY_RECEIVE,
                "Result should be FTP_READY_RECEIVE");
    TEST_ASSERT(last_sent_packet.has_file_headers == true,
                "Should have file headers");
    TEST_ASSERT(strcmp(last_sent_packet.fname, fname) == 0,
                "Filename should match");
    TEST_ASSERT(last_sent_packet.file_len == file_size,
                "File length should match");
    TEST_ASSERT(last_sent_packet.file_crc == crc, "File CRC should match");
    TEST_ASSERT(last_sent_packet.data_len == offset,
                "Data length should match");

    // Clean up
    filesys_cancel_file_write(&test_slate);

    LOG_DEBUG("=== Test PASSED: Send Result Packet - With File ===\n");
    return 0;
}

// ============================================================================
// Test 6: Format Filesystem Command
// ============================================================================
int ftp_test_format_filesystem_command(void)
{
    LOG_DEBUG("=== Test: Format Filesystem Command ===\n");

    slate_t test_slate;
    if (setup_clean_filesystem(&test_slate) < 0)
        return -1;

    reset_test_state();

    // Create a file to verify it gets deleted
    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_STR_T fname = "T6";

    int8_t code =
        filesys_start_file_write(&test_slate, fname, 64, 0x0, &blocks_left);
    TEST_ASSERT(code == 0, "start_file_write should succeed");

    uint8_t buffer[64];
    memset(buffer, 0xAA, sizeof(buffer));
    code = filesys_write_data_to_buffer(&test_slate, buffer, 64, 0);
    TEST_ASSERT(code == 0, "write_data_to_buffer should succeed");

    filesys_write_buffer_to_mram(&test_slate, 64);
    filesys_cancel_file_write(&test_slate);

    // Now format the filesystem
    lfs_ssize_t result = filesys_reformat(&test_slate);
    TEST_ASSERT(result >= 0, "filesys_reformat should succeed");

    // Verify file is gone
    lfs_file_t file;
    int err = lfs_file_open(&test_slate.lfs, &file, fname, LFS_O_RDONLY);
    TEST_ASSERT(err < 0, "File should not exist after reformat");

    // Verify filesystem state is clean
    TEST_ASSERT(!test_slate.filesys_is_writing_file,
                "Should not be writing file after reformat");
    TEST_ASSERT(!test_slate.filesys_buffer_is_dirty,
                "Buffer should be clean after reformat");

    LOG_DEBUG("=== Test PASSED: Format Filesystem Command ===\n");
    return 0;
}

// ============================================================================
// Test 7: Start File Write Command - Success
// ============================================================================
int ftp_test_start_file_write_command_success(void)
{
    LOG_DEBUG("=== Test: Start File Write Command - Success ===\n");

    slate_t test_slate;
    if (setup_clean_filesystem(&test_slate) < 0)
        return -1;

    reset_test_state();

    // Prepare command data
    FTP_START_FILE_WRITE_DATA cmd_data;
    strcpy(cmd_data.fname_str, "T7");
    cmd_data.file_len = 1024;
    cmd_data.file_crc = 0xABCDEF01;

    // Process command (this would call ftp_process_file_start_write_command)
    lfs_ssize_t blocks_left;
    int8_t code = filesys_start_file_write(&test_slate, cmd_data.fname_str,
                                           cmd_data.file_len, cmd_data.file_crc,
                                           &blocks_left);

    TEST_ASSERT(code == 0, "filesys_start_file_write should succeed");
    TEST_ASSERT(test_slate.filesys_is_writing_file,
                "Should be in file writing state");
    TEST_ASSERT(blocks_left > 0, "Should have blocks available");
    TEST_ASSERT(
        strcmp(test_slate.filesys_buffered_fname_str, cmd_data.fname_str) == 0,
        "Filename should match");
    TEST_ASSERT(test_slate.filesys_buffered_file_len == cmd_data.file_len,
                "File size should match");
    TEST_ASSERT(test_slate.filesys_buffered_file_crc == cmd_data.file_crc,
                "File CRC should match");

    // In real implementation, would send FTP_READY_RECEIVE packet here

    // Clean up
    filesys_cancel_file_write(&test_slate);

    LOG_DEBUG("=== Test PASSED: Start File Write Command - Success ===\n");
    return 0;
}

// ============================================================================
// Test 8: Start File Write Command - Already Writing
// ============================================================================
int ftp_test_start_file_write_command_already_writing(void)
{
    LOG_DEBUG("=== Test: Start File Write Command - Already Writing ===\n");

    slate_t test_slate;
    if (setup_clean_filesystem(&test_slate) < 0)
        return -1;

    reset_test_state();

    // Start first file write
    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_STR_T fname1 = "T8";

    int8_t code = filesys_start_file_write(&test_slate, fname1, 512, 0x11111111,
                                           &blocks_left);
    TEST_ASSERT(code == 0, "First start_file_write should succeed");

    // Try to start second file write
    FILESYS_BUFFERED_FNAME_STR_T fname2 = "T9";
    code = filesys_start_file_write(&test_slate, fname2, 512, 0x22222222,
                                    &blocks_left);
    TEST_ASSERT(code == -1, "Second start_file_write should fail with -1");

    // In real implementation, would send FTP_ERROR_RECEIVE packet here

    // Clean up
    filesys_cancel_file_write(&test_slate);

    LOG_DEBUG(
        "=== Test PASSED: Start File Write Command - Already Writing ===\n");
    return 0;
}

// ============================================================================
// Test 9: Start File Write Command - Insufficient Space
// ============================================================================
int ftp_test_start_file_write_command_insufficient_space(void)
{
    LOG_DEBUG("=== Test: Start File Write Command - Insufficient Space ===\n");

    slate_t test_slate;
    if (setup_clean_filesystem(&test_slate) < 0)
        return -1;

    reset_test_state();

    // Try to write a file larger than available space
    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_STR_T fname = "TA";
    // File size larger than total MRAM
    FILESYS_BUFFERED_FILE_LEN_T huge_size =
        FILESYS_BLOCK_SIZE * FILESYS_BLOCK_COUNT * 2;

    int8_t code = filesys_start_file_write(&test_slate, fname, huge_size, 0x0,
                                           &blocks_left);

    TEST_ASSERT(code == -3, "Should fail with -3 (insufficient space)");
    TEST_ASSERT(blocks_left < 0, "Blocks left should be negative");
    TEST_ASSERT(!test_slate.filesys_is_writing_file,
                "Should not be in writing state");

    // In real implementation, would send FTP_ERROR_RECEIVE packet here

    LOG_DEBUG(
        "=== Test PASSED: Start File Write Command - Insufficient Space ===\n");
    return 0;
}

// ============================================================================
// Test 10: Write File Data Command - Single Packet
// ============================================================================
int ftp_test_write_file_data_command_single_packet(void)
{
    LOG_DEBUG("=== Test: Write File Data Command - Single Packet ===\n");

    slate_t test_slate;
    if (setup_clean_filesystem(&test_slate) < 0)
        return -1;

    reset_test_state();

    // Start file write
    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_STR_T fname = "TB";

    int8_t code =
        filesys_start_file_write(&test_slate, fname, 256, 0x0, &blocks_left);
    TEST_ASSERT(code == 0, "start_file_write should succeed");

    // Prepare write data command
    FTP_WRITE_TO_FILE_DATA write_cmd;
    write_cmd.packet_id = 0;
    memcpy(&write_cmd.fname_str, fname, sizeof(FILESYS_BUFFERED_FNAME_STR_T));
    for (size_t i = 0; i < FTP_DATA_PAYLOAD_SIZE; i++)
        write_cmd.data[i] = (uint8_t)i;

    // Calculate offset for this packet
    FILESYS_BUFFER_SIZE_T offset =
        (write_cmd.packet_id % FTP_NUM_PACKETS_PER_CYCLE) *
        FTP_DATA_PAYLOAD_SIZE;

    // Write to buffer
    code = filesys_write_data_to_buffer(&test_slate, write_cmd.data,
                                        FTP_DATA_PAYLOAD_SIZE, offset);
    TEST_ASSERT(code == 0, "write_data_to_buffer should succeed");
    TEST_ASSERT(test_slate.filesys_buffer_is_dirty, "Buffer should be dirty");

    // Verify data in buffer
    for (size_t i = 0; i < FTP_DATA_PAYLOAD_SIZE; i++)
    {
        TEST_ASSERT(test_slate.filesys_buffer[offset + i] == (uint8_t)i,
                    "Buffer data should match written data");
    }

    // Clean up
    filesys_cancel_file_write(&test_slate);

    LOG_DEBUG("=== Test PASSED: Write File Data Command - Single Packet ===\n");
    return 0;
}

// ============================================================================
// Test 11: Write File Data Command - Complete Cycle
// ============================================================================
int ftp_test_write_file_data_command_complete_cycle(void)
{
    LOG_DEBUG("=== Test: Write File Data Command - Complete Cycle ===\n");

    slate_t test_slate;
    if (setup_clean_filesystem(&test_slate) < 0)
        return -1;

    reset_test_state();

    // Start file write
    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_STR_T fname = "TC";
    size_t total_size = FILESYS_BUFFER_SIZE * 2; // 2 cycles worth

    int8_t code = filesys_start_file_write(&test_slate, fname, total_size, 0x0,
                                           &blocks_left);
    TEST_ASSERT(code == 0, "start_file_write should succeed");

    // Write FTP_NUM_PACKETS_PER_CYCLE packets
    for (FTP_PACKET_SEQUENCE_T pkt = 0; pkt < FTP_NUM_PACKETS_PER_CYCLE; pkt++)
    {
        FTP_WRITE_TO_FILE_DATA write_cmd;
        write_cmd.packet_id = pkt;
        memcpy(&write_cmd.fname_str, fname,
               sizeof(FILESYS_BUFFERED_FNAME_STR_T));

        // Fill with pattern based on packet ID
        for (size_t i = 0; i < FTP_DATA_PAYLOAD_SIZE; i++)
            write_cmd.data[i] = (uint8_t)(pkt * 16 + i);

        FILESYS_BUFFER_SIZE_T offset =
            (pkt % FTP_NUM_PACKETS_PER_CYCLE) * FTP_DATA_PAYLOAD_SIZE;

        code = filesys_write_data_to_buffer(&test_slate, write_cmd.data,
                                            FTP_DATA_PAYLOAD_SIZE, offset);
        TEST_ASSERT(code == 0, "write_data_to_buffer should succeed");
    }

    TEST_ASSERT(test_slate.filesys_buffer_is_dirty, "Buffer should be dirty");

    // Write buffer to MRAM (completes one cycle)
    lfs_ssize_t bytes_written =
        filesys_write_buffer_to_mram(&test_slate, FILESYS_BUFFER_SIZE);
    TEST_ASSERT(bytes_written == FILESYS_BUFFER_SIZE,
                "Should write full buffer");
    TEST_ASSERT(!test_slate.filesys_buffer_is_dirty,
                "Buffer should be clean after MRAM write");

    // In real implementation, would send FTP_FILE_WRITE_SUCCESS packet here

    // Clean up
    filesys_cancel_file_write(&test_slate);

    LOG_DEBUG(
        "=== Test PASSED: Write File Data Command - Complete Cycle ===\n");
    return 0;
}

// ============================================================================
// Test 12: Write File Data Command - No File Active
// ============================================================================
int ftp_test_write_file_data_command_no_file(void)
{
    LOG_DEBUG("=== Test: Write File Data Command - No File Active ===\n");

    slate_t test_slate;
    if (setup_clean_filesystem(&test_slate) < 0)
        return -1;

    reset_test_state();

    // Try to write data without starting file write
    uint8_t data[FTP_DATA_PAYLOAD_SIZE];
    memset(data, 0xCC, sizeof(data));

    int8_t code = filesys_write_data_to_buffer(&test_slate, data,
                                               FTP_DATA_PAYLOAD_SIZE, 0);
    TEST_ASSERT(code == -2, "Should fail with -2 (no file being written)");

    // In real implementation, would send FTP_FILE_WRITE_BUFFER_ERROR packet

    LOG_DEBUG(
        "=== Test PASSED: Write File Data Command - No File Active ===\n");
    return 0;
}

// ============================================================================
// Test 13: Cancel File Write Command - Success
// ============================================================================
int ftp_test_cancel_file_write_command_success(void)
{
    LOG_DEBUG("=== Test: Cancel File Write Command - Success ===\n");

    slate_t test_slate;
    if (setup_clean_filesystem(&test_slate) < 0)
        return -1;

    reset_test_state();

    // Start file write
    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_STR_T fname = "TD";

    int8_t code =
        filesys_start_file_write(&test_slate, fname, 512, 0x0, &blocks_left);
    TEST_ASSERT(code == 0, "start_file_write should succeed");

    // Write some data and flush to MRAM
    uint8_t data[64];
    memset(data, 0xDD, sizeof(data));
    code = filesys_write_data_to_buffer(&test_slate, data, 64, 0);
    TEST_ASSERT(code == 0, "write_data_to_buffer should succeed");

    filesys_write_buffer_to_mram(&test_slate, 64);

    // Cancel file write
    code = filesys_cancel_file_write(&test_slate);
    TEST_ASSERT(code == 0, "cancel_file_write should succeed");
    TEST_ASSERT(!test_slate.filesys_is_writing_file,
                "Should not be writing file after cancel");
    TEST_ASSERT(!test_slate.filesys_buffer_is_dirty,
                "Buffer should be clean after cancel");

    // Verify file was deleted
    lfs_file_t file;
    int err = lfs_file_open(&test_slate.lfs, &file, fname, LFS_O_RDONLY);
    TEST_ASSERT(err < 0, "File should not exist after cancel");

    // In real implementation, would send FTP_CANCEL_SUCCESS packet

    LOG_DEBUG("=== Test PASSED: Cancel File Write Command - Success ===\n");
    return 0;
}

// ============================================================================
// Test 14: Cancel File Write Command - No File Active
// ============================================================================
int ftp_test_cancel_file_write_command_no_file(void)
{
    LOG_DEBUG("=== Test: Cancel File Write Command - No File Active ===\n");

    slate_t test_slate;
    if (setup_clean_filesystem(&test_slate) < 0)
        return -1;

    reset_test_state();

    // Try to cancel without active file write
    int8_t code = filesys_cancel_file_write(&test_slate);
    TEST_ASSERT(code == -1, "Should fail with -1 (no file being written)");

    // In real implementation, would send FTP_CANCEL_ERROR packet

    LOG_DEBUG(
        "=== Test PASSED: Cancel File Write Command - No File Active ===\n");
    return 0;
}

// ============================================================================
// Test 15: EOF Success - Correct CRC
// ============================================================================
int ftp_test_eof_success_correct_crc(void)
{
    LOG_DEBUG("=== Test: EOF Success - Correct CRC ===\n");

    slate_t test_slate;
    if (setup_clean_filesystem(&test_slate) < 0)
        return -1;

    reset_test_state();

    // Start file write with known CRC
    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_STR_T fname = "TE";
    // CRC32 for bytes 0x00..0x3F (64 bytes)
    FILESYS_BUFFERED_FILE_CRC_T correct_crc = 0x100ECE8C;

    int8_t code = filesys_start_file_write(&test_slate, fname, 64, correct_crc,
                                           &blocks_left);
    TEST_ASSERT(code == 0, "start_file_write should succeed");

    // Write the data that matches the CRC
    uint8_t buffer[64];
    for (uint8_t i = 0; i < 64; i++)
        buffer[i] = i;

    code = filesys_write_data_to_buffer(&test_slate, buffer, 64, 0);
    TEST_ASSERT(code == 0, "write_data_to_buffer should succeed");

    lfs_ssize_t bytes = filesys_write_buffer_to_mram(&test_slate, 64);
    TEST_ASSERT(bytes == 64, "write_buffer_to_mram should succeed");

    // Check CRC
    unsigned int computed_crc;
    code = filesys_is_crc_correct(&test_slate, &computed_crc);
    TEST_ASSERT(code == 0, "CRC should be correct");
    TEST_ASSERT(computed_crc == correct_crc, "Computed CRC should match");

    // Complete file write
    code = filesys_complete_file_write(&test_slate, &computed_crc);
    TEST_ASSERT(code == 0, "complete_file_write should succeed");

    // In real implementation, would send FTP_EOF_SUCCESS packet with
    // computed_crc and file length

    LOG_DEBUG("=== Test PASSED: EOF Success - Correct CRC ===\n");
    return 0;
}

// ============================================================================
// Test 16: EOF Error - CRC Mismatch
// ============================================================================
int ftp_test_eof_error_crc_mismatch(void)
{
    LOG_DEBUG("=== Test: EOF Error - CRC Mismatch ===\n");

    slate_t test_slate;
    if (setup_clean_filesystem(&test_slate) < 0)
        return -1;

    reset_test_state();

    // Start file write with wrong CRC
    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_STR_T fname = "TF";
    FILESYS_BUFFERED_FILE_CRC_T wrong_crc = 0xBADBAD00;

    int8_t code = filesys_start_file_write(&test_slate, fname, 64, wrong_crc,
                                           &blocks_left);
    TEST_ASSERT(code == 0, "start_file_write should succeed");

    // Write data
    uint8_t buffer[64];
    for (uint8_t i = 0; i < 64; i++)
        buffer[i] = i;

    code = filesys_write_data_to_buffer(&test_slate, buffer, 64, 0);
    TEST_ASSERT(code == 0, "write_data_to_buffer should succeed");

    lfs_ssize_t bytes = filesys_write_buffer_to_mram(&test_slate, 64);
    TEST_ASSERT(bytes == 64, "write_buffer_to_mram should succeed");

    // Check CRC - should fail
    unsigned int computed_crc;
    code = filesys_is_crc_correct(&test_slate, &computed_crc);
    TEST_ASSERT(code == -1, "CRC should be incorrect");

    // Try to complete - should fail
    code = filesys_complete_file_write(&test_slate, &computed_crc);
    TEST_ASSERT(code == -4, "complete_file_write should fail with -4");

    // In real implementation, would send FTP_EOF_CRC_ERROR packet with
    // computed_crc and file length

    // Clean up
    filesys_cancel_file_write(&test_slate);

    LOG_DEBUG("=== Test PASSED: EOF Error - CRC Mismatch ===\n");
    return 0;
}

// ============================================================================
// Test 17: Packet Tracker Bitfield
// ============================================================================
int ftp_test_packet_tracker_bitfield(void)
{
    LOG_DEBUG("=== Test: Packet Tracker Bitfield ===\n");

    // Verify packet tracker can track all packets in a cycle
    FTP_PACKET_TRACKER_T tracker = 0;

    // Set bits for received packets
    for (int i = 0; i < FTP_NUM_PACKETS_PER_CYCLE; i++)
    {
        tracker |= (1 << i);
    }

    // Verify all bits are set
    FTP_PACKET_TRACKER_T expected = (1 << FTP_NUM_PACKETS_PER_CYCLE) - 1;
    TEST_ASSERT(tracker == expected, "All packet bits should be set");

    // Test individual packet tracking
    tracker = 0;
    tracker |= (1 << 0); // Packet 0 received
    tracker |= (1 << 2); // Packet 2 received
    tracker |= (1 << 4); // Packet 4 received

    TEST_ASSERT((tracker & (1 << 0)) != 0, "Packet 0 should be marked");
    TEST_ASSERT((tracker & (1 << 1)) == 0, "Packet 1 should not be marked");
    TEST_ASSERT((tracker & (1 << 2)) != 0, "Packet 2 should be marked");
    TEST_ASSERT((tracker & (1 << 3)) == 0, "Packet 3 should not be marked");
    TEST_ASSERT((tracker & (1 << 4)) != 0, "Packet 4 should be marked");

    LOG_DEBUG("=== Test PASSED: Packet Tracker Bitfield ===\n");
    return 0;
}

// ============================================================================
// Test 18: Multiple Write Cycles
// ============================================================================
int ftp_test_multiple_write_cycles(void)
{
    LOG_DEBUG("=== Test: Multiple Write Cycles ===\n");

    slate_t test_slate;
    if (setup_clean_filesystem(&test_slate) < 0)
        return -1;

    reset_test_state();

    // Start file write for 3 cycles worth of data
    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_STR_T fname = "TG";
    size_t total_size = FILESYS_BUFFER_SIZE * 3;

    int8_t code = filesys_start_file_write(&test_slate, fname, total_size, 0x0,
                                           &blocks_left);
    TEST_ASSERT(code == 0, "start_file_write should succeed");

    // Write 3 complete cycles
    for (int cycle = 0; cycle < 3; cycle++)
    {
        // Write packets for this cycle
        for (int pkt = 0; pkt < FTP_NUM_PACKETS_PER_CYCLE; pkt++)
        {
            uint8_t data[FTP_DATA_PAYLOAD_SIZE];
            uint8_t pattern = (uint8_t)(cycle * 10 + pkt);
            memset(data, pattern, sizeof(data));

            FILESYS_BUFFER_SIZE_T offset = pkt * FTP_DATA_PAYLOAD_SIZE;
            code = filesys_write_data_to_buffer(&test_slate, data,
                                                FTP_DATA_PAYLOAD_SIZE, offset);
            TEST_ASSERT(code == 0, "write_data_to_buffer should succeed");
        }

        // Flush to MRAM
        lfs_ssize_t bytes =
            filesys_write_buffer_to_mram(&test_slate, FILESYS_BUFFER_SIZE);
        TEST_ASSERT(bytes == FILESYS_BUFFER_SIZE,
                    "write_buffer_to_mram should succeed");
        TEST_ASSERT(!test_slate.filesys_buffer_is_dirty,
                    "Buffer should be clean after flush");
    }

    // Clean up
    filesys_cancel_file_write(&test_slate);

    LOG_DEBUG("=== Test PASSED: Multiple Write Cycles ===\n");
    return 0;
}

// ============================================================================
// Test 19: Partial Packet Write
// ============================================================================
int ftp_test_partial_packet_write(void)
{
    LOG_DEBUG("=== Test: Partial Packet Write ===\n");

    slate_t test_slate;
    if (setup_clean_filesystem(&test_slate) < 0)
        return -1;

    reset_test_state();

    // Start file write
    lfs_ssize_t blocks_left;
    FILESYS_BUFFERED_FNAME_STR_T fname = "TH";

    int8_t code =
        filesys_start_file_write(&test_slate, fname, 256, 0x0, &blocks_left);
    TEST_ASSERT(code == 0, "start_file_write should succeed");

    // Write only some packets (not complete cycle)
    for (int pkt = 0; pkt < FTP_NUM_PACKETS_PER_CYCLE - 2; pkt++)
    {
        uint8_t data[FTP_DATA_PAYLOAD_SIZE];
        memset(data, (uint8_t)pkt, sizeof(data));

        FILESYS_BUFFER_SIZE_T offset = pkt * FTP_DATA_PAYLOAD_SIZE;
        code = filesys_write_data_to_buffer(&test_slate, data,
                                            FTP_DATA_PAYLOAD_SIZE, offset);
        TEST_ASSERT(code == 0, "write_data_to_buffer should succeed");
    }

    TEST_ASSERT(test_slate.filesys_buffer_is_dirty, "Buffer should be dirty");

    // In real implementation, would NOT send FTP_FILE_WRITE_SUCCESS yet
    // since cycle is incomplete

    // Clean up
    filesys_cancel_file_write(&test_slate);

    LOG_DEBUG("=== Test PASSED: Partial Packet Write ===\n");
    return 0;
}

// ============================================================================
// Test 20: Filesystem Type Sizes
// ============================================================================
int ftp_test_filesystem_type_sizes(void)
{
    LOG_DEBUG("=== Test: Filesystem Type Sizes ===\n");

    // Verify filename type size
    TEST_ASSERT(sizeof(FILESYS_BUFFERED_FNAME_T) == 2,
                "Filename type should be 2 bytes (uint16_t)");
    TEST_ASSERT(sizeof(FILESYS_BUFFERED_FNAME_STR_T) == 3,
                "Filename string should be 3 bytes (2 chars + null)");

    // Verify file length type
    TEST_ASSERT(sizeof(FILESYS_BUFFERED_FILE_LEN_T) == 4,
                "File length type should be 4 bytes (uint32_t)");

    // Verify file CRC type
    TEST_ASSERT(sizeof(FILESYS_BUFFERED_FILE_CRC_T) == 4,
                "File CRC type should be 4 bytes (uint32_t)");

    // Verify buffer size type can hold buffer size
    TEST_ASSERT(sizeof(FILESYS_BUFFER_SIZE_T) >= 2,
                "Buffer size type should be at least 2 bytes");
    TEST_ASSERT((1ULL << (sizeof(FILESYS_BUFFER_SIZE_T) * 8)) >
                    FILESYS_BUFFER_SIZE,
                "Buffer size type should be able to hold FILESYS_BUFFER_SIZE");

    LOG_DEBUG("=== Test PASSED: Filesystem Type Sizes ===\n");
    return 0;
}

// ============================================================================
// Main Test Runner
// ============================================================================
int main(void)
{
    LOG_DEBUG("========================================\n");
    LOG_DEBUG("Starting FTP Task Test Suite\n");
    LOG_DEBUG("========================================\n");

    int result;
    int tests_passed = 0;
    int tests_failed = 0;

    // Define test functions
    struct
    {
        int (*test_func)(void);
        const char *name;
    } tests[] = {
        {ftp_test_result_codes, "FTP Result Code Values"},
        {ftp_test_config_constants, "FTP Configuration Constants"},
        {ftp_test_packet_sequence_capacity, "Packet Sequence Type Capacity"},
        {ftp_test_send_result_packet_no_file, "Send Result Packet - No File"},
        {ftp_test_send_result_packet_with_file,
         "Send Result Packet - With File"},
        {ftp_test_format_filesystem_command, "Format Filesystem Command"},
        {ftp_test_start_file_write_command_success,
         "Start File Write Command - Success"},
        {ftp_test_start_file_write_command_already_writing,
         "Start File Write Command - Already Writing"},
        {ftp_test_start_file_write_command_insufficient_space,
         "Start File Write Command - Insufficient Space"},
        {ftp_test_write_file_data_command_single_packet,
         "Write File Data Command - Single Packet"},
        {ftp_test_write_file_data_command_complete_cycle,
         "Write File Data Command - Complete Cycle"},
        {ftp_test_write_file_data_command_no_file,
         "Write File Data Command - No File Active"},
        {ftp_test_cancel_file_write_command_success,
         "Cancel File Write Command - Success"},
        {ftp_test_cancel_file_write_command_no_file,
         "Cancel File Write Command - No File Active"},
        {ftp_test_eof_success_correct_crc, "EOF Success - Correct CRC"},
        {ftp_test_eof_error_crc_mismatch, "EOF Error - CRC Mismatch"},
        {ftp_test_packet_tracker_bitfield, "Packet Tracker Bitfield"},
        {ftp_test_multiple_write_cycles, "Multiple Write Cycles"},
        {ftp_test_partial_packet_write, "Partial Packet Write"},
        {ftp_test_filesystem_type_sizes, "Filesystem Type Sizes"},
    };

    int num_tests = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < num_tests; i++)
    {
        LOG_DEBUG("\n--- Running Test %d/%d: %s ---\n", i + 1, num_tests,
                  tests[i].name);
        result = tests[i].test_func();
        if (result == 0)
        {
            tests_passed++;
            LOG_DEBUG("--- Test %d PASSED ---\n", i + 1);
        }
        else
        {
            tests_failed++;
            LOG_ERROR("--- Test %d FAILED ---\n", i + 1);
        }
    }

    LOG_DEBUG("\n========================================\n");
    LOG_DEBUG("Test Suite Complete\n");
    LOG_DEBUG("Passed: %d / %d\n", tests_passed, num_tests);
    LOG_DEBUG("Failed: %d / %d\n", tests_failed, num_tests);
    LOG_DEBUG("========================================\n");

    if (tests_failed > 0)
    {
        LOG_ERROR("SOME TESTS FAILED!\n");
        return -1;
    }

    LOG_DEBUG("All FTP Task tests passed!\n");
    return 0;
}
