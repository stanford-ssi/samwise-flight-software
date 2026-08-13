#include "lfs_mram_wrapper.h"
#include "config.h"
#include "logger.h"

/*
 * FILESYS_MRAM_BASE_OFFSET is a compile-time copy of a layout that actually
 * lives in the partition table, so it can be stale -- the partition table is
 * flashed separately from the firmware. Until filesys_validate_mram_layout()
 * has confirmed the two agree, every access is refused.
 *
 * The failure this prevents: a stale offset silently reads and writes inside
 * Program A or Program B, i.e. on top of firmware, with no error anywhere.
 */
static bool s_layout_validated = false;

void lfs_mram_mark_layout_validated(void)
{
    s_layout_validated = true;
}

bool lfs_mram_layout_is_validated(void)
{
    return s_layout_validated;
}

int lfs_mram_wrap_read(const struct lfs_config *c, lfs_block_t block,
                       lfs_off_t off, void *buffer, lfs_size_t size)
{
    if (!s_layout_validated)
    {
        LOG_ERROR("[filesys] MRAM layout not validated; refusing read");
        return LFS_ERR_IO;
    }

    mram_read(block * c->block_size + off + FILESYS_MRAM_BASE_OFFSET, buffer,
              size);

    return LFS_ERR_OK;
}

int lfs_mram_wrap_prog(const struct lfs_config *c, lfs_block_t block,
                       lfs_off_t off, const void *buffer, lfs_size_t size)
{
    if (!s_layout_validated)
    {
        LOG_ERROR("[filesys] MRAM layout not validated; refusing write");
        return LFS_ERR_IO;
    }

    if (!mram_write(block * c->block_size + off + FILESYS_MRAM_BASE_OFFSET,
                    buffer, size))
        return LFS_ERR_IO;

    return LFS_ERR_OK;
}

int lfs_mram_wrap_erase(const struct lfs_config *c, lfs_block_t block)
{
    // MRAM does not require erase before write, so this is a no-op
    return LFS_ERR_OK;
}

int lfs_mram_wrap_sync(const struct lfs_config *c)
{
    // MRAM is non-volatile and does not require sync, so this is a no-op
    return LFS_ERR_OK;
}
