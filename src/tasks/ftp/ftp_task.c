#include "ftp_task.h"

void ftp_send_result_packet_custom_file(
    slate_t *slate, const FILESYS_BUFFERED_FNAME_STR_T buffered_fname_str,
    FILESYS_BUFFERED_FILE_LEN_T buffered_file_len,
    FILESYS_BUFFERED_FILE_CRC_T buffered_file_crc, ftp_result_t result,
    const void *additional_data, size_t additional_data_len)
{
    const size_t packet_len = sizeof(FILESYS_BUFFERED_FNAME_T) +
                              sizeof(FILESYS_BUFFERED_FILE_LEN_T) +
                              sizeof(FILESYS_BUFFERED_FILE_CRC_T) +
                              sizeof(ftp_result_t) + additional_data_len;

    if (packet_len > PACKET_DATA_SIZE)
    {
        LOG_ERROR(
            "[FTP] FTP result packet %d too large to send! Not sending packet.",
            result);
        return;
    }

    uint8_t data[PACKET_DATA_SIZE];

    // Send fname as 2-byte FILESYS_BUFFERED_FNAME_T (not 3-byte str)
    size_t len = buffer_encode_data(
        data, BUFFER_ENCODE_ARG(string_to_file(buffered_fname_str)),
        BUFFER_ENCODE_ARG(buffered_file_len),
        BUFFER_ENCODE_ARG(buffered_file_crc), BUFFER_ENCODE_ARG(result));

    if (additional_data != NULL && additional_data_len > 0)
    {
        memcpy(data + len, additional_data, additional_data_len);
        len += additional_data_len;
    }

    packet_t pkt;
    // TODO: What are all of these fields supposed to be?
    rfm9x_format_packet(&pkt, 0, 0, 0, 0, len, data);

    if (!queue_try_add(&slate->tx_queue, &pkt))
        LOG_ERROR("[FTP] Failed to enqueue FTP result packet %d for TX!",
                  result);
    else
        LOG_INFO("[FTP] Sent FTP result packet %d", result);
}

void ftp_send_result_packet_no_file(slate_t *slate, ftp_result_t result,
                                    const void *additional_data,
                                    size_t additional_data_len)
{
    ftp_send_result_packet_custom_file(slate, "XX", 0, 0, result,
                                       additional_data, additional_data_len);
}

void ftp_send_result_packet(slate_t *slate, ftp_result_t result,
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
 * and FTP_CANCEL_ERROR.
 *
 * FILESYS_INIT_ERROR and FILESYS_REFORMAT_ERROR use _no_file (no file context).
 * All others use _result_packet (includes current file headers from slate).
 */
inline static void send_ftp_lfs_error_packet(slate_t *slate,
                                             ftp_result_t result,
                                             filesys_error_t filesys_err,
                                             lfs_ssize_t lfs_error_code)
{
    assert(result == FILESYS_INIT_ERROR || result == FILESYS_REFORMAT_ERROR ||
           result == FTP_FILE_WRITE_MRAM_ERROR ||
           result == FTP_FILE_WRITE_BUFFER_ERROR || result == FTP_CANCEL_ERROR);

    uint8_t err_data[PACKET_DATA_SIZE];
    size_t len = buffer_encode_data(err_data, BUFFER_ENCODE_ARG(filesys_err),
                                    BUFFER_ENCODE_ARG(lfs_error_code));

    if (result == FILESYS_INIT_ERROR || result == FILESYS_REFORMAT_ERROR)
        ftp_send_result_packet_no_file(slate, result, err_data, len);
    else
        ftp_send_result_packet(slate, result, err_data, len);
}

/**
 * Works for FTP_READY_RECEIVE, FTP_FILE_WRITE_SUCCESS, and
 * FTP_ERROR_PACKET_OUT_OF_RANGE
 */
inline static void
send_ftp_cycle_info_packet(slate_t *slate, ftp_result_t result,
                           FTP_PACKET_SEQUENCE_T packet_start,
                           FTP_PACKET_TRACKER_T packets_received_tracker)
{
    assert(result == FTP_READY_RECEIVE || result == FTP_FILE_WRITE_SUCCESS ||
           result == FTP_ERROR_PACKET_OUT_OF_RANGE);

    uint8_t result_data[PACKET_DATA_SIZE];
    size_t len = buffer_encode_data(
        result_data, BUFFER_ENCODE_ARG(packet_start),
        BUFFER_ENCODE_ARG(ftp_get_last_packet(slate)),
        BUFFER_ENCODE_RAW(FTP_PACKET_TRACKER_SIZE, packets_received_tracker));

    ftp_send_result_packet(slate, result, result_data, len);
}

void ftp_task_init(slate_t *slate)
{
    LOG_INFO("FTP task is initializing...");

    ftp_tracker_clear(&slate->ftp_packets_received_tracker);
    slate->ftp_start_cycle_packet_id = 0;

    lfs_ssize_t lfs_err = LFS_ERR_OK;
    filesys_error_t res = filesys_initialize(slate, &lfs_err);

    if (res != FILESYS_OK)
    {
        LOG_ERROR(
            "[FTP] FTP task failed to initialize filesystem with error %d.",
            res);

        send_ftp_lfs_error_packet(slate, FILESYS_INIT_ERROR, res, lfs_err);

        return;
    }

    LOG_INFO("FTP task initialized filesystem successfully.");

    // Queues are initialized on command_task.h
}

void ftp_process_reformat_command(slate_t *slate)
{
    ftp_tracker_clear(&slate->ftp_packets_received_tracker);
    slate->ftp_start_cycle_packet_id = 0;

    lfs_ssize_t lfs_err = LFS_ERR_OK;
    filesys_error_t res = filesys_reformat_initialize(slate, &lfs_err);
    if (res != FILESYS_OK)
    {
        LOG_ERROR("[FTP] FTP task failed to format filesystem with error %d.",
                  res);

        send_ftp_lfs_error_packet(slate, FILESYS_REFORMAT_ERROR, res, lfs_err);

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

    lfs_ssize_t lfs_err = LFS_ERR_OK;
    lfs_ssize_t blocksLeftAfterWrite = -1;
    filesys_error_t res = filesys_start_file_write(
        slate, command_data.fname_str, command_data.file_len,
        command_data.file_crc, &lfs_err, &blocksLeftAfterWrite);

    if (res != FILESYS_OK)
    {
        LOG_ERROR("[FTP] Failed to start file write with code %d.", res);

        uint8_t err_data[PACKET_DATA_SIZE];
        size_t len = buffer_encode_data(
            err_data, BUFFER_ENCODE_ARG(res), BUFFER_ENCODE_ARG(lfs_err),
            BUFFER_ENCODE_ARG(blocksLeftAfterWrite));

        // Use command_data fields since slate may not be initialized
        ftp_send_result_packet_custom_file(
            slate, command_data.fname_str, command_data.file_len,
            command_data.file_crc, FTP_ERROR_START_FILE_WRITE, err_data, len);
        return;
    }

    if (blocksLeftAfterWrite < 0)
    {
        LOG_ERROR("[FTP] Could not determine blocks left after file write.");

        uint8_t err_data[PACKET_DATA_SIZE];
        size_t len = buffer_encode_data(
            err_data, BUFFER_ENCODE_ARG(res), BUFFER_ENCODE_ARG(lfs_err),
            BUFFER_ENCODE_ARG(blocksLeftAfterWrite));

        // Use command_data fields since slate may not be initialized
        ftp_send_result_packet_custom_file(
            slate, command_data.fname_str, command_data.file_len,
            command_data.file_crc, FTP_ERROR_START_FILE_WRITE, err_data, len);
        return;
    }

    slate->ftp_start_cycle_packet_id = 0;
    ftp_tracker_clear(&slate->ftp_packets_received_tracker);
    slate->ftp_last_status_report_time = get_absolute_time();

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

    // packet_index is the 0-based index within the current cycle (0 to
    // FTP_NUM_PACKETS_PER_CYCLE - 1)
    const FTP_PACKET_SEQUENCE_T packet_index =
        command_data.packet_id % FTP_NUM_PACKETS_PER_CYCLE;

    // Check for duplicate packet
    if (ftp_tracker_check_bit(&slate->ftp_packets_received_tracker,
                              packet_index))
    {
        LOG_INFO("[FTP] Duplicate packet ID %d received, ignoring.",
                 command_data.packet_id);
        return;
    }

    // 1. Write data to buffer at correct offset
    const FILESYS_BUFFER_SIZE_T offset = packet_index * FTP_DATA_PAYLOAD_SIZE;

    // Write data to buffer
    lfs_ssize_t lfs_err = LFS_ERR_OK;
    filesys_error_t write_res = filesys_write_data_to_buffer(
        slate, command_data.data, command_data.data_len, offset, &lfs_err);

    if (write_res != FILESYS_OK)
    {
        LOG_ERROR("[FTP] Failed to write data to buffer: %d", write_res);
        send_ftp_lfs_error_packet(slate, FTP_FILE_WRITE_BUFFER_ERROR, write_res,
                                  lfs_err);
        return;
    }

    // 2. Update received tracker
    ftp_tracker_set_bit(&slate->ftp_packets_received_tracker, packet_index);

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

    LOG_DEBUG("[FTP] Packet %d received. Expecting %d packets in this cycle.",
              command_data.packet_id, packets_in_this_cycle);

    // If not the last in the cycle, return (status reports are sent
    // periodically)
    if (!ftp_tracker_check_mask_completed(&slate->ftp_packets_received_tracker,
                                          packets_in_this_cycle))
    {
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
        const FILESYS_BUFFERED_FILE_LEN_T bytes_previously_written =
            (FILESYS_BUFFERED_FILE_LEN_T)slate->ftp_start_cycle_packet_id *
            FTP_DATA_PAYLOAD_SIZE;
        bytes_to_write =
            slate->filesys_buffered_file_len - bytes_previously_written;
    }

    lfs_ssize_t mram_lfs_err = LFS_ERR_OK;
    filesys_error_t mram_write_res =
        filesys_write_buffer_to_mram(slate, bytes_to_write, &mram_lfs_err);

    if (mram_write_res != FILESYS_OK)
    {
        LOG_ERROR("[FTP] Failed to write buffer to MRAM: %d", mram_write_res);
        // Send error packet with the error code
        send_ftp_lfs_error_packet(slate, FTP_FILE_WRITE_MRAM_ERROR,
                                  mram_write_res, mram_lfs_err);
        return;
    }

    // Reset received tracker for next cycle
    ftp_tracker_clear(&slate->ftp_packets_received_tracker);

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
    lfs_ssize_t crc_lfs_err = LFS_ERR_OK;
    filesys_error_t crc_compute_err = FILESYS_OK;
    unsigned int computed_crc =
        filesys_compute_crc(slate, &crc_compute_err, &crc_lfs_err);

    lfs_ssize_t complete_lfs_err = LFS_ERR_OK;
    filesys_error_t complete_res =
        filesys_complete_file_write(slate, &complete_lfs_err);

    if (complete_res != FILESYS_OK)
    {
        LOG_ERROR("[FTP] Error completing file write: %d", complete_res);
        // Determine error type
        if (complete_res == FILESYS_ERR_CRC_MISMATCH)
        {
            // Compute on-disk file length (not the expected/received length)
            lfs_ssize_t crc_info_lfs_err = LFS_ERR_OK;
            filesys_file_info_t crc_file_info;
            filesys_error_t crc_info_res =
                filesys_get_file_info(slate, slate->filesys_buffered_fname_str,
                                      &crc_file_info, &crc_info_lfs_err);

            FILESYS_BUFFERED_FILE_LEN_T file_len_on_disk =
                (crc_info_res == FILESYS_OK) ? crc_file_info.file_size : 0;

            uint8_t crc_err_data[PACKET_DATA_SIZE];
            size_t len = buffer_encode_data(
                crc_err_data, BUFFER_ENCODE_ARG(computed_crc),
                BUFFER_ENCODE_ARG(file_len_on_disk));

            ftp_send_result_packet(slate, FTP_EOF_CRC_ERROR, crc_err_data, len);
        }
        else
        {
            send_ftp_lfs_error_packet(slate, FTP_FILE_WRITE_MRAM_ERROR,
                                      complete_res, complete_lfs_err);
        }

        return;
    }

    // Success!
    LOG_INFO("[FTP] File write complete & successful! Computed CRC: 0x%X",
             computed_crc);

    // Get file length on disk
    lfs_ssize_t info_lfs_err = LFS_ERR_OK;
    filesys_file_info_t file_info;
    filesys_error_t info_res = filesys_get_file_info(
        slate, slate->filesys_buffered_fname_str, &file_info, &info_lfs_err);
    FILESYS_BUFFERED_FILE_LEN_T file_len_on_disk =
        (info_res == FILESYS_OK) ? file_info.file_size : 0;

    uint8_t success_data[PACKET_DATA_SIZE];
    size_t len =
        buffer_encode_data(success_data, BUFFER_ENCODE_ARG(computed_crc),
                           BUFFER_ENCODE_ARG(file_len_on_disk));

    // Note slate->filesys_is_writing_file is false at this point, so we cannot
    // use ftp_send_result_packet (which would use _no_file since it checks that
    // flag).
    ftp_send_result_packet_custom_file(slate, slate->filesys_buffered_fname_str,
                                       slate->filesys_buffered_file_len,
                                       slate->filesys_buffered_file_crc,
                                       FTP_EOF_SUCCESS, success_data, len);
}

void ftp_process_file_cancel_write_command(
    slate_t *slate, FTP_CANCEL_FILE_WRITE_DATA command_data)
{
    // Capture file headers before cancel clears slate state
    FILESYS_BUFFERED_FNAME_STR_T fname_str;
    memcpy(fname_str, slate->filesys_buffered_fname_str, sizeof(fname_str));
    FILESYS_BUFFERED_FILE_LEN_T file_len = slate->filesys_buffered_file_len;
    FILESYS_BUFFERED_FILE_CRC_T file_crc = slate->filesys_buffered_file_crc;

    lfs_ssize_t lfs_err = LFS_ERR_OK;
    filesys_error_t res = filesys_cancel_file_write(slate, &lfs_err);

    if (res != FILESYS_OK)
    {
        LOG_ERROR("[FTP] Failed to cancel file write with code %d.", res);
        send_ftp_lfs_error_packet(slate, FTP_CANCEL_ERROR, res, lfs_err);
        return;
    }

    LOG_INFO("[FTP] Successfully cancelled file write for file %s.",
             command_data.fname_str);

    ftp_send_result_packet_custom_file(slate, fname_str, file_len, file_crc,
                                       FTP_CANCEL_SUCCESS, NULL, 0);
}

/**
 * Sends a periodic FTP_STATUS_REPORT packet with cycle info, CRC progress,
 * total bytes written, and file write state.
 */
inline static void send_ftp_status_report(slate_t *slate)
{
    // CRC computed over bytes written to MRAM so far (0 if first cycle)
    FILESYS_BUFFERED_FILE_CRC_T file_crc_so_far = 0;
    if (slate->ftp_start_cycle_packet_id > 0)
    {
        lfs_ssize_t crc_lfs_err = LFS_ERR_OK;
        filesys_error_t crc_err = FILESYS_OK;
        file_crc_so_far = filesys_compute_crc(slate, &crc_err, &crc_lfs_err);
        if (crc_err != FILESYS_OK)
            file_crc_so_far = 0;
    }

    // Total bytes written to MRAM so far
    FILESYS_BUFFERED_FILE_LEN_T total_bytes_written =
        (FILESYS_BUFFERED_FILE_LEN_T)slate->ftp_start_cycle_packet_id *
        FTP_DATA_PAYLOAD_SIZE;

    uint8_t status_report_data[PACKET_DATA_SIZE];
    size_t len = buffer_encode_data(
        status_report_data, BUFFER_ENCODE_ARG(slate->ftp_start_cycle_packet_id),
        BUFFER_ENCODE_ARG(ftp_get_last_packet(slate)),
        BUFFER_ENCODE_RAW(FTP_PACKET_TRACKER_SIZE,
                          slate->ftp_packets_received_tracker),
        BUFFER_ENCODE_ARG(file_crc_so_far),
        BUFFER_ENCODE_ARG(total_bytes_written),
        BUFFER_ENCODE_ARG((slate->filesys_buffer != NULL) ? 1 : 0),
        BUFFER_ENCODE_ARG(slate->filesys_is_writing_file ? 1 : 0));

    ftp_send_result_packet_custom_file(
        slate, slate->filesys_buffered_fname_str,
        slate->filesys_buffered_file_len, slate->filesys_buffered_file_crc,
        FTP_STATUS_REPORT, status_report_data, len);
}

void ftp_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(FTP_TASK_COLOR);
    LOG_INFO("[FTP] FTP task is running...");

    if (!queue_is_empty(&slate->ftp_format_filesystem_data))
    {
        ftp_process_reformat_command(slate);

        uint8_t dummy;
        if (!queue_try_remove(&slate->ftp_format_filesystem_data, &dummy))
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

    // Send periodic status reports during active file transfer
    if (slate->filesys_is_writing_file)
    {
        uint64_t elapsed_us = absolute_time_diff_us(
            slate->ftp_last_status_report_time, get_absolute_time());
        if (elapsed_us >= FTP_STATUS_REPORT_INTERVAL_MS * 1000ULL)
        {
            send_ftp_status_report(slate);
            slate->ftp_last_status_report_time = get_absolute_time();
        }
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
