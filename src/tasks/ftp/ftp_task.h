/**
 * @author Ayush Garg
 * @date  2025-10-29
 */
#pragma once

#include "command_parser.h"
#include "config.h"
#include "macros.h"
#include "payload_uart.h"
#include "scheduler.h"
#include "slate.h"

// LED Color for ftp task - Hot Pink
#define FTP_TASK_COLOR 255, 105, 180

#define MAX_FTP_COMMANDS_PER_DISPATCH 3
#define MAX_FTP_RETRY_COUNT 3

/**
 * Every packet from SAT -> Ground has these headers:
 *
 * Headers (FILESYS_BUFFERED_FNAME_T + FTP_FILE_LEN_T +
 * FILESYS_BUFFERED_FILE_CRC_T) FTP_Result (FTP_RESULT_MNEMONIC_SIZE)
 *
 * Additional Data (varies based on FTP_Result) is written in comments below.
 */
typedef enum
{
    /**
     *  FTP_INIT_ERROR:
     *      LFS_Error_Code (lfs_ssize_t) - LittleFS error code or similar
     *      Data (char[]) - Additional error data (optional, plaintext)
     */
    FILESYS_INIT_ERROR = 0, // Filesystem not initialized properly

    /**
     * FTP_READY_RECIEVE or FTP_FILE_WRITE_SUCCESS:
     *     Packet_Start (FTP_PACKET_ID_T) - First accepted packet ID in set
     * (inclusive)
     *     Packet_End (FTP_PACKET_ID_T) - Last accepted packet ID in set
     * (inclusive)
     */
    FTP_READY_RECIEVE = 1, // Ready to start receiving file packets
    FTP_FILE_WRITE_SUCCESS =
        2, // Recieved FTP_NUM_PACKETS_PER_CYCLE packets successfully

    /**
     * FTP_EOF_SUCCESS:
     *     Computed_CRC (FILESYS_BUFFERED_FILE_CRC_T) - CRC computed over entire
     * file
     *     File_Length_Disk (FTP_FILE_LEN_T) - Length of file as stored on disk
     */
    FTP_EOF_SUCCESS =
        3, // File transfer completed successfully with correct CRC(!)

    /**
     * FTP_CANCEL_SUCCESS:
     *     (No additional data)
     */
    FTP_CANCEL_SUCCESS = 5, // File transfer cancelled successfully

    /**
     * FTP_FILE_WRITE_ERROR or FTP_CANCEL_ERROR:
     *     LFS_Error_Code (lfs_ssize_t) - LittleFS error code or similar
     *     Data (char[]) - Additional error data (optional, plaintext)
     */
    FTP_FILE_WRITE_MRAM_ERROR =
        -3, // Error writing buffered data to MRAM (LittleFS error or similar)
    FTP_CANCEL_ERROR = -5, // Error cancelling file write

    /**
     * FTP_EOF_CRC_ERROR:
     *     Computed_CRC (FILESYS_BUFFERED_FILE_CRC_T) - CRC computed over entire
     * file
     *     File_Length_Disk (FTP_FILE_LEN_T) - Length of file as stored on disk
     *     Data (char[]) - Additional error data (optional, plaintext)
     */
    FTP_EOF_CRC_ERROR = -3, // Error: CRC mismatch at EOF - any other EOF error
                            // goes to FTP_FILE_WRITE_ERROR

    /**
     * All other errors:
     *     Data (char[]) - Additional error data (optional, plaintext)
     */
    FTP_ERROR_RECIEVE = -1,           // Error initializing file write
    FTP_FILE_WRITE_BUFFER_ERROR = -4, // Error writing file data to buffer
    FTP_ERROR = -127,                 // Generic error
} FTP_Result;

// number of bytes used to identify FTP result
#define FTP_RESULT_MNEMONIC_SIZE 1

void ftp_task_init(slate_t *slate);
void ftp_task_dispatch(slate_t *slate);

/* COMMANDS */
/**
 * Processes an FTP_START_FILE_WRITE command. Initializes file write state.
 *
 * @param slate Pointer to the slate structure.
 * @param command_data Pointer to the command data.
 */
void ftp_process_file_start_write_command(
    slate_t *slate, const FTP_START_FILE_WRITE_DATA *command_data);

/**
 * Processes an FTP_WRITE_FILE_DATA command. If successful, writes data to
 * buffer; if buffer is full, writes buffer to MRAM and sends back
 * FTP_FILE_WRITE_SUCCESS so the next set of packets can be sent.
 *
 * @param slate Pointer to the slate structure.
 * @param command_data Pointer to the command data.
 */
void ftp_process_file_write_data_command(
    slate_t *slate, const FTP_WRITE_TO_FILE_DATA *command_data);

/**
 * Processes an FTP_CANCEL_FILE_WRITE command. Cancels the ongoing file write,
 * deleting all data destructively in the process. (I.e. deallocates buffers,
 * the data is not zeroed out).
 *
 * @param slate Pointer to the slate structure.
 * @param command_data Pointer to the command data.
 */
void ftp_process_file_cancel_write_command(
    slate_t *slate, const FTP_CANCEL_FILE_WRITE_DATA *command_data);

/**
 * Processes an FTP_FORMAT_FILESYSTEM command. Formats the filesystem on MRAM.
 * This is a destructive operation that erases all data on the filesystem.
 *
 * @param slate Pointer to the slate structure.
 */
void ftp_process_format_filesystem_command(slate_t *slate);

/* HELPER FUNCTIONS */
/**
 * Sends an FTP result packet back to ground, after running one of these
 * commands.
 *
 * @param slate Pointer to the slate structure.
 * @param result FTP result code.
 * @param additional_data Pointer to additional data (see FTP_Result comments
 * for details).
 * @param additional_data_len Length of the additional data in bytes.
 */
void ftp_send_result_packet(slate_t *slate, FTP_Result result,
                            const void *additional_data, // See FTP_Result
                                                         // comments for details
                            size_t additional_data_len);

extern sched_task_t ftp_task;
