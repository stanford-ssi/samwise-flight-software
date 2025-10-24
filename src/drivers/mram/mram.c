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
 * Maximum number of tracked allocations
 */
#define MAX_MRAM_ALLOCATIONS 32

// Memory allocation tracking
static mram_allocation_t allocations[MAX_MRAM_ALLOCATIONS];
static bool allocation_system_initialized = false;

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

    // Free any allocations that overlap with the clear region
    if (allocation_system_initialized)
    {
        for (int i = 0; i < MAX_MRAM_ALLOCATIONS; i++)
        {
            if (allocations[i].in_use &&
                mram_ranges_overlap(address, length, allocations[i].address,
                                    allocations[i].length))
            {
                // Use helper function for cleaner code
                mram_free_allocation(allocations[i].address);
            }
        }
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

    // Auto-initialize allocation tracking if needed
    if (!allocation_system_initialized)
    {
        mram_allocation_init();
    }

    // Check for collision with existing tracked writes
    if (mram_check_collision(address, length))
    {
        LOG_DEBUG(
            "[mram] Write blocked: collision detected at 0x%06X (len=%zu)",
            address, length);
        return false;
    }

    // Automatically track this write
    if (!mram_register_allocation(address, length))
    {
        LOG_DEBUG("[mram] Warning: Could not track write at 0x%06X (len=%zu)",
                  address, length);
        // Continue with write anyway - tracking failure shouldn't block valid
        // writes
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

/**
 * Initialize the MRAM allocation tracking system
 */
void mram_allocation_init(void)
{
    for (int i = 0; i < MAX_MRAM_ALLOCATIONS; i++)
    {
        allocations[i].address = 0;
        allocations[i].length = 0;
        allocations[i].in_use = false;
    }
    allocation_system_initialized = true;
}

/**
 * Check if two memory ranges overlap
 * @param addr1 Start address of first range
 * @param len1 Length of first range
 * @param addr2 Start address of second range
 * @param len2 Length of second range
 * @return true if ranges overlap, false otherwise
 */
bool mram_ranges_overlap(uint32_t addr1, size_t len1, uint32_t addr2,
                         size_t len2)
{
    if (len1 == 0 || len2 == 0)
    {
        return false;
    }

    uint32_t end1 = addr1 + len1 - 1;
    uint32_t end2 = addr2 + len2 - 1;

    return !(end1 < addr2 || end2 < addr1);
}

/**
 * Register a memory allocation
 * @param address Start address of allocation
 * @param length Size of allocation in bytes
 * @return true if successfully registered, false if no space or collision
 */
bool mram_register_allocation(uint32_t address, size_t length)
{
    if (!allocation_system_initialized)
    {
        mram_allocation_init();
    }

    if (length == 0)
    {
        return false;
    }

    if (mram_check_collision(address, length))
    {
        LOG_DEBUG("[mram] Cannot register allocation at 0x%06X (len=%zu): "
                  "collision detected",
                  address, length);
        return false;
    }

    for (int i = 0; i < MAX_MRAM_ALLOCATIONS; i++)
    {
        if (!allocations[i].in_use)
        {
            allocations[i].address = address;
            allocations[i].length = length;
            allocations[i].in_use = true;
            return true;
        }
    }

    LOG_DEBUG("[mram] Cannot register allocation: no free tracking slots");
    return false;
}

/**
 * Check if a new allocation would collide with existing ones
 * @param address Start address to check
 * @param length Size to check in bytes
 * @return true if collision detected, false if safe
 */
bool mram_check_collision(uint32_t address, size_t length)
{
    if (!allocation_system_initialized)
    {
        return false;
    }

    if (length == 0)
    {
        return false;
    }

    for (int i = 0; i < MAX_MRAM_ALLOCATIONS; i++)
    {
        if (allocations[i].in_use)
        {
            if (mram_ranges_overlap(address, length, allocations[i].address,
                                    allocations[i].length))
            {
                LOG_INFO("[mram] Collision detected: 0x%06X-0x%06X overlaps "
                         "with existing 0x%06X-0x%06X",
                         address, address + length - 1, allocations[i].address,
                         allocations[i].address + allocations[i].length - 1);
                return true;
            }
        }
    }

    return false;
}

/**
 * Free a previously registered allocation
 * @param address Start address of allocation to free
 * @return true if successfully freed, false if not found
 */
bool mram_free_allocation(uint32_t address)
{
    if (!allocation_system_initialized)
    {
        return false;
    }

    for (int i = 0; i < MAX_MRAM_ALLOCATIONS; i++)
    {
        if (allocations[i].in_use && allocations[i].address == address)
        {
            allocations[i].in_use = false;
            return true;
        }
    }

    LOG_DEBUG("[mram] Cannot free allocation at 0x%06X: not found", address);
    return false;
}
