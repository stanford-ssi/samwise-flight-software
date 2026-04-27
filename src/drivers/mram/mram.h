/**
 * @author Lundeen Cahilly
 * @date 2025-08-18
 *
 * This file contains functions for interfacing with MR25H40MDF
 * MRAM via QMI direct mode (CS1 / GPIO47) on a RP2350B chip.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Initialize MRAM and wake from sleep mode
 */
void mram_init(void);

/**
 * Read status register from MRAM
 * @return Status register value
 */
uint8_t mram_read_status(void);

/**
 * Enable write operations on MRAM
 */
void mram_write_enable(void);

/**
 * Disable write operations on MRAM
 */
void mram_write_disable(void);

/**
 * Put MRAM into low power sleep mode
 */
void mram_sleep(void);

/**
 * Wake MRAM from sleep mode
 */
void mram_wake(void);

/**
 * Read data from MRAM at specified address
 * @param address 24-bit address to read from
 * @param data Buffer to store read data
 * @param length Number of bytes to read (max 256)
 */
void mram_read(uint32_t address, uint8_t *data, size_t length);

/**
 * Clear/erase a region of MRAM by writing zeros
 * @param address 24-bit address to clear
 * @param length Number of bytes to clear (max 256)
 */
void mram_clear(uint32_t address, size_t length);

/**
 * Write data to MRAM at specified address
 * @param address 24-bit address to write to
 * @param data Buffer containing data to write
 * @param length Number of bytes to write (max 256)
 * @return true if write succeeded, false if length exceeds maximum
 */
bool mram_write(uint32_t address, const uint8_t *data, size_t length);
