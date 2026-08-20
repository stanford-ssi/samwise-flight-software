/**
 * Boot-device abstraction for the OTA task.
 *
 * The OTA image is written to whichever chip the RP2350 executes from:
 *   - flight hardware (PiCubed): the MR25H40MDF MRAM
 *   - a bare RP2350 dev board:   the QSPI NOR flash
 *
 * The two differ in one way that matters here: NOR flash must be erased
 * before it can be programmed, MRAM must not be (and has no erase op).
 * Hiding that behind ota_dev_* keeps ota_task.c free of device conditionals
 * and, more usefully, lets the real OTA task be exercised on a dev board
 * instead of only on assembled flight hardware.
 *
 * Selected by the MRAM build define, the same one that picks the filesystem
 * backend, so a build always targets one device consistently.
 *
 * TEST builds deliberately take the MRAM branch too: host tests verify the
 * written image by reading it back out of the MRAM mock, which is the
 * behaviour flight hardware has. The flash branch is for RP2350 boards that
 * boot from NOR flash and have no MRAM fitted.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hardware/flash.h" // FLASH_PAGE_SIZE / FLASH_SECTOR_SIZE

#if defined(MRAM) || defined(TEST)

#include "mram.h"

// MRAM is written in place; there is no erase cycle.
static inline bool ota_dev_needs_erase(void)
{
    return false;
}

static inline void ota_dev_erase_sector(uint32_t offset)
{
    (void)offset;
}

static inline bool ota_dev_write_page(uint32_t offset, const uint8_t *data,
                                      size_t len)
{
    return mram_write(offset, data, len);
}

static inline const char *ota_dev_name(void)
{
    return "MRAM";
}

#else

/*
 * NOR flash. Erase is mandatory: programming can only clear bits, so writing
 * over un-erased flash silently produces garbage rather than failing.
 */
static inline bool ota_dev_needs_erase(void)
{
    return true;
}

static inline void ota_dev_erase_sector(uint32_t offset)
{
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

static inline bool ota_dev_write_page(uint32_t offset, const uint8_t *data,
                                      size_t len)
{
    flash_range_program(offset, data, len);
    return true; // flash_range_program cannot report failure
}

static inline const char *ota_dev_name(void)
{
    return "flash";
}

#endif
