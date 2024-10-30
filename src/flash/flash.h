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

/**
 * @brief Structure to persistently store reboot count.
 */
typedef struct {
    uint32_t reboot_counter;
} persistent_data_t;

/**
 * @brief Initialize persistent data structure, setting reboot counter to 1 if uninitialized.
 * @return Pointer to the persistent data.
 */
persistent_data_t *init_persistent_data();

/**
 * @brief Write the persistent data to flash memory.
 * @param data Pointer to the data to write.
 */
void write_persistent_data(persistent_data_t *data);

/**
 * @brief Read the persistent data from flash memory.
 * @return Pointer to the persistent data in flash.
 */
const persistent_data_t *read_persistent_data();

#endif