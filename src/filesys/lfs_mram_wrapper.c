#include "lfs_mram_wrapper.h"
#include "mram.h"

int lfs_mram_wrap_read(const struct lfs_config *c, lfs_block_t block,
                       lfs_off_t off, void *buffer, lfs_size_t size)
{
    mram_read(block * c->block_size + off, buffer, size);
    return 0;
}

int lfs_mram_wrap_prog(const struct lfs_config *c, lfs_block_t block,
                       lfs_off_t off, const void *buffer, lfs_size_t size)
{
    if (!mram_write(block * c->block_size + off, buffer, size))
        return LFS_ERR_IO;

    return 0;
}

int lfs_mram_wrap_erase(const struct lfs_config *c, lfs_block_t block)
{
    // MRAM does not require erase before write, so this is a no-op
    return 0;
}

int lfs_mram_wrap_sync(const struct lfs_config *c)
{
    // MRAM is non-volatile and does not require sync, so this is a no-op
    return 0;
}
