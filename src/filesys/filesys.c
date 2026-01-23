/**
 * @author Ayush Garg
 * @date 2026-01-15
 * @brief Implementation file for the filesystem module.
 */

#include "filesys.h"
#include <string.h>

const struct lfs_config cfg = {
    // block device operations
    .read = lfs_mram_wrap_read,
    .prog = lfs_mram_wrap_prog,
    .erase = lfs_mram_wrap_erase,
    .sync = lfs_mram_wrap_sync,

    // block device configuration
    .read_size = 16,
    .prog_size = 16,

    .block_size = FILESYS_BLOCK_SIZE,
    .block_count = FILESYS_BLOCK_COUNT,

    .cache_size = FILESYS_CFG_CACHE_SIZE,
    .lookahead_size = FILESYS_CFG_LOOKAHEAD_SIZE,
    .block_cycles = 500,

    .prog_buffer = prog_buffer,
    .read_buffer = read_buffer,
    .lookahead_buffer = lookahead_buffer,

    .name_max = sizeof(FILESYS_BUFFERED_FNAME_STR_T),
};

lfs_ssize_t filesys_initialize(slate_t *slate)
{
    // mount the filesystem
    mram_write_enable();
    int err = lfs_mount(&slate->lfs, &cfg);

    if (err < 0)
    {
        LOG_ERROR("[filesys] Failed to mount filesystem: %d", err);
        return err;
    }

    slate->filesys_is_writing_file = false;
    filesys_clear_buffer(slate);

    LOG_INFO("[filesys] Filesystem mounted successfully");
    return 0;
}

lfs_ssize_t filesys_reformat(slate_t *slate)
{
    mram_write_enable();
    int err = lfs_format(&slate->lfs, &cfg);

    if (err < 0)
    {
        LOG_ERROR("[filesys] Failed to format filesystem: %d", err);
        return err;
    }

    LOG_INFO("[filesys] Filesystem formatted successfully");

    lfs_ssize_t errInit = filesys_initialize(slate); // Re-mount after format
    if (errInit < 0)
        return errInit;

    return 0;
}

int8_t filesys_start_file_write(slate_t *slate,
                                FILESYS_BUFFERED_FNAME_STR_T fname_str,
                                FILESYS_BUFFERED_FILE_LEN_T file_size,
                                FILESYS_BUFFERED_FILE_CRC_T file_crc,
                                lfs_ssize_t *blocksLeftAfterWrite)
{
    if (slate->filesys_is_writing_file)
    {
        LOG_ERROR("[filesys] Cannot start new file write for %s; a file is "
                  "already being written: %s",
                  fname_str, slate->filesys_buffered_fname_str);
        return -1;
    }

    if (slate->filesys_buffer_is_dirty)
    {
        LOG_ERROR("[filesys] Something went wrong! Buffer is dirty while not "
                  "writing a file. Clearing buffer forcefully.");

        filesys_clear_buffer(slate);
    }

    lfs_ssize_t fs_size = lfs_fs_size(&slate->lfs);

    if (fs_size < 0)
    {
        LOG_ERROR("[filesys] Failed to get filesystem size: Error %d", fs_size);
        return -2;
    }

    // Get number of blocks needed, which is ceil(file_size / block_size)
    lfs_ssize_t numBlocksNeeded =
        (file_size + cfg.block_size - 1) / cfg.block_size;

    *blocksLeftAfterWrite = cfg.block_count - fs_size - numBlocksNeeded;

    if (fs_size + numBlocksNeeded > cfg.block_count)
    {
        LOG_ERROR(
            "[filesys] Not enough space in filesystem to start file write. "
            "File size: %u bytes, Blocks needed: %d, FS size: %u blocks, "
            "Block count: %u blocks",
            file_size, numBlocksNeeded, fs_size, cfg.block_count);
        return -3;
    }

    // Open file for appending
    memcpy(slate->filesys_buffered_fname_str, fname_str,
           sizeof(FILESYS_BUFFERED_FNAME_STR_T));

    int err = lfs_file_opencfg(
        &slate->lfs, &slate->filesys_lfs_open_file,
        slate->filesys_buffered_fname_str,
        LFS_O_RDWR | LFS_O_CREAT |
            LFS_O_APPEND, // Allow reading for CRC check later on
        &(struct lfs_file_config){.buffer = cache_buffer});

    if (err < 0)
    {
        LOG_ERROR("[filesys] Failed to open file %s for writing: %d",
                  slate->filesys_buffered_fname_str, err);
        return -4;
    }

    // Add CRC as attribute to open file - type 0
    err = lfs_setattr(&slate->lfs, slate->filesys_buffered_fname_str, 0,
                      &file_crc, sizeof(file_crc));
    if (err < 0)
    {
        LOG_ERROR("[filesys] Failed to set CRC attribute for file %s: %d",
                  slate->filesys_buffered_fname_str, err);
        lfs_file_close(&slate->lfs, &slate->filesys_lfs_open_file);
        return -5;
    }

    slate->filesys_is_writing_file = true;
    slate->filesys_buffered_file_len = file_size;
    slate->filesys_buffered_file_crc = file_crc;
    slate->filesys_buffer_is_dirty = false;

    LOG_INFO("[filesys] Started file write for file: %s",
             slate->filesys_buffered_fname_str);
    return 0;
}

int8_t filesys_write_data_to_buffer(slate_t *slate, const uint8_t *data,
                                    FILESYS_BUFFER_SIZE_T n_bytes,
                                    FILESYS_BUFFER_SIZE_T offset)
{
    if (offset + n_bytes > FILESYS_BUFFER_SIZE)
    {
        LOG_ERROR("[filesys] Write exceeds buffer size. Offset: %u, Bytes: %u",
                  offset, n_bytes);
        return -1;
    }

    if (!slate->filesys_is_writing_file)
    {
        LOG_ERROR(
            "[filesys] Cannot write data to buffer; no file is currently being "
            "written.");
        return -2;
    }

    slate->filesys_buffer_is_dirty = true;
    memcpy(&slate->filesys_buffer[offset], data, n_bytes);

    LOG_INFO("[filesys] Wrote %u bytes to buffer at offset %u", n_bytes,
             offset);

    return 0;
}

lfs_ssize_t filesys_write_buffer_to_mram(slate_t *slate,
                                         FILESYS_BUFFER_SIZE_T n_bytes)
{
    if (!slate->filesys_is_writing_file)
    {
        LOG_ERROR(
            "[filesys] Cannot write buffer to MRAM; no file is currently being "
            "written.");
        return -1;
    }

    if (!slate->filesys_buffer_is_dirty)
    {
        LOG_INFO("[filesys] Buffer is clean; no need to write to MRAM.");
        return 0;
    }

    // Write buffer to file
    lfs_ssize_t bytes_written =
        lfs_file_write(&slate->lfs, &slate->filesys_lfs_open_file,
                       slate->filesys_buffer, n_bytes);
    if (bytes_written < 0)
    {
        LOG_ERROR("[filesys] Failed to write buffer to file %s: %d",
                  slate->filesys_buffered_fname_str, bytes_written);
        lfs_file_close(&slate->lfs, &slate->filesys_lfs_open_file);
        return bytes_written;
    }

    filesys_clear_buffer(slate);

    LOG_INFO("[filesys] Wrote %d bytes from buffer to file %s in MRAM",
             bytes_written, slate->filesys_buffered_fname_str);

    return bytes_written;
}

unsigned int filesys_compute_crc(slate_t *slate, int8_t *error_code)
{
    unsigned int crc = 0xFFFFFFFF;

    // Read file in chunks and compute CRC
    uint8_t buffer[FILESYS_READ_BUFFER_SIZE];
    lfs_file_rewind(&slate->lfs, &slate->filesys_lfs_open_file);

    FILESYS_BUFFERED_FILE_LEN_T bytes_remaining =
        slate->filesys_buffered_file_len;
    while (bytes_remaining > 0)
    {
        lfs_size_t to_read = (bytes_remaining < FILESYS_READ_BUFFER_SIZE)
                                 ? bytes_remaining
                                 : FILESYS_READ_BUFFER_SIZE;

        lfs_ssize_t bytes_read = lfs_file_read(
            &slate->lfs, &slate->filesys_lfs_open_file, buffer, to_read);

        if (bytes_read < 0)
        {
            LOG_ERROR("[filesys] Failed to read file at %d bytes left for CRC "
                      "computation: Error code %d",
                      bytes_remaining, bytes_read);
            *error_code = -1;
            return crc; // We will return the crc so far, but error_code
                        // indicates failure
        }

        crc = crc32_continue(buffer, bytes_read, crc);
        bytes_remaining -= bytes_read;
    }

    *error_code = 0;
    return ~crc;
}

int8_t filesys_is_crc_correct(slate_t *slate, unsigned int *crc_out)
{
    if (!slate->filesys_is_writing_file)
    {
        LOG_ERROR(
            "[filesys] Cannot check CRC; no file is currently being written.");
        return -2;
    }

    uint8_t error_code = 0;
    *crc_out = filesys_compute_crc(slate, &error_code);

    if (*crc_out == slate->filesys_buffered_file_crc)
    {
        LOG_INFO("[filesys] CRC check passed for file %s",
                 slate->filesys_buffered_fname_str);
        return 0;
    }
    else
    {
        LOG_ERROR("[filesys] CRC check failed for file %s. Computed: %u, "
                  "Expected: %u",
                  slate->filesys_buffered_fname_str, *crc_out,
                  slate->filesys_buffered_file_crc);
        return -1;
    }
}

int8_t filesys_complete_file_write(slate_t *slate, unsigned int *crc_out)
{
    if (slate->filesys_buffer_is_dirty)
    {
        LOG_ERROR(
            "[filesys] Cannot complete file write; buffer is dirty. Please "
            "write or clear the buffer before completing.");
        return -1;
    }

    // Check CRC here
    int8_t crc_check = filesys_is_crc_correct(slate, crc_out);
    if (crc_check != 0 && crc_check != -1)
    {
        LOG_ERROR("[filesys] Error during CRC check for file %s: %d",
                  slate->filesys_buffered_fname_str, crc_check);
        return -3;
    }
    else if (crc_check == -1)
    {
        LOG_ERROR("[filesys] CRC mismatch for file %s during completion.",
                  slate->filesys_buffered_fname_str);
        return -4;
    }

    LOG_INFO("[filesys] CRC matches for file %s!",
             slate->filesys_buffered_fname_str);

    // Close the file
    int err = lfs_file_close(&slate->lfs, &slate->filesys_lfs_open_file);
    if (err < 0)
    {
        LOG_ERROR("[filesys] Failed to close file %s: %d",
                  slate->filesys_buffered_fname_str, err);
        return -2;
    }

    slate->filesys_is_writing_file = false;
    LOG_INFO("[filesys] Completed file write for file: %s",
             slate->filesys_buffered_fname_str);

    return 0;
}

void filesys_clear_buffer(slate_t *slate)
{
    slate->filesys_buffer_is_dirty = false;
    for (FILESYS_BUFFER_SIZE_T i = 0; i < FILESYS_BUFFER_SIZE; i++)
        slate->filesys_buffer[i] = 0; // Clear buffer contents

    LOG_INFO("[filesys] Marked filesystem buffer as clean.");
}

int8_t filesys_cancel_file_write(slate_t *slate)
{
    if (!slate->filesys_is_writing_file)
    {
        LOG_ERROR(
            "[filesys] Cannot cancel file write; no file is currently being "
            "written.");
        return -1;
    }

    // Close the file
    int err = lfs_file_close(&slate->lfs, &slate->filesys_lfs_open_file);
    if (err < 0)
    {
        LOG_ERROR("[filesys] Failed to close file %s during cancel: %d",
                  slate->filesys_buffered_fname_str, err);
        return -2;
    }

    // Delete the file
    err = lfs_remove(&slate->lfs, slate->filesys_buffered_fname_str);
    if (err < 0)
    {
        LOG_ERROR("[filesys] Failed to delete file %s during cancel: %d",
                  slate->filesys_buffered_fname_str, err);
        return -3;
    }

    filesys_clear_buffer(slate);
    slate->filesys_is_writing_file = false;

    LOG_INFO("[filesys] Cancelled file write and deleted file: %s",
             slate->filesys_buffered_fname_str);

    return 0;
}
