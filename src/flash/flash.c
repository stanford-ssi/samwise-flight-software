/**
 * @author Summit Kawakami
 * @date 2024-10-29
 * 
 * This file contains methods to write to flash, and read and store persistent data
 * between reboots.
 */
#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "flash.h"

#define FLASH_TARGET_OFFSET (256 * 1024)
#define INIT_MARKER 0xABCDABCD // Distinct marker to indicate initialized data

// Read the persistent data from flash
const persistent_data_t *read_persistent_data() {
    const uint8_t *flash_data_ptr = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
    return (const persistent_data_t *)flash_data_ptr;
}

// Write the persistent data to flash
void write_persistent_data(persistent_data_t *data) {
    uint32_t ints = save_and_disable_interrupts();

    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);

    flash_range_program(FLASH_TARGET_OFFSET, (uint8_t *)data, FLASH_PAGE_SIZE);

    restore_interrupts(ints);
}

// Initialize the persistent data structure or load existing data
persistent_data_t *init_persistent_data() {
    static persistent_data_t data;
    const persistent_data_t *flash_data = read_persistent_data();

    // Check if the data has been initialized by looking at the marker
    if (flash_data->marker != INIT_MARKER) {
        // If uninitialized, set initial values
        data.marker = INIT_MARKER;
        data.reboot_counter = 1;
        printf("First boot detected, initializing reboot counter to 1\n");
    } else {
        // If initialized, increment the counter
        data = *flash_data;
        data.reboot_counter += 1;
        printf("Loaded reboot counter from flash: %d\n", data.reboot_counter);
    }

    write_persistent_data(&data);
    return &data;
}
