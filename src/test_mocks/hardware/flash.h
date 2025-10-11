#pragma once

#include <stddef.h>
#include <stdint.h>

// Mock flash functions
#define FLASH_PAGE_SIZE 256
#define FLASH_SECTOR_SIZE 4096
#define FLASH_BLOCK_SIZE 65536

static inline void flash_range_erase(uint32_t flash_offs, size_t count)
{
}
static inline void flash_range_program(uint32_t flash_offs, const uint8_t *data,
                                       size_t count)
{
}
