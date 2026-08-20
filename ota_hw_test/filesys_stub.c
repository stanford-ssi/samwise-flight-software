/**
 * Minimal in-memory stand-in for the three filesystem calls ota_task.c makes.
 *
 * Why this exists: on a bare RP2350 dev board the real filesystem uses
 * lfs_gen_flash_wrapper.c, which reads via
 *
 *     XIP_BASE + LFS_FLASH_BASE + ...        (LFS_FLASH_BASE = 1 MB)
 *
 * The RP2350 remaps the XIP window to the *running* partition, so from inside
 * partition A (152 KiB) that address is far out of bounds and faults. The
 * hardware test confirmed this: it hung at stage 2, filesystem init.
 *
 * That is a defect in the legacy flash backend, not in the OTA task. Serving
 * the image from a plain array removes the filesystem from the picture so the
 * real ota_task_dispatch() can be exercised against real flash, a real
 * partition table, and real TBYB.
 */

#include "filesys.h"
#include "image_b.h"

#include <string.h>

static uint32_t s_read_pos = 0;
static bool s_open = false;

filesys_error_t filesys_open_file_read(slate_t *slate, lfs_file_t *file,
                                       FILESYS_BUFFERED_FNAME_STR_T fname,
                                       filesys_file_info_t *info,
                                       lfs_ssize_t *lfs_error_code)
{
    (void)slate;
    (void)file;
    (void)fname;

    if (lfs_error_code)
        *lfs_error_code = 0;

    s_read_pos = 0;
    s_open = true;

    if (info)
    {
        memset(info, 0, sizeof(*info));
        info->file_size = image_b_bin_len;
    }
    return FILESYS_OK;
}

filesys_error_t filesys_read_data(slate_t *slate, lfs_file_t *file,
                                  void *buffer, FILESYS_BUFFERED_FILE_LEN_T size,
                                  FILESYS_BUFFERED_FILE_LEN_T *bytes_read,
                                  lfs_ssize_t *lfs_error_code)
{
    (void)slate;
    (void)file;

    if (lfs_error_code)
        *lfs_error_code = 0;

    if (!s_open)
        return FILESYS_ERR_READ_FILE;

    uint32_t remaining = image_b_bin_len - s_read_pos;
    uint32_t n = (size < remaining) ? size : remaining;

    memcpy(buffer, &image_b_bin[s_read_pos], n);
    s_read_pos += n;

    if (bytes_read)
        *bytes_read = n;
    return FILESYS_OK;
}

filesys_error_t filesys_close_file_read(slate_t *slate, lfs_file_t *file,
                                        lfs_ssize_t *lfs_error_code)
{
    (void)slate;
    (void)file;
    if (lfs_error_code)
        *lfs_error_code = 0;
    s_open = false;
    return FILESYS_OK;
}
