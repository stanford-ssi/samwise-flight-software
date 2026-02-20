/**
 * @author Ayush Garg
 * @date 2026-01-15
 * @brief Implementation file for the filesystem module.
 */

#include "filesys.h"
#include <string.h>

const struct lfs_config filesys_lfs_cfg = {
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

const struct lfs_file_config filesys_lfs_file_cfg = {
    .buffer = cache_buffer,
};

void filesys_file_open(lfs_t *lfs, lfs_file_t *file, const char *fname,
                       int flags, lfs_ssize_t *lfs_error_code)
{
    *lfs_error_code = LFS_ERR_OK;
    int err = lfs_file_opencfg(lfs, file, fname, flags, &filesys_lfs_file_cfg);
    if (err < 0)
    {
        *lfs_error_code = err;
        LOG_ERROR("[filesys] Failed to open file %s: %d", fname, err);
    }
}

void filesys_file_close(lfs_t *lfs, lfs_file_t *file,
                        lfs_ssize_t *lfs_error_code)
{
    *lfs_error_code = LFS_ERR_OK;
    int err = lfs_file_close(lfs, file);

    if (err < 0)
    {
        *lfs_error_code = err;
        LOG_ERROR("[filesys] Failed to close file: %d", err);
    }
}

filesys_error_t filesys_initialize(slate_t *slate, lfs_ssize_t *lfs_error_code)
{
    *lfs_error_code = LFS_ERR_OK;

    // mount the filesystem
    mram_write_enable();
    mram_init();
    int err = lfs_mount(&slate->lfs, &filesys_lfs_cfg);

    if (err < 0)
    {
        *lfs_error_code = err;
        LOG_ERROR("[filesys] Failed to mount filesystem: %d", err);
        return FILESYS_ERR_MOUNT;
    }

    slate->filesys_is_writing_file = false;
    filesys_clear_buffer(slate);

    LOG_INFO("[filesys] Filesystem mounted successfully");
    return FILESYS_OK;
}

filesys_error_t filesys_reformat_initialize(slate_t *slate,
                                            lfs_ssize_t *lfs_error_code)
{
    *lfs_error_code = LFS_ERR_OK;

    mram_write_enable();
    mram_init();
    int err = lfs_format(&slate->lfs, &filesys_lfs_cfg);

    if (err < 0)
    {
        *lfs_error_code = err;
        LOG_ERROR("[filesys] Failed to format filesystem: %d", err);
        return FILESYS_ERR_REFORMAT;
    }

    LOG_INFO("[filesys] Filesystem formatted successfully");

    filesys_error_t errInit =
        filesys_initialize(slate, lfs_error_code); // Re-mount after format
    if (errInit < 0)
        return errInit;

    return FILESYS_OK;
}

filesys_error_t filesys_start_file_write(slate_t *slate,
                                         FILESYS_BUFFERED_FNAME_STR_T fname_str,
                                         FILESYS_BUFFERED_FILE_LEN_T file_size,
                                         FILESYS_BUFFERED_FILE_CRC_T file_crc,
                                         lfs_ssize_t *lfs_error_code,
                                         lfs_ssize_t *blocks_left_after_write)
{
    *lfs_error_code = LFS_ERR_OK;

    if (slate->filesys_is_writing_file)
    {
        LOG_ERROR("[filesys] Cannot start new file write for %s; a file is "
                  "already being written: %s",
                  fname_str, slate->filesys_buffered_fname_str);
        return FILESYS_ERR_FILE_ALREADY_WRITING;
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
        *lfs_error_code = fs_size;
        LOG_ERROR("[filesys] Failed to get filesystem size: Error %d", fs_size);
        return FILESYS_ERR_GET_FS_SIZE;
    }

    // Get number of blocks needed, which is ceil(file_size / block_size)
    lfs_ssize_t num_blocks_needed =
        (file_size + filesys_lfs_cfg.block_size - 1) /
        filesys_lfs_cfg.block_size;

    *blocks_left_after_write =
        filesys_lfs_cfg.block_count - fs_size - num_blocks_needed;

    if (fs_size + num_blocks_needed > filesys_lfs_cfg.block_count)
    {
        LOG_ERROR(
            "[filesys] Not enough space in filesystem to start file write. "
            "File size: %u bytes, Blocks needed: %d, FS size: %u blocks, "
            "Block count: %u blocks",
            file_size, num_blocks_needed, fs_size, filesys_lfs_cfg.block_count);
        return FILESYS_ERR_NOT_ENOUGH_SPACE;
    }

    // Open file for appending
    memcpy(slate->filesys_buffered_fname_str, fname_str,
           sizeof(FILESYS_BUFFERED_FNAME_STR_T));

    lfs_file_t lfs_open_file;
    lfs_ssize_t open_lfs_err;
    filesys_file_open(&slate->lfs, &lfs_open_file,
                      slate->filesys_buffered_fname_str,
                      LFS_O_CREAT | LFS_O_WRONLY | LFS_O_TRUNC, &open_lfs_err);

    if (open_lfs_err < 0)
    {
        *lfs_error_code = open_lfs_err;
        LOG_ERROR("[filesys] Failed to open file %s for writing: %d",
                  slate->filesys_buffered_fname_str, open_lfs_err);
        return FILESYS_ERR_OPEN_FILE;
    }

    // Add CRC as attribute to open file - type 0
    int err = lfs_setattr(&slate->lfs, slate->filesys_buffered_fname_str, 0,
                          &file_crc, sizeof(file_crc));
    if (err < 0)
    {
        *lfs_error_code = err;
        LOG_ERROR("[filesys] Failed to set CRC attribute for file %s: %d",
                  slate->filesys_buffered_fname_str, err);

        // Discard error from close since we are already reporting the setattr
        // error
        lfs_ssize_t close_lfs_err;
        filesys_file_close(&slate->lfs, &lfs_open_file, &close_lfs_err);

        return FILESYS_ERR_SET_CRC_ATTR;
    }

    // Close file for now - reopen it every time we write
    lfs_ssize_t close_lfs_err;
    filesys_file_close(&slate->lfs, &lfs_open_file, &close_lfs_err);
    if (close_lfs_err < 0)
    {
        *lfs_error_code = close_lfs_err;
        LOG_ERROR(
            "[filesys] Failed to close file %s after setting attributes: %d",
            slate->filesys_buffered_fname_str, close_lfs_err);
        return FILESYS_ERR_CLOSE_FILE;
    }

    slate->filesys_is_writing_file = true;
    slate->filesys_buffered_file_len = file_size;
    slate->filesys_buffered_file_crc = file_crc;
    slate->filesys_buffer_is_dirty = false;

    LOG_INFO("[filesys] Started file write for file: %s",
             slate->filesys_buffered_fname_str);
    return FILESYS_OK;
}

filesys_error_t filesys_write_data_to_buffer(slate_t *slate,
                                             const uint8_t *data,
                                             FILESYS_BUFFER_SIZE_T n_bytes,
                                             FILESYS_BUFFER_SIZE_T offset,
                                             lfs_ssize_t *lfs_error_code)
{
    *lfs_error_code = LFS_ERR_OK;

    if (offset + n_bytes > FILESYS_BUFFER_SIZE)
    {
        LOG_ERROR("[filesys] Write exceeds buffer size. Offset: %u, Bytes: %u",
                  offset, n_bytes);
        return FILESYS_ERR_EXCEED_BUFFER;
    }

    if (!slate->filesys_is_writing_file)
    {
        LOG_ERROR(
            "[filesys] Cannot write data to buffer; no file is currently being "
            "written.");
        return FILESYS_ERR_NO_FILE_WRITING;
    }

    slate->filesys_buffer_is_dirty = true;
    memcpy(&slate->filesys_buffer[offset], data, n_bytes);

    LOG_INFO("[filesys] Wrote %u bytes to buffer at offset %u", n_bytes,
             offset);

    return FILESYS_OK;
}

filesys_error_t filesys_write_buffer_to_mram(slate_t *slate,
                                             FILESYS_BUFFER_SIZE_T n_bytes,
                                             lfs_ssize_t *lfs_error_code)
{
    *lfs_error_code = LFS_ERR_OK;

    if (!slate->filesys_is_writing_file)
    {
        LOG_ERROR(
            "[filesys] Cannot write buffer to MRAM; no file is currently being "
            "written.");
        return FILESYS_ERR_NO_FILE_WRITING;
    }

    if (!slate->filesys_buffer_is_dirty)
    {
        LOG_INFO("[filesys] Buffer is clean; no need to write to MRAM.");
        return FILESYS_OK;
    }

    // Reopen the file for appending
    lfs_file_t lfs_open_file;
    lfs_ssize_t open_lfs_err;
    filesys_file_open(&slate->lfs, &lfs_open_file,
                      slate->filesys_buffered_fname_str,
                      LFS_O_WRONLY | LFS_O_APPEND, &open_lfs_err);

    if (open_lfs_err < 0)
    {
        *lfs_error_code = open_lfs_err;
        LOG_ERROR("[filesys] Failed to open file %s for appending: %d",
                  slate->filesys_buffered_fname_str, open_lfs_err);
        return FILESYS_ERR_OPEN_FILE;
    }

    // Write buffer to file
    lfs_ssize_t bytes_written = lfs_file_write(&slate->lfs, &lfs_open_file,
                                               slate->filesys_buffer, n_bytes);
    if (bytes_written < 0)
    {
        *lfs_error_code = bytes_written;
        LOG_ERROR("[filesys] Failed to write buffer to file %s: %d",
                  slate->filesys_buffered_fname_str, bytes_written);

        // Get amount of space used
        lfs_ssize_t used_size = lfs_file_size(&slate->lfs, &lfs_open_file);
        lfs_ssize_t total_fs_used_size = lfs_fs_size(&slate->lfs);
        LOG_ERROR("[filesys] Current file size: %d bytes, Total FS used size: "
                  "%d blocks",
                  used_size, total_fs_used_size);

        // Discard error from cancel since we are already reporting the write
        // error
        lfs_ssize_t cancel_lfs_err;
        filesys_cancel_file_write(slate, &cancel_lfs_err);

        return FILESYS_ERR_WRITE_MRAM;
    }

    lfs_ssize_t close_lfs_err;
    filesys_file_close(&slate->lfs, &lfs_open_file, &close_lfs_err);
    if (close_lfs_err < 0)
    {
        *lfs_error_code = close_lfs_err;
        LOG_ERROR("[filesys] Failed to close file %s after writing: %d",
                  slate->filesys_buffered_fname_str, close_lfs_err);
        return FILESYS_ERR_CLOSE_FILE;
    }

    filesys_clear_buffer(slate);

    LOG_INFO("[filesys] Wrote %d bytes from buffer to file %s in MRAM",
             bytes_written, slate->filesys_buffered_fname_str);

    return FILESYS_OK;
}

unsigned int filesys_compute_file_crc(lfs_t *lfs, const char *fname,
                                      FILESYS_BUFFERED_FILE_LEN_T file_size,
                                      filesys_error_t *error_code,
                                      lfs_ssize_t *lfs_error_code)
{
    *lfs_error_code = LFS_ERR_OK;
    unsigned int crc = 0xFFFFFFFF;

    lfs_file_t lfs_open_file;
    lfs_ssize_t open_lfs_err;
    filesys_file_open(lfs, &lfs_open_file, fname, LFS_O_RDONLY, &open_lfs_err);

    if (open_lfs_err < 0)
    {
        *lfs_error_code = open_lfs_err;
        LOG_ERROR("[filesys] Failed to open file %s for CRC computation: %d",
                  fname, open_lfs_err);
        *error_code = FILESYS_ERR_OPEN_FILE;
        return crc;
    }

    // Read file in chunks and compute CRC
    uint8_t buffer[FILESYS_READ_BUFFER_SIZE];

    FILESYS_BUFFERED_FILE_LEN_T bytes_remaining = file_size;
    while (bytes_remaining > 0)
    {
        lfs_size_t to_read = (bytes_remaining < FILESYS_READ_BUFFER_SIZE)
                                 ? bytes_remaining
                                 : FILESYS_READ_BUFFER_SIZE;

        lfs_ssize_t bytes_read =
            lfs_file_read(lfs, &lfs_open_file, buffer, to_read);

        if (bytes_read < 0)
        {
            *lfs_error_code = bytes_read;
            LOG_ERROR("[filesys] Failed to read file %s at %d bytes left for "
                      "CRC computation: Error code %d",
                      fname, bytes_remaining, bytes_read);
            *error_code = FILESYS_ERR_CRC_CHECK;

            // Discard error from close since we are already reporting the read
            // error
            lfs_ssize_t close_lfs_err;
            filesys_file_close(lfs, &lfs_open_file, &close_lfs_err);

            return crc; // We will return the crc so far, but error_code
                        // indicates failure
        }

        crc = crc32_continue(buffer, bytes_read, crc);
        bytes_remaining -= bytes_read;
    }

    lfs_ssize_t close_lfs_err;
    filesys_file_close(lfs, &lfs_open_file, &close_lfs_err);

    if (close_lfs_err < 0)
    {
        *lfs_error_code = close_lfs_err;
        LOG_ERROR("[filesys] Failed to close file %s after CRC computation: %d",
                  fname, close_lfs_err);
        *error_code = FILESYS_ERR_CLOSE_FILE;
        return crc;
    }

    *error_code = FILESYS_OK;
    return ~crc;
}

unsigned int filesys_compute_crc(slate_t *slate, filesys_error_t *error_code,
                                 lfs_ssize_t *lfs_error_code)
{
    if (!slate->filesys_is_writing_file)
    {
        LOG_ERROR("[filesys] Cannot compute CRC; no file is currently being "
                  "written.");
        *error_code = FILESYS_ERR_NO_FILE_WRITING;
        *lfs_error_code = LFS_ERR_OK; // No LFS error, just filesys error
        return 0;
    }

    return filesys_compute_file_crc(
        &slate->lfs, slate->filesys_buffered_fname_str,
        slate->filesys_buffered_file_len, error_code, lfs_error_code);
}

filesys_error_t filesys_is_crc_correct(slate_t *slate,
                                       lfs_ssize_t *lfs_error_code)
{
    *lfs_error_code = LFS_ERR_OK;

    if (!slate->filesys_is_writing_file)
    {
        LOG_ERROR(
            "[filesys] Cannot check CRC; no file is currently being written.");
        return FILESYS_ERR_NO_FILE_WRITING;
    }

    filesys_error_t error_code = 0;
    unsigned int computed_crc =
        filesys_compute_crc(slate, &error_code, lfs_error_code);

    if (error_code != FILESYS_OK)
    {
        LOG_ERROR("[filesys] Failed to compute CRC for file %s",
                  slate->filesys_buffered_fname_str);
        return error_code;
    }

    if (computed_crc != slate->filesys_buffered_file_crc)
    {
        LOG_ERROR("[filesys] CRC check failed for file %s. Computed: %u, "
                  "Expected: %u",
                  slate->filesys_buffered_fname_str, computed_crc,
                  slate->filesys_buffered_file_crc);
        return FILESYS_ERR_CRC_MISMATCH;
    }

    LOG_INFO("[filesys] CRC check passed for file %s",
             slate->filesys_buffered_fname_str);
    return FILESYS_OK;
}

filesys_error_t filesys_complete_file_write(slate_t *slate,
                                            lfs_ssize_t *lfs_error_code)
{
    *lfs_error_code = LFS_ERR_OK;

    if (slate->filesys_buffer_is_dirty)
    {
        LOG_ERROR(
            "[filesys] Cannot complete file write; buffer is dirty. Please "
            "write or clear the buffer before completing.");
        return FILESYS_ERR_BUFFER_DIRTY;
    }

    // Check CRC here
    filesys_error_t crc_check = filesys_is_crc_correct(slate, lfs_error_code);
    if (crc_check != FILESYS_OK)
    {
        LOG_INFO("[filesys] CRC check failed during file write completion for "
                 "file %s",
                 slate->filesys_buffered_fname_str);
        return crc_check;
    }

    LOG_INFO("[filesys] CRC matches for file %s!",
             slate->filesys_buffered_fname_str);

    slate->filesys_is_writing_file = false;
    LOG_INFO("[filesys] Completed file write for file: %s",
             slate->filesys_buffered_fname_str);

    return FILESYS_OK;
}

void filesys_clear_buffer(slate_t *slate)
{
    slate->filesys_buffer_is_dirty = false;
    for (FILESYS_BUFFER_SIZE_T i = 0; i < FILESYS_BUFFER_SIZE; i++)
        slate->filesys_buffer[i] = 0; // Clear buffer contents

    LOG_INFO("[filesys] Marked filesystem buffer as clean.");
}

filesys_error_t filesys_cancel_file_write(slate_t *slate,
                                          lfs_ssize_t *lfs_error_code)
{
    *lfs_error_code = LFS_ERR_OK;

    if (!slate->filesys_is_writing_file)
    {
        LOG_ERROR(
            "[filesys] Cannot cancel file write; no file is currently being "
            "written.");
        return FILESYS_ERR_NO_FILE_WRITING;
    }

    // Delete the file
    int err = lfs_remove(&slate->lfs, slate->filesys_buffered_fname_str);
    if (err < 0)
    {
        *lfs_error_code = err;
        LOG_ERROR("[filesys] Failed to delete file %s during cancel: %d",
                  slate->filesys_buffered_fname_str, err);
        return FILESYS_ERR_DELETE_FILE;
    }

    filesys_clear_buffer(slate);
    slate->filesys_is_writing_file = false;

    LOG_INFO("[filesys] Cancelled file write and deleted file: %s",
             slate->filesys_buffered_fname_str);

    return FILESYS_OK;
}

filesys_error_t filesys_list_files(slate_t *slate,
                                   filesys_file_info_t *file_list,
                                   uint16_t max_files,
                                   uint16_t *num_files_found,
                                   lfs_ssize_t *lfs_error_code)
{
    // Note that this current implementation only works with files in the root
    // directory and does not support subdirectories.
    *lfs_error_code = LFS_ERR_OK;
    *num_files_found = 0;

    lfs_dir_t dir;
    int err = lfs_dir_open(&slate->lfs, &dir, FILESYS_ROOT_DIR);
    if (err < 0)
    {
        *lfs_error_code = err;
        LOG_ERROR("[filesys] Failed to open root directory: %d", err);
        return FILESYS_ERR_OPEN_DIR;
    }

    struct lfs_info entry_info;
    for (size_t i = 0; i < FILESYS_MAX_LOOP_LIST_FILES; i++)
    {
        int res = lfs_dir_read(&slate->lfs, &dir, &entry_info);
        if (res < 0)
        {
            *lfs_error_code = res;
            LOG_ERROR("[filesys] Failed to read directory entry: %d", res);
            lfs_dir_close(&slate->lfs, &dir);
            return FILESYS_ERR_READ_DIR;
        }
        if (res == 0)
            break; // No more entries

        // Skip directories (including "." and "..")
        if (entry_info.type != LFS_TYPE_REG)
            continue;

        // Stop if the caller's list is full
        if (*num_files_found >= max_files)
        {
            LOG_INFO("[filesys] file_list full (%u); stopping early",
                     max_files);
            break;
        }

        filesys_file_info_t *info = &file_list[*num_files_found];
        memset(info, 0, sizeof(*info));

        // Copy filename (null-terminated)
        strncpy(info->fname, entry_info.name, sizeof(info->fname) - 1);
        info->fname[sizeof(info->fname) - 1] = '\0';

        // File size from directory entry
        info->file_size = (FILESYS_BUFFERED_FILE_LEN_T)entry_info.size;

        // --- Retrieve expected CRC from file attribute (type 0) ---
        lfs_ssize_t attr_res =
            lfs_getattr(&slate->lfs, entry_info.name, 0, &info->expected_crc,
                        sizeof(info->expected_crc));
        if (attr_res < 0)
        {
            LOG_ERROR("[filesys] Failed to get CRC attribute for file %s: %d",
                      entry_info.name, attr_res);
            info->expected_crc = 0;
        }
        else
        {
            info->flags |= FILESYS_FILE_INFO_EXPECTED_CRC_VALID;
        }

        // --- Compute CRC from on-disk file data ---
        filesys_error_t crc_err;
        lfs_ssize_t crc_lfs_err;
        unsigned int computed = filesys_compute_file_crc(
            &slate->lfs, entry_info.name,
            (FILESYS_BUFFERED_FILE_LEN_T)entry_info.size, &crc_err,
            &crc_lfs_err);

        if (crc_err == FILESYS_OK)
        {
            info->computed_crc = computed;
            info->flags |= FILESYS_FILE_INFO_COMPUTED_CRC_VALID;
        }
        else
        {
            info->computed_crc = 0;
        }

        // Determine CRC match only when both values are valid
        if ((info->flags & FILESYS_FILE_INFO_COMPUTED_CRC_VALID) &&
            (info->flags & FILESYS_FILE_INFO_EXPECTED_CRC_VALID) &&
            (info->computed_crc == info->expected_crc))
        {
            info->flags |= FILESYS_FILE_INFO_CRC_MATCH;
        }

        (*num_files_found)++;
    }

    err = lfs_dir_close(&slate->lfs, &dir);
    if (err < 0)
    {
        *lfs_error_code = err;
        LOG_ERROR("[filesys] Failed to close root directory: %d", err);
        return FILESYS_ERR_CLOSE_DIR;
    }

    LOG_INFO("[filesys] Listed %u files successfully", *num_files_found);
    return FILESYS_OK;
}
