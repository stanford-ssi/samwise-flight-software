#include "ftp_task.h"
#include "filesys/filesys.h"
#include "neopixel.h"

void ftp_send_result_packet_custom_file(
    slate_t *slate, FILESYS_BUFFERED_FNAME_STR_T buffered_fname_str,
    FILESYS_BUFFERED_FILE_LEN_T buffered_file_len,
    FILESYS_BUFFERED_FILE_CRC_T buffered_file_crc, FTP_Result result,
    const void *additional_data, size_t additional_data_len)
{
    const size_t packet_len = sizeof(FILESYS_BUFFERED_FNAME_STR_T) +
                              sizeof(FILESYS_BUFFERED_FILE_LEN_T) +
                              sizeof(FILESYS_BUFFERED_FILE_CRC_T) +
                              sizeof(FTP_Result) + additional_data_len;

    if (packet_len > PACKET_DATA_SIZE)
    {
        LOG_ERROR(
            "[FTP] FTP result packet %d too large to send! Not sending packet.",
            result);
        return;
    }

    uint8_t data[PACKET_DATA_SIZE];

    uint8_t *data_ptr = data;

    memcpy_inc(&data_ptr, &buffered_fname_str,
               sizeof(FILESYS_BUFFERED_FNAME_STR_T));

    memcpy_inc(&data_ptr, &buffered_file_len,
               sizeof(FILESYS_BUFFERED_FILE_LEN_T));

    memcpy_inc(&data_ptr, &buffered_file_crc,
               sizeof(FILESYS_BUFFERED_FILE_CRC_T));

    memcpy_inc(&data_ptr, &result, sizeof(FTP_Result));

    if (additional_data != NULL && additional_data_len > 0)
        memcpy_inc(&data_ptr, additional_data, additional_data_len);

    assert(data_ptr - data == packet_len); // Sanity check

    packet_t pkt;
    // TODO: What are all of these fields supposed to be?
    rfm9x_format_packet(&pkt, 0, 0, 0, 0, packet_len, data);

    if (!queue_try_add(&slate->tx_queue, &pkt))
        LOG_ERROR("[FTP] Failed to enqueue FTP result packet %d for TX!",
                  result);
    else
        LOG_INFO("[FTP] Sent FTP result packet %d", result);
}

void ftp_send_result_packet_no_file(slate_t *slate, FTP_Result result,
                                    const void *additional_data,
                                    size_t additional_data_len)
{
    ftp_send_result_packet_custom_file(slate, "XX", 0, 0, result,
                                       additional_data, additional_data_len);
}

void ftp_send_result_packet(slate_t *slate, FTP_Result result,
                            const void *additional_data,
                            size_t additional_data_len)
{
    if (!slate->filesys_is_writing_file)
    {
        LOG_ERROR("[FTP] Attempted to send FTP result packet %d but no file is "
                  "being written! Sending packet with no file contents.",
                  result);

        ftp_send_result_packet_no_file(slate, result, additional_data,
                                       additional_data_len);
        return;
    }

    ftp_send_result_packet_custom_file(slate, slate->filesys_buffered_fname_str,
                                       slate->filesys_buffered_file_len,
                                       slate->filesys_buffered_file_crc, result,
                                       additional_data, additional_data_len);
}

inline static FTP_PACKET_SEQUENCE_T ftp_get_last_packet(slate_t *slate)
{
    return slate->ftp_start_cycle_packet_id + FTP_NUM_PACKETS_PER_CYCLE - 1;
}

/**
 * Works for FILESYS_INIT_ERROR, FILESYS_REFORMAT_ERROR,
 * FTP_FILE_WRITE_MRAM_ERROR, FTP_FILE_WRITE_BUFFER_ERROR,
 * and FTP_CANCEL_ERROR
 */
inline static void send_ftp_lfs_error_packet(slate_t *slate, FTP_Result result,
                                             lfs_ssize_t lfs_error_code)
{
    assert(result == FILESYS_INIT_ERROR || result == FILESYS_REFORMAT_ERROR ||
           result == FTP_FILE_WRITE_MRAM_ERROR ||
           result == FTP_FILE_WRITE_BUFFER_ERROR || result == FTP_CANCEL_ERROR);

    ftp_send_result_packet_no_file(slate, result, &lfs_error_code,
                                   sizeof(lfs_error_code));
}

/**
 * Works for FTP_READY_RECEIVE, FTP_FILE_WRITE_SUCCESS, and
 * FTP_ERROR_PACKET_OUT_OF_RANGE
 */
inline static void
send_ftp_cycle_info_packet(slate_t *slate, FTP_Result result,
                           FTP_PACKET_SEQUENCE_T packet_start,
                           FTP_PACKET_TRACKER_T packets_received_tracker)
{
    assert(result == FTP_READY_RECEIVE || result == FTP_FILE_WRITE_SUCCESS ||
           result == FTP_ERROR_PACKET_OUT_OF_RANGE);

    uint8_t result_data[sizeof(FTP_PACKET_SEQUENCE_T) * 2 +
                        sizeof(FTP_PACKET_TRACKER_T)];

    uint8_t *result_data_ptr = result_data;
    memcpy_inc(&result_data_ptr, &packet_start, sizeof(FTP_PACKET_SEQUENCE_T));
    memcpy_inc(&result_data_ptr, &ftp_get_last_packet(slate),
               sizeof(FTP_PACKET_SEQUENCE_T));
    memcpy_inc(&result_data_ptr, &packets_received_tracker,
               sizeof(FTP_PACKET_TRACKER_T));

    ftp_send_result_packet(slate, result, result_data, sizeof(result_data));
}

void ftp_task_init(slate_t *slate)
{
    LOG_INFO("FTP task is initializing...");

    slate->ftp_packets_received_tracker = 0;
    slate->ftp_start_cycle_packet_id = 0;

    lfs_ssize_t res = filesys_initialize(slate);

    if (res < 0)
    {
        LOG_ERROR(
            "[FTP] FTP task failed to initialize filesystem with error %d.",
            res);

        send_ftp_lfs_error_packet(slate, FILESYS_INIT_ERROR, res);

        return;
    }

    LOG_INFO("FTP task initialized filesystem successfully.");

    // Queues are initialized on command_task.h
}

void ftp_process_reformat_command(slate_t *slate)
{
    slate->ftp_packets_received_tracker = 0;
    slate->ftp_start_cycle_packet_id = 0;

    lfs_ssize_t res = filesys_reformat(slate);
    if (res < 0)
    {
        LOG_ERROR("[FTP] FTP task failed to format filesystem with error %d.",
                  res);

        send_ftp_lfs_error_packet(slate, FILESYS_REFORMAT_ERROR, res);

        return;
    }

    ftp_send_result_packet_no_file(slate, FILESYS_REFORMAT_SUCCESS, NULL, 0);

    LOG_INFO("[FTP] FTP task reformatted filesystem successfully.");
}

void ftp_process_file_start_write_command(
    slate_t *slate, FTP_START_FILE_WRITE_DATA command_data)
{
    if (slate->filesys_is_writing_file)
    {
        LOG_ERROR(
            "[FTP] Received start file write command but a file is already "
            "being written! Ignoring.");

        ftp_send_result_packet(slate, FTP_ERROR_ALREADY_WRITING_FILE,
                               slate->filesys_buffered_fname_str,
                               sizeof(slate->filesys_buffered_fname_str));

        return;
    }

    lfs_ssize_t blocksLeftAfterWrite = -1;
    int8_t res = filesys_start_file_write(
        slate, command_data.fname_str, command_data.file_len,
        command_data.file_crc, &blocksLeftAfterWrite);

    if (res != 0)
    {
        LOG_ERROR("[FTP] Failed to start file write with code %d.", res);

        uint8_t err_data[sizeof(res) + sizeof(blocksLeftAfterWrite)];
        uint8_t *err_data_ptr = err_data;

        memcpy_inc(&err_data_ptr, &res, sizeof(res));
        memcpy_inc(&err_data_ptr, &blocksLeftAfterWrite,
                   sizeof(blocksLeftAfterWrite));

        ftp_send_result_packet(slate, FTP_ERROR_RECEIVE, err_data,
                               sizeof(err_data));
        return;
    }

    if (blocksLeftAfterWrite < 0)
    {
        LOG_ERROR("[FTP] Could not determine blocks left after file write.");

        uint8_t err_data[sizeof(res) + sizeof(blocksLeftAfterWrite)];
        uint8_t *err_data_ptr = err_data;

        memcpy_inc(&err_data_ptr, &res, sizeof(res));
        memcpy_inc(&err_data_ptr, &blocksLeftAfterWrite,
                   sizeof(blocksLeftAfterWrite));

        ftp_send_result_packet(slate, FTP_ERROR_RECEIVE, err_data,
                               sizeof(err_data));
        return;
    }

    slate->ftp_start_cycle_packet_id = 0;
    slate->ftp_packets_received_tracker = 0;

    LOG_INFO("[FTP] Started write to file! Blocks left after file is done "
             "writing, if successful: %d",
             blocksLeftAfterWrite);

    // Send READY_RECEIVE for first cycle
    send_ftp_cycle_info_packet(slate, FTP_READY_RECEIVE,
                               slate->ftp_start_cycle_packet_id,
                               slate->ftp_packets_received_tracker);
}

void ftp_process_file_write_data_command(slate_t *slate,
                                         FTP_WRITE_TO_FILE_DATA command_data)
{
    if (!slate->filesys_is_writing_file)
    {
        LOG_ERROR("[FTP] Received file write data command but no file is being "
                  "written! Ignoring.");
        ftp_send_result_packet_no_file(slate, FTP_ERROR_NOT_WRITING_FILE, NULL,
                                       0);
        return;
    }

    if (command_data.packet_id < slate->ftp_start_cycle_packet_id ||
        command_data.packet_id >=
            slate->ftp_start_cycle_packet_id + FTP_NUM_PACKETS_PER_CYCLE)
    {
        LOG_ERROR(
            "[FTP] Received packet ID %d outside of current cycle range (%d to "
            "%d)!",
            command_data.packet_id, slate->ftp_start_cycle_packet_id,
            slate->ftp_start_cycle_packet_id + FTP_NUM_PACKETS_PER_CYCLE - 1);

        // Send error packet with expected range and received tracker
        send_ftp_cycle_info_packet(slate, FTP_ERROR_PACKET_OUT_OF_RANGE,
                                   slate->ftp_start_cycle_packet_id,
                                   slate->ftp_packets_received_tracker);
        return;
    }

    // 1. Write data to buffer at correct offset
    // packet_index is the 0-based index within the current cycle (0 to
    // FTP_NUM_PACKETS_PER_CYCLE - 1)
    const FTP_PACKET_SEQUENCE_T packet_index =
        command_data.packet_id % FTP_NUM_PACKETS_PER_CYCLE;
    const FILESYS_BUFFER_SIZE_T offset = packet_index * FTP_DATA_PAYLOAD_SIZE;

    // Write data to buffer
    int8_t write_res = filesys_write_data_to_buffer(
        slate, command_data.data, command_data.data_len, offset);

    if (write_res != 0)
    {
        LOG_ERROR("[FTP] Failed to write data to buffer: %d", write_res);
        send_ftp_lfs_error_packet(slate, FTP_FILE_WRITE_BUFFER_ERROR,
                                  write_res);
        return;
    }

    // 2. Update received tracker
    slate->ftp_packets_received_tracker |= (1 << packet_index);

    // 3. Check if cycle is complete
    const FILESYS_BUFFERED_FILE_LEN_T total_packets =
        (slate->filesys_buffered_file_len + FTP_DATA_PAYLOAD_SIZE - 1) /
        FTP_DATA_PAYLOAD_SIZE;

    // Calculate how many packets we expect in this specific cycle
    // (Usually FTP_NUM_PACKETS_PER_CYCLE, unless it's the last incomplete
    // cycle)
    const FILESYS_BUFFERED_FILE_LEN_T packets_remaining =
        total_packets - slate->ftp_start_cycle_packet_id;

    const FTP_PACKET_SEQUENCE_T packets_in_this_cycle =
        (packets_remaining > FTP_NUM_PACKETS_PER_CYCLE)
            ? FTP_NUM_PACKETS_PER_CYCLE
            : packets_remaining;

    const FTP_PACKET_TRACKER_T expected_mask =
        (1 << packets_in_this_cycle) - 1; // All 1s for expected packets

    LOG_DEBUG("[FTP] Packet %d received. Mask: 0x%X, Expected: 0x%X",
              command_data.packet_id, slate->ftp_packets_received_tracker,
              expected_mask);

    // If not the last in the cycle, send FTP_READY_RECEIVE and return
    if (slate->ftp_packets_received_tracker != expected_mask)
    {
        // Send FTP_READY_RECEIVE with Packet range
        send_ftp_cycle_info_packet(slate, FTP_READY_RECEIVE,
                                   slate->ftp_start_cycle_packet_id,
                                   slate->ftp_packets_received_tracker);
        return;
    }

    // else:
    // 4. Write buffer to MRAM.

    // Calculate bytes to write
    FILESYS_BUFFER_SIZE_T bytes_to_write;
    if (packets_remaining > FTP_NUM_PACKETS_PER_CYCLE)
    {
        // Not the last cycle, so full buffer
        bytes_to_write = FTP_NUM_PACKETS_PER_CYCLE * FTP_DATA_PAYLOAD_SIZE;
    }
    else
    {
        // Last cycle in file. Calculate remaining bytes.
        // We need to subtract what we've conceptually written so far.
        const FTP_PACKET_SEQUENCE_T bytes_previously_written =
            (slate->ftp_start_cycle_packet_id - 1) * FTP_DATA_PAYLOAD_SIZE;
        bytes_to_write =
            slate->filesys_buffered_file_len - bytes_previously_written;
    }

    lfs_ssize_t mram_write_res =
        filesys_write_buffer_to_mram(slate, bytes_to_write);

    if (mram_write_res < 0)
    {
        LOG_ERROR("[FTP] Failed to write buffer to MRAM: %ld", mram_write_res);
        // Send error packet with the error code
        send_ftp_lfs_error_packet(slate, FTP_FILE_WRITE_MRAM_ERROR,
                                  mram_write_res);
        return;
    }

    // Reset received tracker for next cycle
    slate->ftp_packets_received_tracker = 0;

    if (slate->ftp_start_cycle_packet_id + packets_in_this_cycle !=
        total_packets)
    {
        // If not the final cycle, prepare for next cycle and send
        // FTP_FILE_WRITE_SUCCESS
        slate->ftp_start_cycle_packet_id += packets_in_this_cycle;
        send_ftp_cycle_info_packet(slate, FTP_FILE_WRITE_SUCCESS,
                                   slate->ftp_start_cycle_packet_id,
                                   slate->ftp_packets_received_tracker);
        return;
    }

    // else:
    // 5. Write complete! Process EOF.
    unsigned int computed_crc;
    int8_t complete_res = filesys_complete_file_write(slate, &computed_crc);

    if (complete_res != 0)
    {
        LOG_ERROR("[FTP] Error completing file write: %d", complete_res);
        // Determine error type
        if (complete_res == -4) // CRC Mismatch
        {
            uint8_t crc_err_data[sizeof(FILESYS_BUFFERED_FILE_CRC_T) +
                                 sizeof(FILESYS_BUFFERED_FILE_LEN_T)];

            uint8_t *crc_err_data_ptr = crc_err_data;
            memcpy_inc(&crc_err_data_ptr, &computed_crc,
                       sizeof(FILESYS_BUFFERED_FILE_CRC_T));

            memcpy_inc(&crc_err_data_ptr, &slate->filesys_buffered_file_len,
                       sizeof(FILESYS_BUFFERED_FILE_LEN_T));

            ftp_send_result_packet(slate, FTP_EOF_CRC_ERROR, &crc_err_data,
                                   sizeof(crc_err_data));
        }
        else
        {
            send_ftp_lfs_error_packet(slate, FTP_FILE_WRITE_MRAM_ERROR,
                                      complete_res);
        }

        return;
    }

    // Success!
    LOG_INFO("[FTP] File write complete & successful! Computed CRC: 0x%X",
             computed_crc);

    // Get file length on disk
    FILESYS_BUFFERED_FILE_LEN_T file_len_on_disk =
        filesys_get_file_length_on_disk(slate,
                                        slate->filesys_buffered_fname_str);

    uint8_t success_data[sizeof(FILESYS_BUFFERED_FILE_CRC_T) +
                         sizeof(FILESYS_BUFFERED_FILE_LEN_T)];

    uint8_t *success_data_ptr = success_data;
    memcpy_inc(&success_data_ptr, &computed_crc,
               sizeof(FILESYS_BUFFERED_FILE_CRC_T));
    memcpy_inc(&success_data_ptr, &file_len_on_disk,
               sizeof(FILESYS_BUFFERED_FILE_LEN_T));

    ftp_send_result_packet(slate, FTP_EOF_SUCCESS, &success_data,
                           sizeof(success_data));
}

void ftp_process_file_cancel_write_command(
    slate_t *slate, FTP_CANCEL_FILE_WRITE_DATA command_data)
{
    int8_t res = filesys_cancel_file_write(slate);

    if (res != 0)
    {
        LOG_ERROR("[FTP] Failed to cancel file write with code %d.", res);
        send_ftp_lfs_error_packet(slate, FTP_CANCEL_ERROR, res);
        return;
    }

    LOG_INFO("[FTP] Successfully cancelled file write for file %s.",
             command_data.fname_str);
    ftp_send_result_packet(slate, FTP_CANCEL_SUCCESS, NULL, 0);
}

void ftp_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(FTP_TASK_COLOR);
    LOG_INFO("[FTP] FTP task is running...");

    if (!queue_is_empty(&slate->ftp_format_filesystem_data))
    {
        ftp_process_reformat_command(slate);

        if (!queue_try_remove(&slate->ftp_format_filesystem_data, NULL))
            LOG_ERROR("[FTP] Failed to remove format file command from queue.");
    }

    else if (!queue_is_empty(&slate->ftp_start_file_write_data))
    {
        FTP_START_FILE_WRITE_DATA data;

        if (queue_try_remove(&slate->ftp_start_file_write_data, &data))
            ftp_process_file_start_write_command(slate, data);
        else
            LOG_ERROR("[FTP] Failed to remove start file command from queue.");
    }

    else if (!queue_is_empty(&slate->ftp_write_to_file_data))
    {
        FTP_WRITE_TO_FILE_DATA data;

        if (queue_try_remove(&slate->ftp_write_to_file_data, &data))
            ftp_process_file_write_data_command(slate, data);
        else
            LOG_ERROR(
                "[FTP] Failed to remove process file command from queue.");
    }

    else if (!queue_is_empty(&slate->ftp_cancel_file_write_data))
    {
        FTP_CANCEL_FILE_WRITE_DATA data;

        if (queue_try_remove(&slate->ftp_cancel_file_write_data, &data))
            ftp_process_file_cancel_write_command(slate, data);
        else
            LOG_ERROR("[FTP] Failed to remove cancel file command from queue.");
    }

    if (!slate->filesys_is_writing_file)
    {
        ftp_task.next_dispatch =
            make_timeout_time_ms(FTP_IDLE_DISPATCH_MS); // Slow down when
                                                        // idle
    }

    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t ftp_task = {
    .name = "ftp_task",
    .dispatch_period_ms =
        FTP_ACTIVE_DISPATCH_MS, // By default, very fast dispatch to chuck
                                // packets quickly
    .task_init = &ftp_task_init,
    .task_dispatch = &ftp_task_dispatch,
    .next_dispatch = 0};
