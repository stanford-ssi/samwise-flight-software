/**
 * @author Ayush Garg
 * @date 2025-10-29
 * @brief Header file for the filesystem module.
 *
 * This module provides functions to initialize and manage the filesystem,
 * including writing files to MRAM with buffering support.
 *
 * Note: Read, delete, and other filesystem operations are implemented in
 * little-fs. Also note that only 2-byte file names are allowed, and so
 * directories are not supported. Files must be uniquely named (2^16 files max).
 */

#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "crc32.h"

#include "lfs.h"
#include "lfs_mram_wrapper.h"
#include "logger.h"
#include "macros.h"
#include "slate.h"
#include "state_machine.h"
#include "stdint.h"
#include "str_utils.h"
#include "typedefs.h"

/* Error codes for filesys operations */
enum filesys_error
{
    FILESYS_OK = 0,                        // Operation completed successfully
    FILESYS_ERR_FILE_ALREADY_WRITING = -1, // A file is already being written
    FILESYS_ERR_GET_FS_SIZE = -2,          // Failed to get filesystem size
    FILESYS_ERR_NOT_ENOUGH_SPACE = -3,     // Insufficient space on filesystem
    FILESYS_ERR_OPEN_FILE = -4,            // Failed to open file
    FILESYS_ERR_SET_CRC_ATTR = -5,         // Failed to set CRC attribute
    FILESYS_ERR_BUFFER_DIRTY = -6,         // Buffer contains unwritten data
    FILESYS_ERR_EXCEED_BUFFER = -7,        // Write exceeded buffer size
    FILESYS_ERR_CLOSE_FILE = -8,           // Failed to close file
    FILESYS_ERR_CRC_CHECK = -9,            // Error occurred during CRC check
    FILESYS_ERR_CRC_MISMATCH = -10,        // CRC does not match expected value
    FILESYS_ERR_NO_FILE_WRITING = -11,     // No file is currently being written
    FILESYS_ERR_DELETE_FILE = -12,         // Failed to delete file
    FILESYS_ERR_REFORMAT = -13,            // Failed to reformat the filesystem
    FILESYS_ERR_MOUNT = -14,               // Failed to mount the filesystem
    FILESYS_ERR_WRITE_MRAM = -15,          // Failed to write to MRAM
}

#define FILESYS_CFG_CACHE_SIZE 16
#define FILESYS_CFG_LOOKAHEAD_SIZE 16

// Prevent the use of MALLOC (BAD) by LFS!!!
static uint8_t prog_buffer[FILESYS_CFG_CACHE_SIZE];
static uint8_t read_buffer[FILESYS_CFG_CACHE_SIZE];
static uint8_t cache_buffer[FILESYS_CFG_CACHE_SIZE];
static uint8_t lookahead_buffer[FILESYS_CFG_LOOKAHEAD_SIZE];

// configuration of the filesystem is provided by this struct
extern const struct lfs_config cfg;

/**
 * Mounts the filesystem & initializes the overall filesys system.
 * This MUST be run before any other filesystem operations.
 *
 * @return A negative error code on failure.
 */
int8_t filesys_initialize(slate_t *slate);

/**
 * Formats the filesystem on MRAM.
 * This is a destructive operation that erases all data on the filesystem.
 *
 * @param slate Pointer to the slate structure.
 * @return A negative error code on failure.
 */
int8_t filesys_reformat(slate_t *slate);

/**
 * Initializes writing to a file in the filesystem.
 *
 * @param slate Pointer to the slate structure.
 * @param fname The name of the file to buffer.
 * @param file_size The size of the file to buffer.
 * @param file_crc The CRC of the file to buffer.
 * @param blocksLeftAfterWrite Pointer to store the amount of blocks left
 * after the write. Note if this is negative, there is not enough space to write
 * the file. This is not written if the function returns -1 or -2.
 * @return FILESYS_ERR_FILE_ALREADY_WRITING if a file is already being written,
 *         FILESYS_ERR_GET_FS_SIZE if there was an error getting the filesystem
 * size, FILESYS_ERR_NOT_ENOUGH_SPACE if there is not enough space to write the
 * file, FILESYS_ERR_OPEN_FILE if there was an error opening the file for
 * writing/appending, FILESYS_ERR_SET_CRC_ATTR if there was an error setting the
 * CRC attribute, FILESYS_OK on success.
 */
int8_t filesys_start_file_write(slate_t *slate,
                                FILESYS_BUFFERED_FNAME_STR_T fname_str,
                                FILESYS_BUFFERED_FILE_LEN_T file_size,
                                FILESYS_BUFFERED_FILE_CRC_T file_crc,
                                lfs_ssize_t *blocksLeftAfterWrite);

/**
 * Writes data to the current file buffer at the specified offset.
 *
 * @param slate Pointer to the slate structure.
 * @param data Pointer to the data to write.
 * @param n_bytes The number of bytes to write. (Must not exceed
 * FILESYS_BUFFER_SIZE).
 * @param offset The offset in the buffer to start writing at. (Must not exceed
 * FILESYS_BUFFER_SIZE).
 */
int8_t filesys_write_data_to_buffer(slate_t *slate, const uint8_t *data,
                                    FILESYS_BUFFER_SIZE_T n_bytes,
                                    FILESYS_BUFFER_SIZE_T offset);

/**
 * Writes the current buffered state to MRAM as a block, and mark the buffer
 * as clean. Note this ALWAYS appends to the end of the file.
 *
 * @param slate Pointer to the slate structure.
 * @param n_bytes The number of bytes to write for this buffer. Use
 * FILESYS_BUFFER_SIZE to write the entire buffer to MRAM.
 * @return The number of bytes written, or a negative error code on failure.
 */
int8_t filesys_write_buffer_to_mram(slate_t *slate,
                                    FILESYS_BUFFER_SIZE_T n_bytes);

/**
 * Computes the CRC of the file currently being written.
 *
 * @param slate Pointer to the slate structure.
 * @param error_code Pointer to store error code in case of failure, or
 * FILESYS_OK on success.
 * @return The computed CRC value.
 */
unsigned int filesys_compute_crc(slate_t *slate, int8_t *error_code);

/**
 * Validates the CRC of the file currently being written against the stored CRC
 * (on _CRC attribute).
 *
 * @param slate Pointer to the slate structure.
 * @return FILESYS_CRC_CORRECT if the CRC is correct,
 *         FILESYS_CRC_INCORRECT if incorrect,
 *         FILESYS_CRC_NO_FILE if no file is being written.
 */
int8_t filesys_is_crc_correct(slate_t *slate);

/**
 * Marks the filesystem as no longer writing a file. If the buffer is currently
 * dirty, it returns false, and you must either clear the current buffer or
 * write it to MRAM before completing.
 *
 * @param slate Pointer to the slate structure.
 * @return FILESYS_ERR_BUFFER_DIRTY if the buffer is dirty,
 *         FILESYS_ERR_CLOSE_FILE if there was an error closing the file,
 *         FILESYS_ERR_CRC_CHECK if there was an error during CRC check,
 *         FILESYS_ERR_CRC_MISMATCH if the CRC did not match,
 *         FILESYS_OK on success.
 */
int8_t filesys_complete_file_write(slate_t *slate);

/**
 * Marks the current buffer as clean. DESTRUCTIVE OPERATION.
 *
 * @param slate Pointer to the slate structure.
 */
void filesys_clear_buffer(slate_t *slate);

/**
 * Clears the file buffer & marks the filesystem as no longer writing a file,
 * with no checking for data on the buffer.
 * This also deletes/"frees" what was written so far on MRAM, thereby
 * completely cancelling the write operation. DESTRUCTIVE OPERATION.
 *
 * @param slate Pointer to the slate structure.
 * @return FILESYS_ERR_NO_FILE_WRITING if no file is being written,
 *         FILESYS_ERR_CLOSE_FILE if there was an error closing the file,
 *         FILESYS_ERR_DELETE_FILE if there was an error deleting the file,
 *         FILESYS_OK on success.
 */
int8_t filesys_cancel_file_write(slate_t *slate);
