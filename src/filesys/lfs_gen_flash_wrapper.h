/**
 * @file lfs_gen_flash_wrapper.h
 *
 * Identically similar to lfs_mram_wrapper.h, it just wraps
 * the I/O function for the flash for integration with LittleFS.
 *
 * @author Marc Aaron Reyes
 * @data 2026-03-07 (year/month/day)
 */

#pragma once

#include "lfs.h"

// Read a region in a block. Negative error codes are propagated
// to the user.
int lfs_gen_flash_wrap_read(const struct lfs_config *c, lfs_block_t block,
                            lfs_off_t off, void *buffer, lfs_size_t size);

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int lfs_gen_flash_wrap_prog(const struct lfs_config *c, lfs_block_t block,
                            lfs_off_t off, const void *buffer, lfs_size_t size);

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int lfs_gen_flash_wrap_erase(const struct lfs_config *c, lfs_block_t block);

// Sync the state of the underlying block device. Negative error codes
// are propagated to the user.
int lfs_gen_flash_wrap_sync(const struct lfs_config *c);
