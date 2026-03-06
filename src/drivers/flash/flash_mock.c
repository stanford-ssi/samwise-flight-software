/**
 * @brief Mock flash driver for host tests.
 *
 * Provides stub implementations of the flash persistence functions declared in
 * flash.h so that tests can link without the real hardware-dependent driver.
 */

#include "flash.h"

static persistent_data_t mock_data = {0};

persistent_data_t *init_persistent_data(void)
{
    mock_data.marker = 0;
    mock_data.reboot_counter = 0;
    mock_data.burn_wire_attempts = 0;
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
