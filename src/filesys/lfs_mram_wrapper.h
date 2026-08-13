/**
 * @file lfs_mram_wrapper.h
 *
 * This file contains wrapper functions for integrating LittleFS with MRAM
 * as the underlying storage medium.
 *
 * The functions defined here adapt the LittleFS block device interface to use
 * MRAM read, write, erase, and sync operations.
 *
 * @author Ayush Garg
 * @date 2025-11-08
 */

#pragma once

#include "lfs.h"
#include "mram.h"

#include <stdbool.h>

/*
 * Records that the compile-time FILESYS_MRAM_BASE_OFFSET / FILESYS_BLOCK_COUNT
 * have been checked against the bootrom partition table. Reads and writes are
 * refused until this is called, so a stale offset cannot silently land on the
 * firmware partitions. Called by filesys_initialize().
 */
void lfs_mram_mark_layout_validated(void);

bool lfs_mram_layout_is_validated(void);

// Read a region in a block. Negative error codes are propagated
// to the user.
int lfs_mram_wrap_read(const struct lfs_config *c, lfs_block_t block,
                       lfs_off_t off, void *buffer, lfs_size_t size);

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int lfs_mram_wrap_prog(const struct lfs_config *c, lfs_block_t block,
                       lfs_off_t off, const void *buffer, lfs_size_t size);

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int lfs_mram_wrap_erase(const struct lfs_config *c, lfs_block_t block);

// Sync the state of the underlying block device. Negative error codes
// are propagated to the user.
int lfs_mram_wrap_sync(const struct lfs_config *c);
