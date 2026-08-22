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
#include <string.h>

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

/*
 * Read back from the target partition, to verify what was written.
 *
 * Deliberately does not go through XIP: the RP2350 remaps the XIP window to
 * the *running* partition, so an address inside the target partition is out of
 * bounds from here and faults. mram_read issues a direct QMI command and is
 * unaffected by the remapping.
 */
static inline bool ota_dev_read_page(uint32_t offset, uint8_t *data, size_t len)
{
    mram_read(offset, data, len);
    return true;
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

/*
 * Read back from the target partition, to verify what was written.
 *
 * This cannot use XIP. The RP2350 remaps the XIP window to the *running*
 * partition, so XIP_BASE + <target offset> is out of bounds and faults. Issue
 * a plain 03h READ over the flash's SPI instead, which takes an absolute
 * device address and is unaffected by the remapping.
 */
static inline bool ota_dev_read_page(uint32_t offset, uint8_t *data, size_t len)
{
    if (len > FLASH_PAGE_SIZE)
    {
        return false;
    }

    // 1 command byte + 3 address bytes, then len bytes clocked out.
    uint8_t tx[4 + FLASH_PAGE_SIZE];
    uint8_t rx[4 + FLASH_PAGE_SIZE];

    tx[0] = 0x03; // READ
    tx[1] = (uint8_t)((offset >> 16) & 0xFF);
    tx[2] = (uint8_t)((offset >> 8) & 0xFF);
    tx[3] = (uint8_t)(offset & 0xFF);
    memset(&tx[4], 0, len);

    flash_do_cmd(tx, rx, 4 + len);
    memcpy(data, &rx[4], len);
    return true;
}

static inline const char *ota_dev_name(void)
{
    return "flash";
}

#endif
