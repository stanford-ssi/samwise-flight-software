/**
 * @author  Claude Code
 * @date    2025-10-18
 *
 * File system task for MRAM testing - writes and reads random data
 */

#include "filesys_task.h"
#include "macros.h"
#include "mram.h"
#include "neopixel.h"
#include "pico/stdlib.h"
#include <stdlib.h>

#define TEST_DATA_SIZE 16
#define MRAM_MAX_ADDRESS 0x80000  // 512KB MRAM

static uint32_t test_count = 0;

void filesys_task_init(slate_t *slate)
{
    LOG_INFO("Filesys task initializing...");
    
    // Initialize MRAM
    mram_init();
    mram_allocation_init();
}

void filesys_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(FILESYS_TASK_COLOR);
    
    // Generate random address within MRAM bounds
    uint32_t random_address = rand() % (MRAM_MAX_ADDRESS - TEST_DATA_SIZE);
    
    // Create test data
    uint8_t write_data[TEST_DATA_SIZE];
    uint8_t read_data[TEST_DATA_SIZE];
    
    // Fill write buffer with test pattern
    for (int i = 0; i < TEST_DATA_SIZE; i++) {
        write_data[i] = (uint8_t)(test_count + i);
    }
    
    LOG_DEBUG("Filesys task #%lu: Writing to MRAM address 0x%lX", 
              test_count, random_address);
    
    // Write data to MRAM
    if (mram_write(random_address, write_data, TEST_DATA_SIZE)) {
        LOG_DEBUG("Write successful, reading back data...");
        
        // Clear read buffer
        for (int i = 0; i < TEST_DATA_SIZE; i++) {
            read_data[i] = 0;
        }
        
        // Read data back
        mram_read(random_address, read_data, TEST_DATA_SIZE);
        
        // Log the contents
        LOG_DEBUG("Read data: %02X %02X %02X %02X %02X %02X %02X %02X "
                  "%02X %02X %02X %02X %02X %02X %02X %02X",
                  read_data[0], read_data[1], read_data[2], read_data[3],
                  read_data[4], read_data[5], read_data[6], read_data[7],
                  read_data[8], read_data[9], read_data[10], read_data[11],
                  read_data[12], read_data[13], read_data[14], read_data[15]);
        
        // Verify data integrity
        bool data_matches = true;
        for (int i = 0; i < TEST_DATA_SIZE; i++) {
            if (write_data[i] != read_data[i]) {
                data_matches = false;
                break;
            }
        }
        
        if (data_matches) {
            LOG_DEBUG("Data verification: PASS");
        } else {
            LOG_DEBUG("Data verification: FAIL");
        }
    } else {
        LOG_DEBUG("MRAM write failed at address 0x%lX", random_address);
    }
    
    test_count++;
    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t filesys_task = {
    .name = "filesys",
    .dispatch_period_ms = 5000,  // Run every 5 seconds
    .task_init = &filesys_task_init,
    .task_dispatch = &filesys_task_dispatch,
    .next_dispatch = 0
};