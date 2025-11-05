#include "mram.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MOCK_MRAM_SIZE 4096

static uint8_t mock_mram[MOCK_MRAM_SIZE];
static bool write_enabled = false;

void mram_init(void)
{
    write_enabled = true;
    printf("[Mock MRAM] Initialized\n");
}

// NOTE: This function only works on read/write permissions on the
// entire chip, rather than interacting with BP0, BP1, etc.
uint8_t mram_read_status(void)
{
    return write_enabled ? 0x02 : 0x00;
}

void mram_write_enable(void)
{
    write_enabled = true;
    printf("[Mock MRAM] Write enabled\n");
}

void mram_write_disable(void)
{
    write_enabled = false;
    printf("[Mock MRAM] Write disabled\n");
}

void mram_sleep(void)
{
    printf("[Mock MRAM] Entering sleep mode\n");
}

void mram_wake(void)
{
    printf("[Mock MRAM] Waking up\n");
}

void mram_read(uint32_t address, uint8_t *data, size_t length)
{
    if (address + length > MOCK_MRAM_SIZE)
    {
        printf("[Mock MRAM] Read out of bounds\n");
        return;
    }
    memcpy(data, &mock_mram[address], length);
    printf("[Mock MRAM] Read %zu bytes from address 0x%06X\n", length, address);
}

void mram_clear(uint32_t address, size_t length)
{
    if (address + length > MOCK_MRAM_SIZE)
    {
        printf("[Mock MRAM] Clear out of bounds\n");
        return;
    }
    memset(&mock_mram[address], 0, length);
    printf("[Mock MRAM] Cleared %zu bytes at address 0x%06X\n", length,
           address);
}

bool mram_write(uint32_t address, const uint8_t *data, size_t length)
{
    if (!write_enabled)
    {
        printf("[Mock MRAM] Write failed: write not enabled\n");
        return false;
    }
    if (address + length > MOCK_MRAM_SIZE)
    {
        printf("[Mock MRAM] Write out of bounds\n");
        return false;
    }
    memcpy(&mock_mram[address], data, length);
    printf("[Mock MRAM] Wrote %zu bytes to address 0x%06X\n", length, address);
    return true;
}

void mram_allocation_init(void)
{
    printf("[Mock MRAM] Allocation system initialized\n");
}

bool mram_ranges_overlap(uint32_t addr1, size_t len1, uint32_t addr2,
                         size_t len2)
{
    return (addr1 < addr2 + len2) && (addr2 < addr1 + len1);
}

bool mram_register_allocation(uint32_t address, size_t length)
{
    printf("[Mock MRAM] Registered allocation at address 0x%06X, length %zu\n",
           address, length);
    return true;
}

bool mram_check_collision(uint32_t address, size_t length)
{
    // Mock implementation: Always return false (no collision)
    printf("[Mock MRAM] Checked collision at address 0x%06X, length %zu\n",
           address, length);
    return false;
}

bool mram_free_allocation(uint32_t address)
{
    // Mock implementation: Always return true (successful free)
    printf("[Mock MRAM] Freed allocation at address 0x%06X\n", address);
    return true;
}