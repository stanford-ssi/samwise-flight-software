/**
 * @author Lundeen Cahilly
 * @date 2025-08-18
 *
 * This file contains functions for interfacing with MR25H40MDF
 * MRAM using QSPI on a RP2350B chip.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Memory allocation tracking structure
 */
typedef struct
{
    uint32_t address;
    size_t length;
    bool in_use;
} mram_allocation_t;

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
 * @return true if write succeeded, false if blocked or failed
 */
bool mram_write(uint32_t address, const uint8_t *data, size_t length);

/**
 * Initialize the MRAM allocation tracking system
 */
void mram_allocation_init(void);

/**
 * Check if two memory ranges overlap
 * @param addr1 Start address of first range
 * @param len1 Length of first range
 * @param addr2 Start address of second range
 * @param len2 Length of second range
 * @return true if ranges overlap, false otherwise
 */
bool mram_ranges_overlap(uint32_t addr1, size_t len1, uint32_t addr2,
                         size_t len2);

/**
 * Register a memory allocation
 * @param address Start address of allocation
 * @param length Size of allocation in bytes
 * @return true if successfully registered, false if no space or collision
 */
bool mram_register_allocation(uint32_t address, size_t length);

/**
 * Check if a new allocation would collide with existing ones
 * @param address Start address to check
 * @param length Size to check in bytes
 * @return true if collision detected, false if safe
 */
bool mram_check_collision(uint32_t address, size_t length);

/**
 * Free a previously registered allocation
 * @param address Start address of allocation to free
 * @return true if successfully freed, false if not found
 */
bool mram_free_allocation(uint32_t address);
