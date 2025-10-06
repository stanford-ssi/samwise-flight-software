/**
 * @author Summit Kawakami
 * @date 2024-10-28
 *
 * This file defines types and global declarations for flash and data structure
 * persistence.
 */
#pragma once

#ifdef TEST_MODE
#include "hal_interface.h"
#else
#include "hal_interface.h"
#include "hardware/flash.h"
#endif

#include <stdint.h>
#include <stdio.h>

/**
 * @brief Structure to persistently store reboot count and initialization
 * marker.
 */
typedef struct
{
    uint32_t marker;             // Marker to verify initialization
    uint32_t reboot_counter;     // Actual counter
    uint32_t burn_wire_attempts; // Number of burn wire attempts
} persistent_data_t;

/**
 * @brief Read the persistent data from flash.
 * @return Pointer to the persistent data in flash.
 */
const persistent_data_t *read_persistent_data(void);

/**
 * @brief Write the persistent data to flash.
 * @param data Pointer to the data to write.
 */
void write_persistent_data(persistent_data_t *data);

/**
 * @brief Initialize persistent data structure, setting reboot counter to 1 if
 * uninitialized.
 * @return Pointer to the persistent data.
 */
persistent_data_t *init_persistent_data(void);

/**
 * @brief Increment the reboot counter and write to flash.
 */
void increment_reboot_counter(void);

/**
 * @brief Get the current reboot counter value.
 * @return The current reboot counter.
 */
uint32_t get_reboot_counter(void);

void increment_burn_wire_attempts();
uint32_t get_burn_wire_attempts();
void reset_burn_wire_attempts();
