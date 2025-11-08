#include "filesys.h"

// variables used by the filesystem
lfs_t lfs;
lfs_file_t file;

// configuration of the filesystem is provided by this struct
const struct lfs_config cfg = {
    // block device operations
    .read = lfs_mram_wrap_read,
    .prog = lfs_mram_wrap_prog,
    .erase = lfs_mram_wrap_erase,
    .sync = lfs_mram_wrap_sync,

    // block device configuration
    .read_size = 16,
    .prog_size = 16,

    // TODO: What is our average file size?
    // This is currently set to 1KB blocks, which is approx. what we buffer
    // in RAM during FTP.
    .block_size = 1024,
    .block_count = 512, // 512KB MRAM total

    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = 500,
};

lfs_ssize_t filesys_initialize(slate_t *slate)
{
    // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);

    if (err)
    {
        LOG_ERROR("[filesys] Failed to mount filesystem: %d", err);
        return err;
    }

    slate->filesys_is_writing_file = false;
    slate->filesys_buffer_is_dirty = false;

    LOG_INFO("[filesys] Filesystem mounted successfully");
    return 0;
}

int8_t filesys_start_file_write(slate_t *slate, FILESYS_BUFFERED_FNAME_T fname,
                                FILESYS_BUFFERED_FILE_LEN_T file_size,
                                FILESYS_BUFFERED_FILE_CRC_T file_crc,
                                lfs_ssize_t *blocksLeftAfterWrite)
{
    if (slate->filesys_is_writing_file)
    {
        LOG_ERROR("[filesys] Cannot start new file write for %s; a file is "
                  "already being written: %s",
                  fname, slate->filesys_buffered_fname);
        return -1;
    }

    if (slate->filesys_buffer_is_dirty)
    {
        LOG_ERROR("[filesys] Something went wrong! Buffer is dirty while not "
                  "writing a file. Clearing buffer forcefully.");

        filesys_clear_buffer(slate);
    }

    lfs_ssize_t fs_size = lfs_fs_size(&lfs);

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
    char fname_str[sizeof(FILESYS_BUFFERED_FNAME_T) + 1];
    toString(slate->filesys_buffered_fname, fname_str);
    int err = lfs_file_open(&lfs, &file, fname_str,
                            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
    if (err)
    {
        LOG_ERROR("[filesys] Failed to open file %s for writing: %d", fname,
                  err);
        return -4;
    }

    slate->filesys_is_writing_file = true;
    slate->filesys_buffered_fname = fname;
    slate->filesys_buffered_file_len = file_size;
    slate->filesys_buffered_file_crc = file_crc;
    slate->filesys_buffer_is_dirty = false;

    LOG_INFO("[filesys] Started file write for file: %s", fname);
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
                                         FILESYS_BUFFERED_FNAME_T fname,
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
        lfs_file_write(&lfs, &file, slate->filesys_buffer, n_bytes);
    if (bytes_written < 0)
    {
        LOG_ERROR("[filesys] Failed to write buffer to file %s: %d", fname,
                  bytes_written);
        lfs_file_close(&lfs, &file);
        return bytes_written;
    }

    filesys_clear_buffer(slate);

    LOG_INFO("[filesys] Wrote %d bytes from buffer to file %s in MRAM",
             bytes_written, fname);

    return bytes_written;
}

int8_t filesys_complete_file_write(slate_t *slate)
{
    if (slate->filesys_buffer_is_dirty)
    {
        LOG_ERROR(
            "[filesys] Cannot complete file write; buffer is dirty. Please "
            "write or clear the buffer before completing.");
        return -1;
    }

    // Close the file
    int err = lfs_file_close(&lfs, &file);
    if (err)
    {
        LOG_ERROR("[filesys] Failed to close file %s: %d",
                  slate->filesys_buffered_fname, err);
        return -2;
    }

    slate->filesys_is_writing_file = false;
    LOG_INFO("[filesys] Completed file write for file: %s",
             slate->filesys_buffered_fname);

    return 0;
}

void filesys_clear_buffer(slate_t *slate)
{
    slate->filesys_buffer_is_dirty = false;
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
    int err = lfs_file_close(&lfs, &file);
    if (err)
    {
        LOG_ERROR("[filesys] Failed to close file %s during cancel: %d",
                  slate->filesys_buffered_fname, err);
        return -2;
    }

    // Delete the file
    char fname_str[sizeof(FILESYS_BUFFERED_FNAME_T) + 1];
    toString(slate->filesys_buffered_fname, fname_str);
    err = lfs_remove(&lfs, fname_str);
    if (err)
    {
        LOG_ERROR("[filesys] Failed to delete file %s during cancel: %d",
                  slate->filesys_buffered_fname, err);
        return -3;
    }

    filesys_clear_buffer(slate);
    slate->filesys_is_writing_file = false;

    LOG_INFO("[filesys] Cancelled file write and deleted file: %s",
             slate->filesys_buffered_fname);

    return 0;
}
