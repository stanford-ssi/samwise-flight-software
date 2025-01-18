/**
 * @author Summit Kawakami
 * @date 2024-10-28
 * 
 * This file defines types and global declarations for flash and data structure
 * persistence.
 */
#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

#define INIT_MARKER 0xABCDABCD // Distinct marker to indicate initialized data

/**
 * @brief Structure to persistently store reboot count and initialization marker.
 */
typedef struct {
    uint32_t marker;         // Marker to verify initialization
    uint32_t reboot_counter; // Actual counter
} persistent_data_t;

/**
 * @brief Initialize persistent data structure, setting reboot counter to 1 if uninitialized.
 * @return Pointer to the persistent data.
 */
persistent_data_t *init_persistent_data(void);

void increment_reboot_counter();
uint32_t get_reboot_counter();

#endif // FLASH_H
