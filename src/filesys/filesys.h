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

#include <stdio.h>
#include <string.h>

#include "lfs.h"
#include "logger.h"
#include "macros.h"
#include "slate.h"
#include "state_machine.h"
#include "stdint.h"
#include "typedefs.h"

/**
 * Mounts the filesystem & initializes the overall filesys system.
 * This MUST be run before any other filesystem operations.
 *
 * @return A negative error code on failure.
 */
lfs_ssize_t filesys_initialize(void);

/**
 * Initializes writing to a file in the filesystem.
 *
 * @param slate Pointer to the slate structure.
 * @param fname The name of the file to buffer.
 * @param file_size The size of the file to buffer.
 * @param file_crc The CRC of the file to buffer.
 */
void filesys_start_file_write(slate_t *slate, FILESYS_BUFFERED_FNAME_T fname,
                              FILESYS_BUFFERED_FILE_LEN_T file_size,
                              FILESYS_BUFFERED_FILE_CRC_T file_crc);

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
void filesys_write_data_to_buffer(slate_t *slate, const uint8_t *data,
                                  FILESYS_BUFFER_SIZE_T n_bytes,
                                  FILESYS_BUFFER_SIZE_T offset);

/**
 * Writes the current buffered state to MRAM as a block, and mark the buffer
 * as clean. Note this ALWAYS appends to the end of the file.
 *
 * @param slate Pointer to the slate structure.
 * @param fname The name of the file to write (for identification purposes).
 * @param n_bytes The number of bytes to write for this buffer. Use
 * FILESYS_BUFFER_SIZE to write the entire buffer to MRAM.
 * @return The number of bytes written, or a negative error code on failure.
 */
lfs_ssize_t filesys_write_buffer_to_mram(slate_t *slate,
                                         FILESYS_BUFFERED_FNAME_T fname,
                                         FILESYS_BUFFER_SIZE_T n_bytes);

/**
 * Marks the filesystem as no longer writing a file. If the buffer is currently
 * dirty, it returns false, and you must either clear the current buffer or
 * write it to MRAM before completing.
 *
 * @param slate Pointer to the slate structure.
 * @return true if successfully completed, false if buffer is dirty.
 */
bool filesys_complete_file_write(slate_t *slate);

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
 * @return A negative error code on failure.
 */
lfs_ssize_t filesys_cancel_file_write(slate_t *slate);
