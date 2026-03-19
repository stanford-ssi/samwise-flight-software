#ifndef TEST
#include "lfs_gen_flash_wrapper.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

// PLEASE FOR THE LOVE OF GOD, BUDDHA, OR WHATEVER YOU BELIEVE IN
// DEFINE THIS PROPERLY SO THAT PARTITIONING WORKS PROPERLY. DO NOT
// LET AI DEFINE THIS.
// PLEASEEEEEE CHANGE LATER.
// PLEASE CHECK THE DOCUMENTATION THAT THE OFFSET IS NOT FUCKING WITH
// ANYTHING ELSE.

// Start at 1 MB,
#define LFS_FLASH_BASE (1024 * 1024)

int lfs_gen_flash_wrap_read(const struct lfs_config *c, lfs_block_t block,
                            lfs_off_t off, void *buffer, lfs_size_t size)
{
    unsigned int flash_addr =
        XIP_BASE + LFS_FLASH_BASE + (block * c->block_size) + off;
    memcpy(buffer, (const void *)flash_addr, size);

    return 0;
}

int lfs_gen_flash_wrap_prog(const struct lfs_config *c, lfs_block_t block,
                            lfs_off_t off, const void *buffer, lfs_size_t size)
{
    unsigned int flash_offset = LFS_FLASH_BASE + (block * c->block_size) + off;

    unsigned int ints = save_and_disable_interrupts();
    flash_range_program(flash_offset, (const uint8_t *)buffer, size);
    restore_interrupts(ints);

    return 0;
}

int lfs_gen_flash_wrap_erase(const struct lfs_config *c, lfs_block_t block)
{
    unsigned int flash_offset = LFS_FLASH_BASE + (block * c->block_size);

    unsigned int ints = save_and_disable_interrupts();
    flash_range_erase(flash_offset, c->block_size);
    restore_interrupts(ints);

    return 0;
}

int lfs_gen_flash_wrap_sync(const struct lfs_config *c)
{
    return 0;
}
#endif
