#include "mram.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Match the MRAM size from config.h (FILESYS_BLOCK_SIZE * FILESYS_BLOCK_COUNT)
#define MOCK_MRAM_SIZE (1024 * 512)

static uint8_t mock_mram[MOCK_MRAM_SIZE];
static bool write_enabled = false;

void mram_init(void)
{
    write_enabled = false;
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
    write_enabled = false;
    printf("[Mock MRAM] Waking up\n");
}

void mram_read(uint32_t address, uint8_t *data, size_t length)
{
    if (length > 256)
    {
        return;
    }

    if (address + length > MOCK_MRAM_SIZE)
    {
        printf("[Mock MRAM] Read out of bounds\n");
        return;
    }

    memcpy(data, &mock_mram[address], length);
}

void mram_clear(uint32_t address, size_t length)
{
    if (length > 256)
    {
        return;
    }

    if (address + length > MOCK_MRAM_SIZE)
    {
        printf("[Mock MRAM] Clear out of bounds\n");
        return;
    }

    write_enabled = true;
    memset(&mock_mram[address], 0, length);
}

bool mram_write(uint32_t address, const uint8_t *data, size_t length)
{
    if (length > 256)
    {
        return false;
    }

    if (address + length > MOCK_MRAM_SIZE)
    {
        printf("[Mock MRAM] Write out of bounds\n");
        return false;
    }

    write_enabled = true;
    memcpy(&mock_mram[address], data, length);
    return true;
}
