/**
 * @author Lundeen Cahilly
 * @date 2025-08-18
 *
 * This file contains functions for interfacing with MR25H40MDF
 * MRAM using QSPI on a RP2350B chip.
 */

#include "mram.h"

#include <string.h>

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

#include "logger.h"
#include "macros.h"

// MRAM COMMANDS
#define WREN_CMD 0x06 // Write Enable
#define WRDI_CMD 0x04 // Write Disable
#define RDSR_CMD 0x05 // Read Status Register
#define WRSR_CMD 0x01 // Write Status Register
#define READ_CMD 0x03
#define WRITE_CMD 0x02
#define SLEEP_CMD 0xB9
#define WAKE_CMD 0xAB

// MRAM Timing Constants
#define WAKE_TIME_US 400 // Wake-up time in microseconds

// Status Register Bit Masks
#define STATUS_WEL_BIT 0x02  // Write Enable Latch (bit 1)
#define STATUS_BP0_BIT 0x04  // Block Protect 0 (bit 2)
#define STATUS_BP1_BIT 0x08  // Block Protect 1 (bit 3)
#define STATUS_SRWD_BIT 0x80 // Status Register Write Disable (bit 7)

/**
 * Initialize MRAM and wake from sleep mode
 */
void mram_init(void)
{
    LOG_INFO("[mram] Init start");

    uint8_t wake_cmd = WAKE_CMD;

    uint32_t interrupts = save_and_disable_interrupts();
    flash_do_cmd(&wake_cmd, NULL, 1);
    restore_interrupts(interrupts);

    sleep_us(WAKE_TIME_US);

    LOG_INFO("[mram] Init complete");
}

/**
 * Read status register from MRAM
 * @return Status register value
 */
uint8_t mram_read_status(void)
{
    uint8_t tx_buf[2] = {RDSR_CMD, 0x00};
    uint8_t rx_buf[2] = {0x00, 0x00};

    uint32_t interrupts = save_and_disable_interrupts();
    flash_do_cmd(tx_buf, rx_buf, 2);
    restore_interrupts(interrupts);

    uint8_t status = rx_buf[1];
    return status;
}

/**
 * Enable write operations on MRAM
 */
void mram_write_enable(void)
{
    uint8_t cmd = WREN_CMD;

    uint32_t interrupts = save_and_disable_interrupts();
    flash_do_cmd(&cmd, NULL, 1);
    restore_interrupts(interrupts);
}

/**
 * Read data from MRAM at specified address
 * @param address 24-bit address to read from
 * @param data Buffer to store read data
 * @param length Number of bytes to read (max 256)
 */
void mram_read(uint32_t address, uint8_t *data, size_t length)
{
    if (length > 256)
    {
        return;
    }

    static uint8_t cmd_buf[256 + 4];
    static uint8_t rx_buf[256 + 4];

    cmd_buf[0] = READ_CMD;
    cmd_buf[1] = (address >> 16) & 0xFF;
    cmd_buf[2] = (address >> 8) & 0xFF;
    cmd_buf[3] = address & 0xFF;

    for (size_t i = 0; i < length; i++)
    {
        cmd_buf[4 + i] = 0x00;
    }

    uint32_t interrupts = save_and_disable_interrupts();
    flash_do_cmd(cmd_buf, rx_buf, 4 + length);
    restore_interrupts(interrupts);

    memcpy(data, &rx_buf[4], length);
}

/**
 * Clear/erase a region of MRAM by writing zeros
 * @param address 24-bit address to clear
 * @param length Number of bytes to clear (max 256)
 */
void mram_clear(uint32_t address, size_t length)
{
    if (length > 256)
    {
        LOG_INFO("[mram] Clear failed: length %zu exceeds maximum", length);
        return;
    }

    mram_write_enable();

    static uint8_t clear_buf[256 + 4];

    // Set up command and address
    clear_buf[0] = WRITE_CMD;
    clear_buf[1] = (address >> 16) & 0xFF;
    clear_buf[2] = (address >> 8) & 0xFF;
    clear_buf[3] = address & 0xFF;

    // Fill with zeros
    memset(&clear_buf[4], 0x00, length);

    uint32_t interrupts = save_and_disable_interrupts();
    flash_do_cmd(clear_buf, NULL, 4 + length);
    restore_interrupts(interrupts);
}

/**
 * Write data to MRAM at specified address
 * @param address 24-bit address to write to
 * @param data Buffer containing data to write
 * @param length Number of bytes to write (max 256)
 */
bool mram_write(uint32_t address, const uint8_t *data, size_t length)
{
    if (length > 256)
    {
        LOG_DEBUG("[mram] Write failed: length %zu exceeds maximum", length);
        return false;
    }

    mram_write_enable();

    static uint8_t cmd_buf[256 + 4];

    cmd_buf[0] = WRITE_CMD;
    cmd_buf[1] = (address >> 16) & 0xFF;
    cmd_buf[2] = (address >> 8) & 0xFF;
    cmd_buf[3] = address & 0xFF;

    memcpy(&cmd_buf[4], data, length);

    uint32_t interrupts = save_and_disable_interrupts();
    flash_do_cmd(cmd_buf, NULL, 4 + length);
    restore_interrupts(interrupts);

    return true;
}

/**
 * Disable write operations on MRAM
 */
void mram_write_disable(void)
{
    uint8_t cmd = WRDI_CMD;

    uint32_t interrupts = save_and_disable_interrupts();
    flash_do_cmd(&cmd, NULL, 1);
    restore_interrupts(interrupts);
}

/**
 * Put MRAM into low power sleep mode
 */
void mram_sleep(void)
{
    uint8_t cmd = SLEEP_CMD;

    uint32_t interrupts = save_and_disable_interrupts();
    flash_do_cmd(&cmd, NULL, 1);
    restore_interrupts(interrupts);
}

/**
 * Wake MRAM from sleep mode
 */
void mram_wake(void)
{
    uint8_t cmd = WAKE_CMD;

    uint32_t interrupts = save_and_disable_interrupts();
    flash_do_cmd(&cmd, NULL, 1);
    restore_interrupts(interrupts);

    sleep_us(WAKE_TIME_US);
}
