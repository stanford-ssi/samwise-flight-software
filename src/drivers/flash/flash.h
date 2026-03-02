/**
 * @author Summit Kawakami
 * @date 2024-10-28
 *
 * This file defines types and global declarations for flash and data structure
 * persistence.
 */
#pragma once

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
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
 * @brief Initialize persistent data structure, setting reboot counter to 1 if
 * uninitialized.
 * @return Pointer to the persistent data.
 */
persistent_data_t *init_persistent_data(void);

void increment_reboot_counter();
uint32_t get_reboot_counter();
void increment_burn_wire_attempts();
uint32_t get_burn_wire_attempts();
void reset_burn_wire_attempts();
