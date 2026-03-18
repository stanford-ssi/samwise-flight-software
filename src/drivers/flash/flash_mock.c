/**
 * @file flash_mock.c
 * @brief Mock implementation of flash driver for host testing.
 */

#include "flash.h"
#include <string.h>

static persistent_data_t mock_data = {0};

persistent_data_t *init_persistent_data(void)
{
    memset(&mock_data, 0, sizeof(mock_data));
    mock_data.marker = 0xDEADBEEF;
    mock_data.reboot_counter = 1;
    return &mock_data;
}

void increment_reboot_counter()
{
    mock_data.reboot_counter++;
}

uint32_t get_reboot_counter()
{
    return mock_data.reboot_counter;
}

void increment_burn_wire_attempts()
{
    mock_data.burn_wire_attempts++;
}

uint32_t get_burn_wire_attempts()
{
    return mock_data.burn_wire_attempts;
}

void reset_burn_wire_attempts()
{
    mock_data.burn_wire_attempts = 0;
}
