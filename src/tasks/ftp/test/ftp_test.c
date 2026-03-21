/**
 * @author Claude (generated), reviewed by Ayush Garg
 * @date 2026-03-18
 * @brief Unit tests for the FTP task module.
 *
 * Tests FTP packet tracker helpers, command processing functions, and
 * end-to-end file transfer workflows. MRAM is mocked via mram_mock.c;
 * the filesystem (LittleFS) runs over the mock MRAM.
 *
 * Tests verify both slate state changes and the contents of packets
 * enqueued to tx_queue via ftp_test_pop_last_packet /
 * ftp_test_get_ground_info.
 */

#include "ftp_test.h"

#include "str_utils.h"

/*
 * ============================================================================
 * Test data
 * ============================================================================
 *
 * CRC values computed with Python's zlib.crc32().
 */

// 1-packet file: 205 bytes, pattern 0x00..0xCC repeating
static const uint32_t ftp_test_file_1pkt_crc = 0xe5599aee;
static const FILESYS_BUFFERED_FILE_LEN_T ftp_test_file_1pkt_len = 205;

// 3-packet file: 615 bytes, pattern 0x00..0xFF repeating
static const uint32_t ftp_test_file_3pkt_crc = 0x8e6e6a2d;
static const FILESYS_BUFFERED_FILE_LEN_T ftp_test_file_3pkt_len = 615;

// 2-packet file with partial last packet: 400 bytes
static const uint32_t ftp_test_file_partial_crc = 0xe0bbcad4;
static const FILESYS_BUFFERED_FILE_LEN_T ftp_test_file_partial_len = 400;

static const uint32_t ftp_test_file_2cycles_crc = 0x1c291cae;
static const FILESYS_BUFFERED_FILE_LEN_T ftp_test_file_2cycles_len =
    (FTP_NUM_PACKETS_PER_CYCLE + 1) * FTP_DATA_PAYLOAD_SIZE;

// Intentionally wrong CRC for mismatch tests
static const uint32_t ftp_test_wrong_crc = 0xDEADBEEF;

// Standard test filename
static const FILESYS_BUFFERED_FNAME_STR_T ftp_test_fname1 = "AB";
static const FILESYS_BUFFERED_FNAME_STR_T ftp_test_fname2 = "CD";

/*
 * ============================================================================
 * Packet inspection utilities
 * ============================================================================
 */

packet_t ftp_test_pop_last_packet(slate_t *slate)
{
    packet_t out_packet;
    LOG_DEBUG("[FTP TEST] Length of queue: %d",
              queue_get_level(&slate->tx_queue));
    bool result = queue_try_remove(&slate->tx_queue, &out_packet);
    ASSERT(result);
    return out_packet;
}

typedef struct ftp_test_ground_info
{
    FILESYS_BUFFERED_FNAME_T fname;
    FILESYS_BUFFERED_FILE_LEN_T file_len;
    FILESYS_BUFFERED_FILE_CRC_T file_crc;
    ftp_result_t ftp_result;
    uint8_t data[PACKET_DATA_SIZE - sizeof(FILESYS_BUFFERED_FNAME_T) -
                 sizeof(FILESYS_BUFFERED_FILE_LEN_T) -
                 sizeof(FILESYS_BUFFERED_FILE_CRC_T) - sizeof(ftp_result_t)];
} __attribute__((packed)) ftp_test_ground_info_t;

ftp_test_ground_info_t ftp_test_get_ground_info(slate_t *slate, packet_t pkt)
{
    ftp_test_ground_info_t ground_info;
    memcpy(&ground_info, pkt.data, sizeof(ground_info));
    return ground_info;
}

/**
 * Parsed additional data for cycle-info packets
 * (FTP_READY_RECEIVE, FTP_FILE_WRITE_SUCCESS, FTP_ERROR_PACKET_OUT_OF_RANGE).
 */
typedef struct ftp_test_cycle_info
{
    FTP_PACKET_SEQUENCE_T packet_start;
    FTP_PACKET_SEQUENCE_T packet_end;
    FTP_PACKET_TRACKER_T tracker;
} __attribute__((packed)) ftp_test_cycle_info_t;

static ftp_test_cycle_info_t
ftp_test_parse_cycle_info(const ftp_test_ground_info_t *info)
{
    ftp_test_cycle_info_t cycle;
    memcpy(&cycle, info->data, sizeof(cycle));
    return cycle;
}

/**
 * Parsed additional data for EOF packets (FTP_EOF_SUCCESS, FTP_EOF_CRC_ERROR).
 */
typedef struct ftp_test_eof_info
{
    FILESYS_BUFFERED_FILE_CRC_T computed_crc;
    FILESYS_BUFFERED_FILE_LEN_T file_len_on_disk;
} __attribute__((packed)) ftp_test_eof_info_t;

static ftp_test_eof_info_t
ftp_test_parse_eof_info(const ftp_test_ground_info_t *info)
{
    ftp_test_eof_info_t eof;
    memcpy(&eof, info->data, sizeof(eof));
    return eof;
}

/**
 * Drain all packets from the TX queue, discarding them.
 */
static void ftp_test_drain_queue(slate_t *slate)
{
    packet_t pkt;
    while (queue_try_remove(&slate->tx_queue, &pkt))
        ;
}

/*
 * ============================================================================
 * Helpers
 * ============================================================================
 */

/**
 * Construct an FTP_WRITE_TO_FILE_DATA for a given packet_id, filling data
 * with the correct portion of a repeating 0x00..0xFF pattern based on the
 * packet's position in the file.
 *
 * @param packet_id  The packet sequence ID.
 * @param file_len   Total file length (used to compute data_len for last pkt).
 * @param out        Output command data struct.
 */
static void ftp_test_make_write_data(FTP_PACKET_SEQUENCE_T packet_id,
                                     FILESYS_BUFFERED_FNAME_STR_T fname,
                                     FILESYS_BUFFERED_FILE_LEN_T file_len,
                                     FTP_WRITE_TO_FILE_DATA *out)
{
    memcpy(out->fname_str, fname, sizeof(out->fname_str));
    out->packet_id = packet_id;

    // Calculate how many bytes this packet carries
    const uint32_t byte_offset = (uint32_t)packet_id * FTP_DATA_PAYLOAD_SIZE;
    const uint32_t remaining = file_len - byte_offset;
    out->data_len = (remaining < FTP_DATA_PAYLOAD_SIZE)
                        ? (PACKET_SIZE_T)remaining
                        : (PACKET_SIZE_T)FTP_DATA_PAYLOAD_SIZE;

    // Fill data with pattern matching the file offset
    for (uint16_t i = 0; i < out->data_len; i++)
        out->data[i] = (uint8_t)((byte_offset + i) % 256);
}

/**
 * Construct an FTP_START_FILE_WRITE_DATA.
 */
static FTP_START_FILE_WRITE_DATA
ftp_test_make_start_data(FILESYS_BUFFERED_FNAME_STR_T fname,
                         FILESYS_BUFFERED_FILE_LEN_T file_len,
                         FILESYS_BUFFERED_FILE_CRC_T file_crc)
{
    FTP_START_FILE_WRITE_DATA data;
    memcpy(data.fname_str, fname, sizeof(data.fname_str));
    data.file_len = file_len;
    data.file_crc = file_crc;
    return data;
}

/**
 * Construct an FTP_CANCEL_FILE_WRITE_DATA.
 */
static FTP_CANCEL_FILE_WRITE_DATA
ftp_test_make_cancel_data(FILESYS_BUFFERED_FNAME_STR_T fname)
{
    FTP_CANCEL_FILE_WRITE_DATA data;
    memcpy(data.fname_str, fname, sizeof(data.fname_str));
    return data;
}

/**
 * Initialize a clean slate and reformat the filesystem for testing.
 */
int ftp_test_setup(slate_t *slate)
{
    TEST_ASSERT(clear_and_init_slate(slate) == 0,
                "Failed to initialize slate for FTP test setup!");

    // Initialize tx_queue so FTP can enqueue result packets
    queue_init(&slate->tx_queue, sizeof(packet_t), 16);

    lfs_ssize_t lfs_err;
    filesys_error_t res = filesys_reformat_initialize(slate, &lfs_err);
    TEST_ASSERT(res == FILESYS_OK,
                "Failed to reformat filesystem for FTP test setup: %d (LFS: "
                "%d)",
                res, lfs_err);

    // Clear FTP-specific state
    ftp_tracker_clear(&slate->ftp_packets_received_tracker);
    slate->ftp_start_cycle_packet_id = 0;

    return 0;
}

/**
 * Helper: start a file write via filesys + FTP and verify success.
 * Sets up slate as if ftp_process_file_start_write_command succeeded.
 */
static int ftp_test_helper_start_file_write(
    slate_t *slate, FILESYS_BUFFERED_FNAME_STR_T fname,
    FILESYS_BUFFERED_FILE_LEN_T file_len, FILESYS_BUFFERED_FILE_CRC_T file_crc)
{
    FTP_START_FILE_WRITE_DATA start_data =
        ftp_test_make_start_data(fname, file_len, file_crc);
    ftp_process_file_start_write_command(slate, start_data);

    TEST_ASSERT(slate->filesys_is_writing_file,
                "Expected filesys_is_writing_file to be true after start");

    // Drain and verify the FTP_READY_RECEIVE packet
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_READY_RECEIVE,
                "Expected FTP_READY_RECEIVE after start, got %d",
                info.ftp_result);

    return 0;
}

/**
 * Helper: send all packets for a single cycle starting at the current
 * ftp_start_cycle_packet_id.
 *
 * @param num_packets Number of packets to send in this cycle.
 * @param file_len    Total file length.
 */
static int
ftp_test_helper_send_cycle_packets(slate_t *slate, uint16_t num_packets,
                                   FILESYS_BUFFERED_FNAME_STR_T fname,
                                   FILESYS_BUFFERED_FILE_LEN_T file_len)
{
    const FTP_PACKET_SEQUENCE_T base = slate->ftp_start_cycle_packet_id;
    for (uint16_t i = 0; i < num_packets; i++)
    {
        FTP_WRITE_TO_FILE_DATA wd;
        ftp_test_make_write_data(base + i, fname, file_len, &wd);
        ftp_process_file_write_data_command(slate, wd);
    }
    return 0;
}

/*
 * ============================================================================
 * A. Packet Tracker Tests
 * ============================================================================
 */

int ftp_test_tracker_clear(slate_t *slate)
{
    LOG_DEBUG("=== Test: Tracker Clear ===");

    FTP_PACKET_TRACKER_T tracker;
    // Dirty the tracker first
    memset(tracker.bytes, 0xFF, FTP_PACKET_TRACKER_SIZE);

    ftp_tracker_clear(&tracker);

    for (uint16_t i = 0; i < FTP_PACKET_TRACKER_SIZE; i++)
    {
        TEST_ASSERT(tracker.bytes[i] == 0,
                    "Tracker byte %u should be 0 after clear, got 0x%02X", i,
                    tracker.bytes[i]);
    }

    return 0;
}

int ftp_test_tracker_set_and_check(slate_t *slate)
{
    LOG_DEBUG("=== Test: Tracker Set and Check ===");

    FTP_PACKET_TRACKER_T tracker;
    ftp_tracker_clear(&tracker);

    // Set a few specific bits
    ftp_tracker_set_bit(&tracker, 0);
    ftp_tracker_set_bit(&tracker, 7);
    ftp_tracker_set_bit(&tracker, 8);
    ftp_tracker_set_bit(&tracker, 255);

    // Verify set bits
    TEST_ASSERT(ftp_tracker_check_bit(&tracker, 0), "Bit 0 should be set");
    TEST_ASSERT(ftp_tracker_check_bit(&tracker, 7), "Bit 7 should be set");
    TEST_ASSERT(ftp_tracker_check_bit(&tracker, 8), "Bit 8 should be set");
    TEST_ASSERT(ftp_tracker_check_bit(&tracker, 255), "Bit 255 should be set");

    // Verify unset bits
    TEST_ASSERT(!ftp_tracker_check_bit(&tracker, 1), "Bit 1 should NOT be set");
    TEST_ASSERT(!ftp_tracker_check_bit(&tracker, 6), "Bit 6 should NOT be set");
    TEST_ASSERT(!ftp_tracker_check_bit(&tracker, 9), "Bit 9 should NOT be set");
    TEST_ASSERT(!ftp_tracker_check_bit(&tracker, 254),
                "Bit 254 should NOT be set");

    return 0;
}

int ftp_test_tracker_check_mask_full_bytes(slate_t *slate)
{
    LOG_DEBUG("=== Test: Tracker Check Mask - Full Bytes ===");

    FTP_PACKET_TRACKER_T tracker;
    ftp_tracker_clear(&tracker);

    // Set first 16 bits (2 full bytes)
    for (uint16_t i = 0; i < 16; i++)
        ftp_tracker_set_bit(&tracker, i);

    TEST_ASSERT(ftp_tracker_check_mask_completed(&tracker, 16),
                "Mask should be complete for 16 bits");
    TEST_ASSERT(ftp_tracker_check_mask_completed(&tracker, 8),
                "Mask should be complete for first 8 bits");
    TEST_ASSERT(!ftp_tracker_check_mask_completed(&tracker, 17),
                "Mask should NOT be complete for 17 bits (bit 16 unset)");

    return 0;
}

int ftp_test_tracker_check_mask_partial_byte(slate_t *slate)
{
    LOG_DEBUG("=== Test: Tracker Check Mask - Partial Byte ===");

    FTP_PACKET_TRACKER_T tracker;
    ftp_tracker_clear(&tracker);

    // Set first 11 bits (1 full byte + 3 bits)
    for (uint16_t i = 0; i < 11; i++)
        ftp_tracker_set_bit(&tracker, i);

    TEST_ASSERT(ftp_tracker_check_mask_completed(&tracker, 11),
                "Mask should be complete for 11 bits");
    TEST_ASSERT(ftp_tracker_check_mask_completed(&tracker, 10),
                "Mask should be complete for 10 bits (subset)");
    TEST_ASSERT(!ftp_tracker_check_mask_completed(&tracker, 12),
                "Mask should NOT be complete for 12 bits");

    return 0;
}

int ftp_test_tracker_check_mask_incomplete(slate_t *slate)
{
    LOG_DEBUG("=== Test: Tracker Check Mask - Incomplete ===");

    FTP_PACKET_TRACKER_T tracker;
    ftp_tracker_clear(&tracker);

    // Set all bits except bit 5
    for (uint16_t i = 0; i < 16; i++)
    {
        if (i != 5)
            ftp_tracker_set_bit(&tracker, i);
    }

    TEST_ASSERT(!ftp_tracker_check_mask_completed(&tracker, 16),
                "Mask should NOT be complete with bit 5 missing");
    TEST_ASSERT(ftp_tracker_check_mask_completed(&tracker, 5),
                "Mask should be complete for first 5 bits (before gap)");

    return 0;
}

/*
 * ============================================================================
 * B. Reformat Tests
 * ============================================================================
 *
 * Expected packet behavior (verified once capturing queue mock is available):
 * - ftp_process_reformat_command sends FILESYS_REFORMAT_SUCCESS on success
 *   with fname="XX", file_len=0, file_crc=0, no additional data.
 * - On failure, sends FILESYS_REFORMAT_ERROR with filesys_error + lfs_error.
 */

int ftp_test_reformat_success(slate_t *slate)
{
    LOG_DEBUG("=== Test: Reformat Success ===");

    // Reformat should succeed on a clean filesystem
    ftp_process_reformat_command(slate);

    // Verify packet: FILESYS_REFORMAT_SUCCESS, fname="XX", len=0, crc=0
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FILESYS_REFORMAT_SUCCESS,
                "Expected FILESYS_REFORMAT_SUCCESS, got %d", info.ftp_result);
    TEST_ASSERT(info.fname == string_to_file("XX"),
                "Expected fname 'XX', got 0x%04X", info.fname);
    TEST_ASSERT(info.file_len == 0, "Expected file_len 0, got %u",
                info.file_len);
    TEST_ASSERT(info.file_crc == 0, "Expected file_crc 0, got 0x%08X",
                info.file_crc);

    return 0;
}

int ftp_test_reformat_clears_state(slate_t *slate)
{
    LOG_DEBUG("=== Test: Reformat Clears State ===");

    // Dirty the FTP state
    ftp_tracker_set_bit(&slate->ftp_packets_received_tracker, 42);
    slate->ftp_start_cycle_packet_id = 999;

    ftp_process_reformat_command(slate);

    // Verify state is cleared
    TEST_ASSERT(slate->ftp_start_cycle_packet_id == 0,
                "ftp_start_cycle_packet_id should be 0 after reformat, got %u",
                slate->ftp_start_cycle_packet_id);

    TEST_ASSERT(
        !ftp_tracker_check_bit(&slate->ftp_packets_received_tracker, 42),
        "Tracker bit 42 should be cleared after reformat");

    for (uint16_t i = 0; i < FTP_PACKET_TRACKER_SIZE; i++)
    {
        TEST_ASSERT(slate->ftp_packets_received_tracker.bytes[i] == 0,
                    "Tracker byte %u should be 0 after reformat", i);
    }

    // Verify packet
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FILESYS_REFORMAT_SUCCESS,
                "Expected FILESYS_REFORMAT_SUCCESS, got %d", info.ftp_result);

    return 0;
}

/*
 * ============================================================================
 * C. Start File Write Tests
 * ============================================================================
 *
 * Expected packet behavior:
 * - On success: FTP_READY_RECEIVE with packet_start=0,
 *   packet_end=FTP_NUM_PACKETS_PER_CYCLE-1, empty tracker bitfield.
 *   Headers: fname from command, file_len from command, file_crc from command.
 * - If already writing: FTP_ERROR_ALREADY_WRITING_FILE with current file's
 *   fname_str as additional data.
 * - On filesys error: FTP_ERROR_START_FILE_WRITE with filesys_error +
 *   lfs_error + blocks_left.
 */

int ftp_test_start_write_success(slate_t *slate)
{
    LOG_DEBUG("=== Test: Start Write Success ===");

    FTP_START_FILE_WRITE_DATA data = ftp_test_make_start_data(
        ftp_test_fname1, ftp_test_file_1pkt_len, ftp_test_file_1pkt_crc);

    ftp_process_file_start_write_command(slate, data);

    TEST_ASSERT(slate->filesys_is_writing_file,
                "filesys_is_writing_file should be true after start write");

    // Verify packet: FTP_READY_RECEIVE with file headers and cycle info
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_READY_RECEIVE,
                "Expected FTP_READY_RECEIVE, got %d", info.ftp_result);
    TEST_ASSERT(info.fname == string_to_file(ftp_test_fname1),
                "Expected fname 'AB'");
    TEST_ASSERT(info.file_len == ftp_test_file_1pkt_len,
                "Expected file_len %u, got %u", ftp_test_file_1pkt_len,
                info.file_len);
    TEST_ASSERT(info.file_crc == ftp_test_file_1pkt_crc,
                "Expected file_crc 0x%08X, got 0x%08X", ftp_test_file_1pkt_crc,
                info.file_crc);

    ftp_test_cycle_info_t cycle = ftp_test_parse_cycle_info(&info);
    TEST_ASSERT(cycle.packet_start == 0, "Expected packet_start 0, got %u",
                cycle.packet_start);
    TEST_ASSERT(cycle.packet_end == FTP_NUM_PACKETS_PER_CYCLE - 1,
                "Expected packet_end %u, got %u", FTP_NUM_PACKETS_PER_CYCLE - 1,
                cycle.packet_end);

    return 0;
}

int ftp_test_start_write_already_writing(slate_t *slate)
{
    LOG_DEBUG("=== Test: Start Write - Already Writing ===");

    // Start first file
    FTP_START_FILE_WRITE_DATA data1 = ftp_test_make_start_data(
        ftp_test_fname1, ftp_test_file_1pkt_len, ftp_test_file_1pkt_crc);
    ftp_process_file_start_write_command(slate, data1);

    TEST_ASSERT(slate->filesys_is_writing_file,
                "First file write should be in progress");

    // Attempt to start second file while first is in progress
    FTP_START_FILE_WRITE_DATA data2 = ftp_test_make_start_data(
        ftp_test_fname2, ftp_test_file_3pkt_len, ftp_test_file_3pkt_crc);

    ftp_process_file_start_write_command(slate, data2);

    // Should still be writing the first file
    TEST_ASSERT(slate->filesys_is_writing_file,
                "Should still be writing first file");
    TEST_ASSERT(memcmp(slate->filesys_buffered_fname_str, ftp_test_fname1,
                       sizeof(FILESYS_BUFFERED_FNAME_STR_T)) == 0,
                "Filename should still be original file 'AB', not 'CD'");

    // Drain the first READY_RECEIVE from starting file 1
    ftp_test_pop_last_packet(slate);

    // Verify packet: FTP_ERROR_ALREADY_WRITING_FILE with first file's headers
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_ERROR_ALREADY_WRITING_FILE,
                "Expected FTP_ERROR_ALREADY_WRITING_FILE, got %d",
                info.ftp_result);
    TEST_ASSERT(info.fname == string_to_file(ftp_test_fname1),
                "Error packet should have first file's fname 'AB'");
    TEST_ASSERT(info.file_len == ftp_test_file_1pkt_len,
                "Error packet should have first file's len");
    TEST_ASSERT(info.file_crc == ftp_test_file_1pkt_crc,
                "Error packet should have first file's crc");

    return 0;
}

int ftp_test_start_write_sets_slate_state(slate_t *slate)
{
    LOG_DEBUG("=== Test: Start Write Sets Slate State ===");

    FTP_START_FILE_WRITE_DATA data = ftp_test_make_start_data(
        ftp_test_fname1, ftp_test_file_3pkt_len, ftp_test_file_3pkt_crc);

    ftp_process_file_start_write_command(slate, data);

    TEST_ASSERT(slate->filesys_is_writing_file,
                "filesys_is_writing_file should be true");

    TEST_ASSERT(memcmp(slate->filesys_buffered_fname_str, ftp_test_fname1,
                       sizeof(FILESYS_BUFFERED_FNAME_STR_T)) == 0,
                "Buffered filename should match command data");

    TEST_ASSERT(slate->filesys_buffered_file_len == ftp_test_file_3pkt_len,
                "Buffered file length should be %u, got %u",
                ftp_test_file_3pkt_len, slate->filesys_buffered_file_len);

    TEST_ASSERT(slate->filesys_buffered_file_crc == ftp_test_file_3pkt_crc,
                "Buffered file CRC should be 0x%08X, got 0x%08X",
                ftp_test_file_3pkt_crc, slate->filesys_buffered_file_crc);

    TEST_ASSERT(slate->ftp_start_cycle_packet_id == 0,
                "Start cycle packet ID should be 0, got %u",
                slate->ftp_start_cycle_packet_id);

    // Verify tracker is cleared
    for (uint16_t i = 0; i < FTP_PACKET_TRACKER_SIZE; i++)
    {
        TEST_ASSERT(slate->ftp_packets_received_tracker.bytes[i] == 0,
                    "Tracker byte %u should be 0 after start write", i);
    }

    // Verify packet: FTP_READY_RECEIVE with correct headers
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_READY_RECEIVE,
                "Expected FTP_READY_RECEIVE, got %d", info.ftp_result);
    TEST_ASSERT(info.fname == string_to_file(ftp_test_fname1),
                "Expected fname 'AB'");
    TEST_ASSERT(info.file_len == ftp_test_file_3pkt_len,
                "Expected file_len %u, got %u", ftp_test_file_3pkt_len,
                info.file_len);
    TEST_ASSERT(info.file_crc == ftp_test_file_3pkt_crc,
                "Expected file_crc 0x%08X, got 0x%08X", ftp_test_file_3pkt_crc,
                info.file_crc);

    ftp_test_cycle_info_t cycle = ftp_test_parse_cycle_info(&info);
    TEST_ASSERT(cycle.packet_start == 0, "Expected packet_start 0, got %u",
                cycle.packet_start);

    return 0;
}

/*
 * ============================================================================
 * D. Write File Data Tests
 * ============================================================================
 *
 * Expected packet behavior per case:
 *
 * - No file writing: FTP_ERROR_NOT_WRITING_FILE, fname="XX"
 * - Single packet file (cycle complete + final): FTP_EOF_SUCCESS with
 *   computed CRC and file length on disk
 * - Mid-cycle: No packet sent (periodic status reports only)
 * - Cycle complete (not final): FTP_FILE_WRITE_SUCCESS with new
 *   packet_start, packet_end, empty tracker
 * - Out of range: FTP_ERROR_PACKET_OUT_OF_RANGE with current range + tracker
 * - Duplicate: Silently ignored, no packet
 */

int ftp_test_write_data_no_file(slate_t *slate)
{
    LOG_DEBUG("=== Test: Write Data - No File ===");

    // Ensure no file is being written
    TEST_ASSERT(!slate->filesys_is_writing_file,
                "No file should be writing at start");

    FTP_WRITE_TO_FILE_DATA wd;
    ftp_test_make_write_data(0, ftp_test_fname1, ftp_test_file_1pkt_len, &wd);

    ftp_process_file_write_data_command(slate, wd);

    // Verify packet: FTP_ERROR_NOT_WRITING_FILE, fname="XX", len=0, crc=0
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_ERROR_NOT_WRITING_FILE,
                "Expected FTP_ERROR_NOT_WRITING_FILE, got %d", info.ftp_result);
    TEST_ASSERT(info.fname == string_to_file("XX"),
                "Expected fname 'XX', got 0x%04X", info.fname);
    TEST_ASSERT(info.file_len == 0, "Expected file_len 0, got %u",
                info.file_len);
    TEST_ASSERT(info.file_crc == 0, "Expected file_crc 0, got 0x%08X",
                info.file_crc);

    return 0;
}

int ftp_test_write_data_single_packet_file(slate_t *slate)
{
    LOG_DEBUG("=== Test: Write Data - Single Packet File (EOF) ===");

    // Start a 1-packet file (205 bytes)
    int rc = ftp_test_helper_start_file_write(
        slate, ftp_test_fname1, ftp_test_file_1pkt_len, ftp_test_file_1pkt_crc);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    // Send the single packet (completes cycle and file)
    FTP_WRITE_TO_FILE_DATA wd;
    ftp_test_make_write_data(0, ftp_test_fname1, ftp_test_file_1pkt_len, &wd);
    ftp_process_file_write_data_command(slate, wd);

    // After EOF, file write should be complete
    TEST_ASSERT(!slate->filesys_is_writing_file,
                "File write should be complete after single packet EOF");

    // Verify packet: FTP_EOF_SUCCESS with correct CRC and file length.
    // Note: EOF is sent after filesys_complete_file_write clears slate state,
    // so headers use the "no file" defaults (fname="XX", len=0, crc=0).
    // The actual file info is in additional data.
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_EOF_SUCCESS,
                "Expected FTP_EOF_SUCCESS, got %d", info.ftp_result);

    ftp_test_eof_info_t eof = ftp_test_parse_eof_info(&info);
    TEST_ASSERT(eof.computed_crc == ftp_test_file_1pkt_crc,
                "Expected computed CRC 0x%08X, got 0x%08X",
                ftp_test_file_1pkt_crc, eof.computed_crc);
    TEST_ASSERT(eof.file_len_on_disk == ftp_test_file_1pkt_len,
                "Expected file_len_on_disk %u, got %u", ftp_test_file_1pkt_len,
                eof.file_len_on_disk);

    return 0;
}

int ftp_test_write_data_mid_cycle_no_response(slate_t *slate)
{
    LOG_DEBUG("=== Test: Write Data - Mid Cycle No Response ===");

    // Start a 3-packet file
    int rc = ftp_test_helper_start_file_write(
        slate, ftp_test_fname1, ftp_test_file_3pkt_len, ftp_test_file_3pkt_crc);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    // Send only packet 0 of 3
    FTP_WRITE_TO_FILE_DATA wd;
    ftp_test_make_write_data(0, ftp_test_fname1, ftp_test_file_3pkt_len, &wd);
    ftp_process_file_write_data_command(slate, wd);

    // File should still be writing
    TEST_ASSERT(slate->filesys_is_writing_file,
                "File write should still be in progress");

    // Verify tracker has bit 0 set
    TEST_ASSERT(ftp_tracker_check_bit(&slate->ftp_packets_received_tracker, 0),
                "Tracker bit 0 should be set after receiving packet 0");
    TEST_ASSERT(!ftp_tracker_check_bit(&slate->ftp_packets_received_tracker, 1),
                "Tracker bit 1 should NOT be set");
    TEST_ASSERT(!ftp_tracker_check_bit(&slate->ftp_packets_received_tracker, 2),
                "Tracker bit 2 should NOT be set");

    // No packet is sent mid-cycle; verify queue is empty
    TEST_ASSERT(queue_is_empty(&slate->tx_queue),
                "No packet should be enqueued mid-cycle");

    return 0;
}

int ftp_test_write_data_complete_cycle_not_final(slate_t *slate)
{
    LOG_DEBUG("=== Test: Write Data - Complete Cycle (Not Final) ===");

    // We don't care about CRC for this test (won't reach EOF)
    int rc = ftp_test_helper_start_file_write(
        slate, ftp_test_fname1, ftp_test_file_2cycles_len, ftp_test_wrong_crc);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    TEST_ASSERT(slate->ftp_start_cycle_packet_id == 0,
                "Should start at packet 0");

    // Send all 256 packets for the first cycle
    ftp_test_helper_send_cycle_packets(slate, FTP_NUM_PACKETS_PER_CYCLE,
                                       ftp_test_fname1,
                                       ftp_test_file_2cycles_len);

    // After completing first cycle, should advance to next cycle
    TEST_ASSERT(slate->ftp_start_cycle_packet_id == FTP_NUM_PACKETS_PER_CYCLE,
                "Should advance to packet %u after first cycle, got %u",
                FTP_NUM_PACKETS_PER_CYCLE, slate->ftp_start_cycle_packet_id);

    // Tracker should be cleared for new cycle
    for (uint16_t i = 0; i < FTP_PACKET_TRACKER_SIZE; i++)
    {
        TEST_ASSERT(slate->ftp_packets_received_tracker.bytes[i] == 0,
                    "Tracker byte %u should be 0 after cycle completion", i);
    }

    // File should still be writing (not final cycle)
    TEST_ASSERT(slate->filesys_is_writing_file,
                "File write should still be in progress after non-final cycle");

    // Verify packet: FTP_FILE_WRITE_SUCCESS with new cycle range
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_FILE_WRITE_SUCCESS,
                "Expected FTP_FILE_WRITE_SUCCESS, got %d", info.ftp_result);

    ftp_test_cycle_info_t cycle = ftp_test_parse_cycle_info(&info);
    TEST_ASSERT(cycle.packet_start == FTP_NUM_PACKETS_PER_CYCLE,
                "Expected packet_start %u, got %u", FTP_NUM_PACKETS_PER_CYCLE,
                cycle.packet_start);
    TEST_ASSERT(cycle.packet_end == 2 * FTP_NUM_PACKETS_PER_CYCLE - 1,
                "Expected packet_end %u, got %u",
                2 * FTP_NUM_PACKETS_PER_CYCLE - 1, cycle.packet_end);

    // Tracker should be empty in the new cycle
    for (uint16_t i = 0; i < FTP_PACKET_TRACKER_SIZE; i++)
    {
        TEST_ASSERT(cycle.tracker.bytes[i] == 0,
                    "Tracker byte %u should be 0 in cycle info", i);
    }

    return 0;
}

int ftp_test_write_data_complete_final_cycle(slate_t *slate)
{
    LOG_DEBUG("=== Test: Write Data - Complete Final Cycle (EOF) ===");

    // 3-packet file: all in one cycle
    int rc = ftp_test_helper_start_file_write(
        slate, ftp_test_fname1, ftp_test_file_3pkt_len, ftp_test_file_3pkt_crc);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    // Send all 3 packets
    ftp_test_helper_send_cycle_packets(slate, 3, ftp_test_fname1,
                                       ftp_test_file_3pkt_len);

    // Should reach EOF
    TEST_ASSERT(!slate->filesys_is_writing_file,
                "File write should be complete after final cycle");

    // Verify packet: FTP_EOF_SUCCESS
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_EOF_SUCCESS,
                "Expected FTP_EOF_SUCCESS, got %d", info.ftp_result);

    ftp_test_eof_info_t eof = ftp_test_parse_eof_info(&info);
    TEST_ASSERT(eof.file_len_on_disk == ftp_test_file_3pkt_len,
                "Expected file_len_on_disk %u, got %u", ftp_test_file_3pkt_len,
                eof.file_len_on_disk);

    return 0;
}

int ftp_test_write_data_out_of_range_too_low(slate_t *slate)
{
    LOG_DEBUG("=== Test: Write Data - Out of Range (Too Low) ===");

    // Start a multi-cycle file and complete first cycle to advance packet_id
    const uint32_t file_len =
        (uint32_t)(FTP_NUM_PACKETS_PER_CYCLE + 1) * FTP_DATA_PAYLOAD_SIZE;
    int rc = ftp_test_helper_start_file_write(slate, ftp_test_fname1, file_len,
                                              0x00000000);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    // Complete first cycle
    ftp_test_helper_send_cycle_packets(slate, FTP_NUM_PACKETS_PER_CYCLE,
                                       ftp_test_fname1, file_len);

    TEST_ASSERT(slate->ftp_start_cycle_packet_id == FTP_NUM_PACKETS_PER_CYCLE,
                "Should be on second cycle");

    // Send a packet from the previous cycle (packet 0, which is out of range)
    FTP_WRITE_TO_FILE_DATA wd;
    ftp_test_make_write_data(0, ftp_test_fname1, file_len, &wd);
    ftp_process_file_write_data_command(slate, wd);

    // Tracker should NOT have any bit set for this out-of-range packet
    TEST_ASSERT(!ftp_tracker_check_bit(&slate->ftp_packets_received_tracker, 0),
                "Tracker bit 0 should NOT be set for out-of-range packet");

    // Drain the FTP_FILE_WRITE_SUCCESS from completing cycle 1
    ftp_test_pop_last_packet(slate);

    // Verify packet: FTP_ERROR_PACKET_OUT_OF_RANGE with second cycle range
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_ERROR_PACKET_OUT_OF_RANGE,
                "Expected FTP_ERROR_PACKET_OUT_OF_RANGE, got %d",
                info.ftp_result);

    ftp_test_cycle_info_t cycle = ftp_test_parse_cycle_info(&info);
    TEST_ASSERT(cycle.packet_start == FTP_NUM_PACKETS_PER_CYCLE,
                "Expected packet_start %u, got %u", FTP_NUM_PACKETS_PER_CYCLE,
                cycle.packet_start);

    return 0;
}

int ftp_test_write_data_out_of_range_too_high(slate_t *slate)
{
    LOG_DEBUG("=== Test: Write Data - Out of Range (Too High) ===");

    // Start a 3-packet file
    int rc = ftp_test_helper_start_file_write(
        slate, ftp_test_fname1, ftp_test_file_3pkt_len, ftp_test_file_3pkt_crc);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    // Send a packet beyond the current cycle (packet 256 when cycle is 0..255)
    FTP_WRITE_TO_FILE_DATA wd;
    ftp_test_make_write_data(FTP_NUM_PACKETS_PER_CYCLE, ftp_test_fname1,
                             ftp_test_file_3pkt_len, &wd);
    ftp_process_file_write_data_command(slate, wd);

    // State should be unchanged
    TEST_ASSERT(slate->ftp_start_cycle_packet_id == 0,
                "Cycle should not advance on out-of-range packet");

    // Verify packet: FTP_ERROR_PACKET_OUT_OF_RANGE with first cycle range
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_ERROR_PACKET_OUT_OF_RANGE,
                "Expected FTP_ERROR_PACKET_OUT_OF_RANGE, got %d",
                info.ftp_result);

    ftp_test_cycle_info_t cycle = ftp_test_parse_cycle_info(&info);
    TEST_ASSERT(cycle.packet_start == 0, "Expected packet_start 0, got %u",
                cycle.packet_start);
    TEST_ASSERT(cycle.packet_end == FTP_NUM_PACKETS_PER_CYCLE - 1,
                "Expected packet_end %u, got %u", FTP_NUM_PACKETS_PER_CYCLE - 1,
                cycle.packet_end);

    return 0;
}

int ftp_test_write_data_duplicate_ignored(slate_t *slate)
{
    LOG_DEBUG("=== Test: Write Data - Duplicate Ignored ===");

    // Start a 3-packet file
    int rc = ftp_test_helper_start_file_write(
        slate, ftp_test_fname1, ftp_test_file_3pkt_len, ftp_test_file_3pkt_crc);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    // Send packet 0
    FTP_WRITE_TO_FILE_DATA wd;
    ftp_test_make_write_data(0, ftp_test_fname1, ftp_test_file_3pkt_len, &wd);
    ftp_process_file_write_data_command(slate, wd);

    TEST_ASSERT(ftp_tracker_check_bit(&slate->ftp_packets_received_tracker, 0),
                "Tracker bit 0 should be set after first send");

    // Send packet 0 again (duplicate)
    ftp_process_file_write_data_command(slate, wd);

    // Bit should still be set, no error, no crash
    TEST_ASSERT(ftp_tracker_check_bit(&slate->ftp_packets_received_tracker, 0),
                "Tracker bit 0 should still be set after duplicate");

    // File should still be in progress (not accidentally completed)
    TEST_ASSERT(slate->filesys_is_writing_file,
                "File write should still be in progress");

    // No packet is sent for duplicates; verify queue is empty
    TEST_ASSERT(queue_is_empty(&slate->tx_queue),
                "No packet should be enqueued for duplicate");

    return 0;
}

int ftp_test_write_data_out_of_order(slate_t *slate)
{
    LOG_DEBUG("=== Test: Write Data - Out of Order ===");

    // Start a 3-packet file
    int rc = ftp_test_helper_start_file_write(
        slate, ftp_test_fname1, ftp_test_file_3pkt_len, ftp_test_file_3pkt_crc);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    // Send packets in reverse order: 2, 1, 0
    for (int i = 2; i >= 0; i--)
    {
        FTP_WRITE_TO_FILE_DATA wd;
        ftp_test_make_write_data((FTP_PACKET_SEQUENCE_T)i, ftp_test_fname1,
                                 ftp_test_file_3pkt_len, &wd);
        ftp_process_file_write_data_command(slate, wd);
    }

    // All 3 packets received -> cycle complete -> EOF
    TEST_ASSERT(!slate->filesys_is_writing_file,
                "File write should be complete after all packets (out of "
                "order)");

    // Verify packet: FTP_EOF_SUCCESS
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_EOF_SUCCESS,
                "Expected FTP_EOF_SUCCESS, got %d", info.ftp_result);

    ftp_test_eof_info_t eof = ftp_test_parse_eof_info(&info);
    TEST_ASSERT(eof.file_len_on_disk == ftp_test_file_3pkt_len,
                "Expected file_len_on_disk %u, got %u", ftp_test_file_3pkt_len,
                eof.file_len_on_disk);

    return 0;
}

int ftp_test_write_data_last_packet_partial_size(slate_t *slate)
{
    LOG_DEBUG("=== Test: Write Data - Last Packet Partial Size ===");

    // File of 400 bytes: 2 packets, second has 400 - 205 = 195 bytes
    int rc = ftp_test_helper_start_file_write(slate, ftp_test_fname1,
                                              ftp_test_file_partial_len,
                                              ftp_test_file_partial_crc);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    // Send packet 0 (full 205 bytes)
    FTP_WRITE_TO_FILE_DATA wd0;
    ftp_test_make_write_data(0, ftp_test_fname1, ftp_test_file_partial_len,
                             &wd0);
    TEST_ASSERT(wd0.data_len == FTP_DATA_PAYLOAD_SIZE,
                "First packet should be full size (%u), got %u",
                FTP_DATA_PAYLOAD_SIZE, wd0.data_len);
    ftp_process_file_write_data_command(slate, wd0);

    // Send packet 1 (195 bytes - partial)
    FTP_WRITE_TO_FILE_DATA wd1;
    ftp_test_make_write_data(1, ftp_test_fname1, ftp_test_file_partial_len,
                             &wd1);
    TEST_ASSERT(wd1.data_len == 195, "Last packet should be 195 bytes, got %u",
                wd1.data_len);
    ftp_process_file_write_data_command(slate, wd1);

    // Should complete
    TEST_ASSERT(!slate->filesys_is_writing_file,
                "File write should be complete after partial last packet");

    // Verify packet: FTP_EOF_SUCCESS with file_len_on_disk == 400
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_EOF_SUCCESS,
                "Expected FTP_EOF_SUCCESS, got %d", info.ftp_result);

    ftp_test_eof_info_t eof = ftp_test_parse_eof_info(&info);
    TEST_ASSERT(eof.file_len_on_disk == ftp_test_file_partial_len,
                "Expected file_len_on_disk %u, got %u",
                ftp_test_file_partial_len, eof.file_len_on_disk);

    return 0;
}

/*
 * ============================================================================
 * E. Cancel File Write Tests
 * ============================================================================
 *
 * Expected packet behavior:
 * - On success: FTP_CANCEL_SUCCESS with original file's fname/len/crc headers
 *   (captured before cancel clears state). No additional data.
 * - On error: FTP_CANCEL_ERROR with filesys_error + lfs_error.
 */

int ftp_test_cancel_success(slate_t *slate)
{
    LOG_DEBUG("=== Test: Cancel Success ===");

    // Start a file write
    int rc = ftp_test_helper_start_file_write(
        slate, ftp_test_fname1, ftp_test_file_3pkt_len, ftp_test_file_3pkt_crc);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    // Send one packet so there's some state
    FTP_WRITE_TO_FILE_DATA wd;
    ftp_test_make_write_data(0, ftp_test_fname1, ftp_test_file_3pkt_len, &wd);
    ftp_process_file_write_data_command(slate, wd);

    // Cancel
    FTP_CANCEL_FILE_WRITE_DATA cancel_data =
        ftp_test_make_cancel_data(ftp_test_fname1);
    ftp_process_file_cancel_write_command(slate, cancel_data);

    // File write should be cancelled
    TEST_ASSERT(!slate->filesys_is_writing_file,
                "File write should be cancelled");

    // Verify packet: FTP_CANCEL_SUCCESS with original file's headers
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_CANCEL_SUCCESS,
                "Expected FTP_CANCEL_SUCCESS, got %d", info.ftp_result);
    TEST_ASSERT(info.fname == string_to_file(ftp_test_fname1),
                "Cancel packet should have original fname 'AB'");
    TEST_ASSERT(info.file_len == ftp_test_file_3pkt_len,
                "Cancel packet should have original file_len");
    TEST_ASSERT(info.file_crc == ftp_test_file_3pkt_crc,
                "Cancel packet should have original file_crc");

    return 0;
}

int ftp_test_cancel_clears_state(slate_t *slate)
{
    LOG_DEBUG("=== Test: Cancel Clears State ===");

    // Start a file and send some packets
    int rc = ftp_test_helper_start_file_write(
        slate, ftp_test_fname1, ftp_test_file_3pkt_len, ftp_test_file_3pkt_crc);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    FTP_WRITE_TO_FILE_DATA wd;
    ftp_test_make_write_data(0, ftp_test_fname1, ftp_test_file_3pkt_len, &wd);
    ftp_process_file_write_data_command(slate, wd);

    // Cancel
    FTP_CANCEL_FILE_WRITE_DATA cancel_data =
        ftp_test_make_cancel_data(ftp_test_fname1);
    ftp_process_file_cancel_write_command(slate, cancel_data);

    TEST_ASSERT(!slate->filesys_is_writing_file,
                "filesys_is_writing_file should be false after cancel");

    // Verify cancel packet
    packet_t cancel_pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t cancel_info =
        ftp_test_get_ground_info(slate, cancel_pkt);
    TEST_ASSERT(cancel_info.ftp_result == FTP_CANCEL_SUCCESS,
                "Expected FTP_CANCEL_SUCCESS, got %d", cancel_info.ftp_result);

    // After cancel, should be able to start a new file
    FTP_START_FILE_WRITE_DATA new_data = ftp_test_make_start_data(
        ftp_test_fname1, ftp_test_file_1pkt_len, ftp_test_file_1pkt_crc);
    ftp_process_file_start_write_command(slate, new_data);

    TEST_ASSERT(slate->filesys_is_writing_file,
                "Should be able to start new file after cancel");

    // Verify new file's READY_RECEIVE
    packet_t new_pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t new_info = ftp_test_get_ground_info(slate, new_pkt);
    TEST_ASSERT(new_info.ftp_result == FTP_READY_RECEIVE,
                "Expected FTP_READY_RECEIVE for new file, got %d",
                new_info.ftp_result);
    TEST_ASSERT(new_info.file_len == ftp_test_file_1pkt_len,
                "New file READY_RECEIVE should have correct len");

    return 0;
}

/*
 * ============================================================================
 * F. End-to-End Workflow Tests
 * ============================================================================
 */

int ftp_test_e2e_single_cycle_file(slate_t *slate)
{
    LOG_DEBUG("=== Test: E2E Single Cycle File ===");

    // Reformat first for a clean slate
    ftp_process_reformat_command(slate);
    ftp_test_drain_queue(slate); // drain FILESYS_REFORMAT_SUCCESS

    // Start a 3-packet (615-byte) file
    int rc = ftp_test_helper_start_file_write(
        slate, ftp_test_fname1, ftp_test_file_3pkt_len, ftp_test_file_3pkt_crc);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    // Send all 3 packets in order
    ftp_test_helper_send_cycle_packets(slate, 3, ftp_test_fname1,
                                       ftp_test_file_3pkt_len);

    // File should be complete
    TEST_ASSERT(!slate->filesys_is_writing_file,
                "File write should be complete");

    // Verify file exists on filesystem via filesys API
    lfs_ssize_t lfs_err;
    filesys_file_info_t file_info;
    filesys_error_t res = filesys_get_file_info(slate, (char *)ftp_test_fname1,
                                                &file_info, &lfs_err);
    TEST_ASSERT(res == FILESYS_OK,
                "Should be able to get file info after successful write");
    TEST_ASSERT(file_info.file_size == ftp_test_file_3pkt_len,
                "File size should be %u, got %u", ftp_test_file_3pkt_len,
                file_info.file_size);

    // Verify EOF packet
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_EOF_SUCCESS,
                "Expected FTP_EOF_SUCCESS, got %d", info.ftp_result);

    ftp_test_eof_info_t eof = ftp_test_parse_eof_info(&info);
    TEST_ASSERT(eof.computed_crc == ftp_test_file_3pkt_crc,
                "Expected CRC 0x%08X, got 0x%08X", ftp_test_file_3pkt_crc,
                eof.computed_crc);
    TEST_ASSERT(eof.file_len_on_disk == ftp_test_file_3pkt_len,
                "Expected file_len_on_disk %u, got %u", ftp_test_file_3pkt_len,
                eof.file_len_on_disk);

    return 0;
}

int ftp_test_e2e_multi_cycle_file(slate_t *slate)
{
    LOG_DEBUG("=== Test: E2E Multi-Cycle File ===");

    ftp_process_reformat_command(slate);
    ftp_test_drain_queue(slate); // drain FILESYS_REFORMAT_SUCCESS

    // File that spans exactly 2 cycles: 257 packets
    // (256 in first cycle, 1 in second)
    const uint32_t file_len = (uint32_t)257 * FTP_DATA_PAYLOAD_SIZE;

    int rc = ftp_test_helper_start_file_write(slate, ftp_test_fname1, file_len,
                                              516126806);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    // Complete first cycle (256 packets)
    ftp_test_helper_send_cycle_packets(slate, FTP_NUM_PACKETS_PER_CYCLE,
                                       ftp_test_fname1, file_len);

    TEST_ASSERT(slate->filesys_is_writing_file,
                "Should still be writing after first cycle");
    TEST_ASSERT(slate->ftp_start_cycle_packet_id == FTP_NUM_PACKETS_PER_CYCLE,
                "Should be on second cycle, got start_id=%u",
                slate->ftp_start_cycle_packet_id);

    // Verify FTP_FILE_WRITE_SUCCESS after first cycle
    packet_t cycle_pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t cycle_info =
        ftp_test_get_ground_info(slate, cycle_pkt);
    TEST_ASSERT(cycle_info.ftp_result == FTP_FILE_WRITE_SUCCESS,
                "Expected FTP_FILE_WRITE_SUCCESS after cycle 1, got %d",
                cycle_info.ftp_result);

    ftp_test_cycle_info_t cycle = ftp_test_parse_cycle_info(&cycle_info);
    TEST_ASSERT(cycle.packet_start == FTP_NUM_PACKETS_PER_CYCLE,
                "Expected packet_start %u, got %u", FTP_NUM_PACKETS_PER_CYCLE,
                cycle.packet_start);

    // Complete second cycle (1 packet)
    ftp_test_helper_send_cycle_packets(slate, 1, ftp_test_fname1, file_len);

    TEST_ASSERT(!slate->filesys_is_writing_file,
                "File write should be complete after second cycle");

    // Verify EOF packet
    packet_t eof_pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t eof_info = ftp_test_get_ground_info(slate, eof_pkt);
    TEST_ASSERT(eof_info.ftp_result == FTP_EOF_SUCCESS ||
                    eof_info.ftp_result == FTP_EOF_CRC_ERROR,
                "Expected FTP_EOF_SUCCESS or FTP_EOF_CRC_ERROR, got %d",
                eof_info.ftp_result);

    return 0;
}

int ftp_test_e2e_cancel_then_new_file(slate_t *slate)
{
    LOG_DEBUG("=== Test: E2E Cancel Then New File ===");

    ftp_process_reformat_command(slate);
    ftp_test_drain_queue(slate); // drain FILESYS_REFORMAT_SUCCESS

    // Start first file
    int rc = ftp_test_helper_start_file_write(
        slate, ftp_test_fname1, ftp_test_file_3pkt_len, ftp_test_file_3pkt_crc);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    // Send 1 packet
    FTP_WRITE_TO_FILE_DATA wd;
    ftp_test_make_write_data(0, ftp_test_fname1, ftp_test_file_3pkt_len, &wd);
    ftp_process_file_write_data_command(slate, wd);

    // Cancel
    FTP_CANCEL_FILE_WRITE_DATA cancel_data =
        ftp_test_make_cancel_data(ftp_test_fname1);
    ftp_process_file_cancel_write_command(slate, cancel_data);

    TEST_ASSERT(!slate->filesys_is_writing_file,
                "Should not be writing after cancel");

    // Verify cancel packet
    packet_t cancel_pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t cancel_info =
        ftp_test_get_ground_info(slate, cancel_pkt);
    TEST_ASSERT(cancel_info.ftp_result == FTP_CANCEL_SUCCESS,
                "Expected FTP_CANCEL_SUCCESS, got %d", cancel_info.ftp_result);
    TEST_ASSERT(cancel_info.fname == string_to_file(ftp_test_fname1),
                "Cancel packet should have original fname 'AB'");

    // Start a new file with different name
    rc = ftp_test_helper_start_file_write(
        slate, ftp_test_fname2, ftp_test_file_1pkt_len, ftp_test_file_1pkt_crc);
    TEST_ASSERT(rc == 0,
                "ftp_test_helper_start_file_write failed for new file");
    TEST_ASSERT(memcmp(slate->filesys_buffered_fname_str, ftp_test_fname2,
                       sizeof(FILESYS_BUFFERED_FNAME_STR_T)) == 0,
                "Should be writing file 'CD'");

    return 0;
}

int ftp_test_e2e_crc_mismatch(slate_t *slate)
{
    LOG_DEBUG("=== Test: E2E CRC Mismatch ===");

    ftp_process_reformat_command(slate);
    ftp_test_drain_queue(slate); // drain FILESYS_REFORMAT_SUCCESS

    // Start a file with an intentionally wrong CRC
    int rc = ftp_test_helper_start_file_write(
        slate, ftp_test_fname1, ftp_test_file_3pkt_len, ftp_test_wrong_crc);
    TEST_ASSERT(rc == 0, "ftp_test_helper_start_file_write failed");

    // Send all packets correctly
    ftp_test_helper_send_cycle_packets(slate, 3, ftp_test_fname1,
                                       ftp_test_file_3pkt_len);

    // File write should complete (even though CRC will mismatch)
    TEST_ASSERT(!slate->filesys_is_writing_file,
                "File write should complete even with CRC mismatch");

    // Verify packet: FTP_EOF_CRC_ERROR with actual computed CRC
    packet_t pkt = ftp_test_pop_last_packet(slate);
    ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
    TEST_ASSERT(info.ftp_result == FTP_EOF_CRC_ERROR,
                "Expected FTP_EOF_CRC_ERROR, got %d", info.ftp_result);

    ftp_test_eof_info_t eof = ftp_test_parse_eof_info(&info);
    TEST_ASSERT(eof.computed_crc != ftp_test_wrong_crc,
                "Computed CRC should differ from the wrong CRC");
    TEST_ASSERT(eof.computed_crc == ftp_test_file_3pkt_crc,
                "Computed CRC should be the actual data CRC 0x%08X, got "
                "0x%08X",
                ftp_test_file_3pkt_crc, eof.computed_crc);
    TEST_ASSERT(eof.file_len_on_disk == ftp_test_file_3pkt_len,
                "Expected file_len_on_disk %u, got %u", ftp_test_file_3pkt_len,
                eof.file_len_on_disk);

    return 0;
}

/*
 * ============================================================================
 * G. Init Tests
 * ============================================================================
 *
 * Expected packet behavior:
 * - On success: No packet sent (just initializes filesystem).
 * - On failure: FILESYS_INIT_ERROR with filesys_error + lfs_error,
 *   fname="XX", len=0, crc=0.
 */

int ftp_test_init_success(slate_t *slate)
{
    LOG_DEBUG("=== Test: Init Success ===");

    // ftp_task_init calls filesys_initialize, so we need a clean slate
    // but NOT a pre-initialized filesystem (init does that itself).
    // Note: since ftp_test_setup already calls reformat, init should
    // succeed on an already-initialized filesystem.
    ftp_task_init(slate);

    // After init, state should be clean
    TEST_ASSERT(slate->ftp_start_cycle_packet_id == 0,
                "Start cycle packet ID should be 0 after init");

    // Init success sends no packet
    TEST_ASSERT(queue_is_empty(&slate->tx_queue),
                "No packet should be enqueued on successful init");

    return 0;
}

int ftp_test_init_clears_state(slate_t *slate)
{
    LOG_DEBUG("=== Test: Init Clears State ===");

    // Dirty the state
    ftp_tracker_set_bit(&slate->ftp_packets_received_tracker, 100);
    slate->ftp_start_cycle_packet_id = 500;

    ftp_task_init(slate);

    TEST_ASSERT(slate->ftp_start_cycle_packet_id == 0,
                "Start cycle should be 0 after init, got %u",
                slate->ftp_start_cycle_packet_id);

    TEST_ASSERT(
        !ftp_tracker_check_bit(&slate->ftp_packets_received_tracker, 100),
        "Tracker bit 100 should be cleared after init");

    // Init success sends no packet
    TEST_ASSERT(queue_is_empty(&slate->tx_queue),
                "No packet should be enqueued on successful init");

    return 0;
}

int ftp_test_real_file(slate_t *slate)
{
    const char *file_path = getenv("FTP_TEST_REAL_FILE_PATH");

    TEST_ASSERT(
        file_path != NULL,
        "FTP_TEST_REAL_FILE_PATH environment variable must be set to run "
        "real file test");

    // 1. Compute CRC and length of the file
    FILE *file = fopen(file_path, "rb");
    TEST_ASSERT(file != NULL, "Failed to open real file at %s with error: %s",
                file_path, strerror(errno));

    uint8_t buffer[FILESYS_BUFFER_SIZE];
    unsigned int crc = 0xFFFFFFFF; // Initial CRC value
    size_t num_read;
    size_t total_bytes = 0;

    while ((num_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        LOG_DEBUG("Read %zu bytes from real file", num_read);
        crc = crc32_continue(buffer, num_read, crc);
        total_bytes += num_read;
    }

    crc = ~crc; // Finalize CRC by inverting bits
    LOG_DEBUG("Computed CRC for real file with size %zu: 0x%08X", total_bytes,
              crc);

    fclose(file);

    // 2. Write file via FTP
    ftp_process_file_start_write_command(
        slate, ftp_test_make_start_data(ftp_test_fname1, total_bytes, crc));

    ftp_test_ground_info_t ground_info =
        ftp_test_get_ground_info(slate, ftp_test_pop_last_packet(slate));
    TEST_ASSERT(ground_info.ftp_result == FTP_READY_RECEIVE,
                "Expected FTP_READY_RECEIVE, got %d", ground_info.ftp_result);
    TEST_ASSERT(ground_info.fname == string_to_file(ftp_test_fname1),
                "READY_RECEIVE should have fname 'AB'");
    TEST_ASSERT(ground_info.file_len == total_bytes,
                "READY_RECEIVE should have correct file_len, expected %zu, got "
                "%u",
                total_bytes, ground_info.file_len);

    ftp_test_cycle_info_t cycle_info = ftp_test_parse_cycle_info(&ground_info);
    TEST_ASSERT(cycle_info.packet_start == 0,
                "Expected packet_start 0 for real file, got %u",
                cycle_info.packet_start);
    TEST_ASSERT(
        queue_get_level(&slate->tx_queue) == 0,
        "No packet should be enqueued after READY_RECEIVE for real file");

    file = fopen(file_path, "rb");
    TEST_ASSERT(file != NULL, "Failed to open real file at %s with error: %s",
                file_path, strerror(errno));

    uint8_t buffer_packet[FTP_DATA_PAYLOAD_SIZE];
    uint16_t packet_id = 0;
    while ((num_read = fread(buffer_packet, 1, sizeof(buffer_packet), file)) >
           0)
    {
        FTP_WRITE_TO_FILE_DATA wd;
        ftp_test_make_write_data(packet_id, ftp_test_fname1, total_bytes, &wd);

        // Copy the actual data read into the wd.data buffer
        memcpy(wd.data, buffer_packet, num_read);
        wd.data_len = num_read;
        ftp_process_file_write_data_command(slate, wd);

        // Check if new cycle started
        if (queue_get_level(&slate->tx_queue) > 0)
        {
            packet_t pkt = ftp_test_pop_last_packet(slate);
            ftp_test_ground_info_t info = ftp_test_get_ground_info(slate, pkt);
            TEST_ASSERT(info.ftp_result == FTP_FILE_WRITE_SUCCESS ||
                            info.ftp_result == FTP_EOF_SUCCESS,
                        "Expected FTP_FILE_WRITE_SUCCESS after cycle, got %d",
                        info.ftp_result);

            if (info.ftp_result == FTP_EOF_SUCCESS)
            {
                ftp_test_eof_info_t eof_info = ftp_test_parse_eof_info(&info);
                TEST_ASSERT(eof_info.computed_crc == crc,
                            "Expected CRC 0x%08X, got 0x%08X", crc,
                            eof_info.computed_crc);
                TEST_ASSERT(eof_info.file_len_on_disk == total_bytes,
                            "Expected file_len_on_disk %zu, got %u",
                            total_bytes, eof_info.file_len_on_disk);
                LOG_DEBUG("Reached EOF for real file after packet_id %u",
                          packet_id);
            }
            else
            {

                cycle_info = ftp_test_parse_cycle_info(&info);
                TEST_ASSERT(cycle_info.packet_start == packet_id + 1,
                            "Expected next cycle to start at packet_id %u, got "
                            "%u",
                            packet_id + 1, cycle_info.packet_start);
                LOG_DEBUG("Completed cycle for real file: now packet_start=%u, "
                          "packet_end=%u",
                          cycle_info.packet_start, cycle_info.packet_end);
            }
        }

        packet_id++;
    }

    fclose(file);

    // 3. Check if file was written correctly
    lfs_ssize_t lfs_err;
    filesys_file_info_t info;
    filesys_error_t res =
        filesys_get_file_info(slate, (char *)ftp_test_fname1, &info, &lfs_err);
    TEST_ASSERT(res == FILESYS_OK,
                "Should be able to get file info for real file after write");
    TEST_ASSERT(info.file_size == total_bytes,
                "File size should be %zu, got %u", total_bytes, info.file_size);

    // 4. Byte-by-byte comparison (should not be needed because of CRC32, but
    // just in case)
    file = fopen(file_path, "rb");
    TEST_ASSERT(file != NULL, "Failed to open real file at %s with error: %s",
                file_path, strerror(errno));
    uint8_t buffer_written[FILESYS_BUFFER_SIZE];
    size_t offset = 0;

    lfs_file_t lfs_file;
    filesys_file_info_t written_info;
    filesys_open_file_read(slate, &lfs_file, ftp_test_fname1, &written_info,
                           &lfs_err);

    FILESYS_BUFFERED_FILE_LEN_T filesys_num_read;
    while ((num_read = fread(buffer_packet, 1, sizeof(buffer_written), file)) >
           0)
    {
        // Read the corresponding data from the written file
        filesys_error_t error = filesys_read_data(
            slate, &lfs_file, buffer_written, sizeof(buffer_written),
            &filesys_num_read, &lfs_err);

        TEST_ASSERT(error >= 0,
                    "Failed to read back written file with "
                    "error: %d",
                    lfs_err);

        TEST_ASSERT(filesys_num_read == num_read,
                    "Read back size should match original file, expected %zu, "
                    "got %zd at offset %zu",
                    num_read, filesys_num_read, offset);

        TEST_ASSERT(
            memcmp(buffer_packet, buffer_written, num_read) == 0,
            "Data read back from written file should match original file");

        offset += num_read;
    }

    return 0;
}

/*
 * ============================================================================
 * Test Registration
 * ============================================================================
 */

const test_harness_case_t ftp_tests[] = {
    /* A. Packet Tracker */
    {0, ftp_test_tracker_clear, "Tracker Clear", false},
    {1, ftp_test_tracker_set_and_check, "Tracker Set and Check", false},
    {2, ftp_test_tracker_check_mask_full_bytes,
     "Tracker Check Mask - Full Bytes", false},
    {3, ftp_test_tracker_check_mask_partial_byte,
     "Tracker Check Mask - Partial Byte", false},
    {4, ftp_test_tracker_check_mask_incomplete,
     "Tracker Check Mask - Incomplete", false},

    /* B. Reformat */
    {10, ftp_test_reformat_success, "Reformat Success", true},
    {11, ftp_test_reformat_clears_state, "Reformat Clears State", true},

    /* C. Start File Write */
    {20, ftp_test_start_write_success, "Start Write Success", true},
    {21, ftp_test_start_write_already_writing, "Start Write - Already Writing",
     true},
    {22, ftp_test_start_write_sets_slate_state, "Start Write Sets Slate State",
     true},

    /* D. Write File Data */
    {30, ftp_test_write_data_no_file, "Write Data - No File", true},
    {31, ftp_test_write_data_single_packet_file,
     "Write Data - Single Packet File", true},
    {32, ftp_test_write_data_mid_cycle_no_response,
     "Write Data - Mid Cycle No Response", true},
    {33, ftp_test_write_data_complete_cycle_not_final,
     "Write Data - Complete Cycle (Not Final)", true},
    {34, ftp_test_write_data_complete_final_cycle,
     "Write Data - Complete Final Cycle", true},
    {35, ftp_test_write_data_out_of_range_too_low,
     "Write Data - Out of Range (Low)", true},
    {36, ftp_test_write_data_out_of_range_too_high,
     "Write Data - Out of Range (High)", true},
    {37, ftp_test_write_data_duplicate_ignored,
     "Write Data - Duplicate Ignored", true},
    {38, ftp_test_write_data_out_of_order, "Write Data - Out of Order", true},
    {39, ftp_test_write_data_last_packet_partial_size,
     "Write Data - Last Packet Partial Size", true},

    /* E. Cancel */
    {50, ftp_test_cancel_success, "Cancel Success", true},
    {51, ftp_test_cancel_clears_state, "Cancel Clears State", true},

    /* F. End-to-End */
    {80, ftp_test_e2e_single_cycle_file, "E2E Single Cycle File", true},
    {81, ftp_test_e2e_multi_cycle_file, "E2E Multi-Cycle File", true},
    {82, ftp_test_e2e_cancel_then_new_file, "E2E Cancel Then New File", true},
    {83, ftp_test_e2e_crc_mismatch, "E2E CRC Mismatch", true},

    /* G. Init */
    {90, ftp_test_init_success, "Init Success", true},
    {91, ftp_test_init_clears_state, "Init Clears State", true},

    /* H. Real File */
    {100, ftp_test_real_file, "Real File", true}};

const size_t ftp_tests_len = sizeof(ftp_tests) / sizeof(test_harness_case_t);

int main()
{
    return test_harness_run("FTP", ftp_tests, ftp_tests_len, ftp_test_setup);
}
