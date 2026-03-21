/**
 * @author Ayush Garg
 * @date 2025-10-29
 * @brief Header file for the filesystem module.
 *
 * This module provides functions to initialize and manage the filesystem,
 * including writing files to MRAM with buffering support and reading files
 * from MRAM with CRC verification.
 *
 * Note: Delete and other filesystem operations are implemented in
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
    FILESYS_ERR_MALLOC = -15,              // Failed to allocate memory
    FILESYS_ERR_WRITE_MRAM = -16,          // Failed to write to MRAM
    FILESYS_ERR_OPEN_DIR = -17,            // Failed to open directory
    FILESYS_ERR_READ_DIR = -18,            // Failed to read directory entry
    FILESYS_ERR_CLOSE_DIR = -19,           // Failed to close directory
    FILESYS_ERR_GET_CRC_ATTR = -20,        // Failed to get CRC attribute
    FILESYS_ERR_READ_FILE = -21,           // Failed to read from file
    FILESYS_ERR_SEEK_FILE = -22,           // Failed to seek in file
    FILESYS_ERR_FILE_SIZE = -23,           // Failed to get file size
};

typedef int32_t filesys_error_t;

// Size of block caches in bytes. Each cache buffers a portion of a block in
// RAM. The littlefs needs a read cache, a program cache, and one additional
// cache per file. Larger caches can improve performance by storing more
// data and reducing the number of disk accesses. Must be a multiple of the
// read and program sizes, and a factor of the block size.
// Recommended to keep it small to save RAM.
#define FILESYS_CFG_CACHE_SIZE 16

// Size of the lookahead buffer in bytes. A larger lookahead buffer
// increases the number of blocks found during an allocation pass. The
// lookahead buffer is stored as a compact bitmap, so each byte of RAM
// can track 8 blocks.
// Recommended to keep it small to save RAM.
#define FILESYS_CFG_LOOKAHEAD_SIZE 16

// Prevent the use of MALLOC (BAD) by LFS!!!
static uint8_t prog_buffer[FILESYS_CFG_CACHE_SIZE] = {0};
static uint8_t read_buffer[FILESYS_CFG_CACHE_SIZE] = {0};
static uint8_t cache_buffer[FILESYS_CFG_CACHE_SIZE] = {0};
static uint8_t lookahead_buffer[FILESYS_CFG_LOOKAHEAD_SIZE] = {0};

// configuration of the filesystem is provided by this struct
extern const struct lfs_config filesys_lfs_cfg;

// configuration of file operations (always use lfs_file_opencfg with this)
extern const struct lfs_file_config filesys_lfs_file_cfg;

/**
 * Mounts the filesystem & initializes the overall filesys system.
 * This MUST be run before any other filesystem operations.
 *
 * @param slate Pointer to the slate structure.
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error. Note that LFS can be OK but
 * filesys can still fail (e.g. not enough space in buffer).
 * @return A negative error code on failure.
 */
filesys_error_t filesys_initialize(slate_t *slate, lfs_ssize_t *lfs_error_code);

/**
 * Formats the filesystem on MRAM.
 * This is a destructive operation that erases all data on the filesystem.
 *
 * @param slate Pointer to the slate structure.
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error. Note that LFS can be OK but
 * filesys can still fail (e.g. not enough space in buffer).
 * @return A negative error code on failure.
 */
filesys_error_t filesys_reformat_initialize(slate_t *slate,
                                            lfs_ssize_t *lfs_error_code);

// TODO: Make all return statements make sense, including error_code
/**
 * Initializes writing to a file in the filesystem.
 *
 * @param slate Pointer to the slate structure.
 * @param fname The name of the file to buffer.
 * @param file_size The size of the file to buffer.
 * @param file_crc The CRC of the file to buffer.
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error. Note that LFS can be OK but
 * filesys can still fail (e.g. not enough space in buffer).
 * @param blocks_left_after_write Pointer to store the amount of blocks left
 * after the write. Note if this is negative, there is not enough space to write
 * the file. This is not written if the function returns -1 or -2.
 * @return // FILESYS_ERR_FILE_ALREADY_WRITING if a file is already being
 * written,
 * // FILESYS_ERR_GET_FS_SIZE if there was an error getting the filesystem size,
 * // FILESYS_ERR_NOT_ENOUGH_SPACE if there is not enough space to write the
 * file,
 * // FILESYS_ERR_OPEN_FILE if there was an error opening the file for
 * writing/appending,
 * // FILESYS_ERR_SET_CRC_ATTR if there was an error setting the CRC attribute,
 * // FILESYS_OK on success.
 */
filesys_error_t filesys_start_file_write(
    slate_t *slate, const FILESYS_BUFFERED_FNAME_STR_T fname_str,
    FILESYS_BUFFERED_FILE_LEN_T file_size, FILESYS_BUFFERED_FILE_CRC_T file_crc,
    lfs_ssize_t *lfs_error_code, lfs_ssize_t *blocks_left_after_write);

/**
 * Writes data to the current file buffer at the specified offset.
 *
 * @param slate Pointer to the slate structure.
 * @param data Pointer to the data to write.
 * @param n_bytes The number of bytes to write. (Must not exceed
 * FILESYS_BUFFER_SIZE).
 * @param offset The offset in the buffer to start writing at. (Must not exceed
 * FILESYS_BUFFER_SIZE).
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error. Note that LFS can be OK but
 * filesys can still fail (e.g. not enough space in buffer).
 */
filesys_error_t filesys_write_data_to_buffer(slate_t *slate,
                                             const uint8_t *data,
                                             FILESYS_BUFFER_SIZE_T n_bytes,
                                             FILESYS_BUFFER_SIZE_T offset,
                                             lfs_ssize_t *lfs_error_code);

/**
 * Writes the current buffered state to MRAM as a block, and mark the buffer
 * as clean. Note this ALWAYS appends to the end of the file.
 *
 * @param slate Pointer to the slate structure.
 * @param n_bytes The number of bytes to write for this buffer. Use
 * FILESYS_BUFFER_SIZE to write the entire buffer to MRAM.
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error. Note that LFS can be OK but
 * filesys can still fail (e.g. not enough space in buffer).
 * @return The number of bytes written, or a negative error code on failure.
 */
filesys_error_t filesys_write_buffer_to_mram(slate_t *slate,
                                             FILESYS_BUFFER_SIZE_T n_bytes,
                                             lfs_ssize_t *lfs_error_code);

/**
 * Returns a pointer to the internal lfs_t singleton.
 * Intended for test code that needs direct LFS access.
 */
lfs_t *filesys_get_lfs(void);

/**
 * Computes the CRC of the file currently being written.
 *
 * @param slate Pointer to the slate structure.
 * @param error_code Pointer to store error code in case of failure, or
 * FILESYS_OK on success.
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error. Note that LFS can be OK but
 * filesys can still fail (e.g. not enough space in buffer).
 * @return The computed CRC value.
 */
unsigned int filesys_compute_crc(slate_t *slate, filesys_error_t *error_code,
                                 lfs_ssize_t *lfs_error_code);

/**
 * Validates the CRC of the file currently being written against the stored CRC
 * (on _CRC attribute).
 *
 * @param slate Pointer to the slate structure.
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error. Note that LFS can be OK but
 * filesys can still fail (e.g. not enough space in buffer).
 * @return FILESYS_CRC_CORRECT if the CRC is correct,
 *         FILESYS_CRC_INCORRECT if incorrect,
 *         FILESYS_CRC_NO_FILE if no file is being written.
 */
filesys_error_t filesys_is_crc_correct(slate_t *slate,
                                       lfs_ssize_t *lfs_error_code);

/**
 * Marks the filesystem as no longer writing a file. If the buffer is currently
 * dirty, it returns false, and you must either clear the current buffer or
 * write it to MRAM before completing.
 *
 * @param slate Pointer to the slate structure.
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error. Note that LFS can be OK but
 * filesys can still fail (e.g. not enough space in buffer).
 * @return // FILESYS_ERR_BUFFER_DIRTY if the buffer is dirty,
 *         // FILESYS_ERR_CLOSE_FILE if there was an error closing the file,
 *         // FILESYS_ERR_CRC_CHECK if there was an error during CRC check,
 *         // FILESYS_ERR_CRC_MISMATCH if the CRC did not match,
 *         // FILESYS_OK on success.
 */
filesys_error_t filesys_complete_file_write(slate_t *slate,
                                            lfs_ssize_t *lfs_error_code);

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
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error. Note that LFS can be OK but
 * filesys can still fail (e.g. not enough space in buffer).
 * @return FILESYS_ERR_NO_FILE_WRITING if no file is being written,
 *         FILESYS_ERR_CLOSE_FILE if there was an error closing the file,
 *         FILESYS_ERR_DELETE_FILE if there was an error deleting the file,
 *         FILESYS_OK on success.
 */
filesys_error_t filesys_cancel_file_write(slate_t *slate,
                                          lfs_ssize_t *lfs_error_code);

/**
 * Information about a single file on the filesystem.
 * Populated by filesys_list_files for each file found.
 *
 * Packed to minimize memory footprint. Boolean flags are stored in a single
 * uint8_t using explicit bit masks so the layout is deterministic across
 * compilers.
 */

// Bit masks for filesys_file_info_t.flags
#define FILESYS_FILE_INFO_CRC_MATCH 0x01          // computed == expected CRC
#define FILESYS_FILE_INFO_COMPUTED_CRC_VALID 0x02 // computed CRC is valid
#define FILESYS_FILE_INFO_EXPECTED_CRC_VALID 0x04 // expected CRC attr was read

typedef struct __attribute__((packed))
{
    FILESYS_BUFFERED_FNAME_STR_T fname;    // File name (null-terminated)
    FILESYS_BUFFERED_FILE_LEN_T file_size; // File size on disk (bytes)
    FILESYS_BUFFERED_FILE_CRC_T
    computed_crc; // CRC32 computed from on-disk data
    FILESYS_BUFFERED_FILE_CRC_T expected_crc; // CRC32 stored as file attribute
    uint8_t flags; // Bitfield, see FILESYS_FILE_INFO_*
} filesys_file_info_t;

/**
 * Retrieves information about a single file, including its size, stored CRC
 * attribute, and computed on-disk CRC. The result is written into a
 * caller-provided filesys_file_info_t whose flags indicate which fields are
 * valid.
 *
 * This is the shared helper used by both filesys_list_files and
 * filesys_open_file_read.
 *
 * @param slate Pointer to the slate structure.
 * @param fname Null-terminated filename to query.
 * @param info Pointer to a caller-allocated filesys_file_info_t to populate.
 * @param lfs_error_code Pointer to store the first LFS error code encountered.
 *        LFS_ERR_OK if no LFS error occurred.
 * @return FILESYS_OK on success (info is fully populated),
 *         FILESYS_ERR_OPEN_FILE if the file could not be opened to get its
 *         size,
 *         FILESYS_ERR_FILE_SIZE if the file size could not be determined,
 *         FILESYS_ERR_CLOSE_FILE if the temporary file handle could not be
 *         closed.
 *         Note: CRC attribute or computation failures are recorded in
 *         info->flags rather than causing this function to return an error.
 */
filesys_error_t filesys_get_file_info(slate_t *slate,
                                      const FILESYS_BUFFERED_FNAME_STR_T fname,
                                      filesys_file_info_t *info,
                                      lfs_ssize_t *lfs_error_code);

/**
 * Lists all files on the filesystem, computing each file's on-disk CRC32
 * and retrieving its stored (expected) CRC32 attribute. Results are written
 * into the caller-provided array.
 *
 * If CRC computation or attribute retrieval fails for a particular file, the
 * corresponding _valid flag in filesys_file_info_t is set to 0 and listing
 * continues.
 *
 * @param slate Pointer to the slate structure.
 * @param file_list Pointer to a caller-allocated array of filesys_file_info_t
 *        that will be populated with file information.
 * @param max_files Maximum number of entries that file_list can hold (array
 *        capacity). If there are more files on disk than max_files, only
 *        max_files entries are written.
 * @param num_files_found Pointer to a variable that will receive the total
 *        number of files written to file_list.
 * @param lfs_error_code Pointer to store the first LFS error code encountered.
 *        LFS_ERR_OK if no LFS error occurred.
 * @return FILESYS_OK on success,
 *         FILESYS_ERR_OPEN_DIR if the root directory could not be opened,
 *         FILESYS_ERR_READ_DIR if a directory read failed,
 *         FILESYS_ERR_CLOSE_DIR if the directory could not be closed.
 */
filesys_error_t filesys_list_files(slate_t *slate,
                                   filesys_file_info_t *file_list,
                                   uint16_t max_files,
                                   uint16_t *num_files_found,
                                   lfs_ssize_t *lfs_error_code);

/* ===== Read Operations ===== */
/* These functions provide a standardized interface for reading files from MRAM.
 * CRC integrity is verified when a file is opened for reading, so every read
 * session is guaranteed to start from a file whose on-disk data matches its
 * stored CRC attribute. The caller owns the lfs_file_t object and must pass it
 * to every subsequent read operation. */

/**
 * Opens a file for reading and verifies its CRC32 integrity.
 *
 * The file's on-disk data is read to compute a CRC32 which is compared against
 * the stored CRC attribute. If the CRC does not match, the file is not opened
 * and an error is returned. The caller must provide an lfs_file_t object that
 * will be used for all subsequent read operations on this file.
 *
 * File metadata (size, computed CRC, expected CRC, validity flags) is written
 * to the caller-provided filesys_file_info_t, even on CRC mismatch, so the
 * caller can inspect the values.
 *
 * @param slate Pointer to the slate structure.
 * @param file Pointer to a caller-allocated lfs_file_t to be initialized.
 * @param fname The name of the file to open.
 * @param info Pointer to a caller-allocated filesys_file_info_t that will be
 *        populated with file metadata and CRC results.
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error.
 * @return FILESYS_ERR_OPEN_FILE if the file could not be opened,
 *         FILESYS_ERR_GET_CRC_ATTR if the stored CRC attribute could not be
 *         retrieved,
 *         FILESYS_ERR_CRC_CHECK if an error occurred during CRC computation,
 *         FILESYS_ERR_CRC_MISMATCH if the computed CRC does not match the
 *         stored CRC,
 *         FILESYS_OK on success.
 */
filesys_error_t filesys_open_file_read(slate_t *slate, lfs_file_t *file,
                                       const FILESYS_BUFFERED_FNAME_STR_T fname,
                                       filesys_file_info_t *info,
                                       lfs_ssize_t *lfs_error_code);

/**
 * Reads data from an open file at its current file position.
 *
 * This is a thin wrapper around lfs_file_read. The file position is advanced
 * by the number of bytes actually read.
 *
 * @param slate Pointer to the slate structure.
 * @param file Pointer to an open lfs_file_t (from filesys_open_file_read).
 * @param buffer Pointer to the buffer to store the read data.
 * @param size Number of bytes to read.
 * @param bytes_read Pointer to store the actual number of bytes read.
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error.
 * @return FILESYS_ERR_READ_FILE if the read failed,
 *         FILESYS_OK on success.
 */
filesys_error_t filesys_read_data(slate_t *slate, lfs_file_t *file,
                                  void *buffer,
                                  FILESYS_BUFFERED_FILE_LEN_T size,
                                  FILESYS_BUFFERED_FILE_LEN_T *bytes_read,
                                  lfs_ssize_t *lfs_error_code);

/**
 * Seeks to a position in an open read file.
 *
 * This is a thin wrapper around lfs_file_seek.
 *
 * @param slate Pointer to the slate structure.
 * @param file Pointer to an open lfs_file_t (from filesys_open_file_read).
 * @param offset The offset to seek to.
 * @param whence The seek origin: LFS_SEEK_SET, LFS_SEEK_CUR, or LFS_SEEK_END.
 * @param new_position Pointer to store the new absolute file position after the
 * seek.
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error.
 * @return FILESYS_ERR_SEEK_FILE if the seek failed,
 *         FILESYS_OK on success.
 */
filesys_error_t
filesys_read_file_seek(slate_t *slate, lfs_file_t *file, lfs_soff_t offset,
                       int whence, FILESYS_BUFFERED_FILE_LEN_T *new_position,
                       lfs_ssize_t *lfs_error_code);

/**
 * Returns the current position in an open read file.
 *
 * This is a thin wrapper around lfs_file_tell.
 *
 * @param slate Pointer to the slate structure.
 * @param file Pointer to an open lfs_file_t (from filesys_open_file_read).
 * @param position Pointer to store the current file position.
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error.
 * @return FILESYS_ERR_SEEK_FILE if the tell failed,
 *         FILESYS_OK on success.
 */
filesys_error_t filesys_read_file_tell(slate_t *slate, lfs_file_t *file,
                                       FILESYS_BUFFERED_FILE_LEN_T *position,
                                       lfs_ssize_t *lfs_error_code);

/**
 * Returns the size of an open read file.
 *
 * This is a thin wrapper around lfs_file_size.
 *
 * @param slate Pointer to the slate structure.
 * @param file Pointer to an open lfs_file_t (from filesys_open_file_read).
 * @param size Pointer to store the file size in bytes.
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error.
 * @return FILESYS_ERR_FILE_SIZE if the size query failed,
 *         FILESYS_OK on success.
 */
filesys_error_t filesys_read_file_size(slate_t *slate, lfs_file_t *file,
                                       FILESYS_BUFFERED_FILE_LEN_T *size,
                                       lfs_ssize_t *lfs_error_code);

/**
 * Closes an open read file.
 *
 * Releases any resources associated with the open file.
 *
 * @param slate Pointer to the slate structure.
 * @param file Pointer to an open lfs_file_t (from filesys_open_file_read).
 * @param lfs_error_code Pointer to store error code in case of failure.
 * LFS_ERR_OK if there is no relevant LFS error.
 * @return FILESYS_ERR_CLOSE_FILE if the file could not be closed,
 *         FILESYS_OK on success.
 */
filesys_error_t filesys_close_file_read(slate_t *slate, lfs_file_t *file,
                                        lfs_ssize_t *lfs_error_code);
