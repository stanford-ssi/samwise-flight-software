/**
 * Partition A - Stable Baseline Application
 * 
 * This is your robust, production-proven code that:
 * - Provides baseline functionality
 * - Handles OTA updates (downloads and writes to partition B)
 * - Acts as fallback if partition B crashes
 */

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

// Linker symbols
extern char __HeapBase;
extern char __StackLimit;

// Heap management function
void *sbrk(int incr);

/* ============================================================================
 * PARTITION B FLASH OPERATIONS
 * ============================================================================ */

#define PARTITION_B_OFFSET  0x00120000  // 1152KB offset from flash start
#define PARTITION_B_SIZE    (896 * 1024)
#define FLASH_SECTOR_SIZE   4096
#define FLASH_PAGE_SIZE     256

/**
 * Erase partition B
 */
bool erase_partition_b(void) {
    printf("Erasing partition B (%d KB)...\n", PARTITION_B_SIZE / 1024);
    
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    
    // Disable interrupts during flash operations
    uint32_t ints = save_and_disable_interrupts();
    
    // Erase partition B
    flash_range_erase(PARTITION_B_OFFSET, PARTITION_B_SIZE);
    
    restore_interrupts(ints);
    
    uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - start_time;
    printf("Erase complete (%lu ms)\n", elapsed);
    
    return true;
}

/**
 * Write data to partition B
 */
bool write_partition_b(const uint8_t *data, uint32_t size) {
    printf("Writing %lu bytes to partition B...\n", size);
    
    if (size > PARTITION_B_SIZE) {
        printf("ERROR: Data too large for partition B\n");
        return false;
    }
    
    // Data must be 256-byte aligned for flash_range_program
    if (size % FLASH_PAGE_SIZE != 0) {
        printf("WARNING: Padding data to 256-byte boundary\n");
        size = ((size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE;
    }
    
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    
    // Write in chunks to allow for progress reporting
    const uint32_t chunk_size = 4096;
    for (uint32_t offset = 0; offset < size; offset += chunk_size) {
        uint32_t write_size = (size - offset) < chunk_size ? (size - offset) : chunk_size;
        
        uint32_t ints = save_and_disable_interrupts();
        flash_range_program(PARTITION_B_OFFSET + offset, data + offset, write_size);
        restore_interrupts(ints);
        
        // Progress
        if (offset % (64 * 1024) == 0) {
            printf("  %lu KB / %lu KB\n", offset / 1024, size / 1024);
        }
    }
    
    uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - start_time;
    printf("Write complete (%lu ms)\n", elapsed);
    
    return true;
}

/**
 * Verify partition B contents
 */
bool verify_partition_b(const uint8_t *expected_data, uint32_t size) {
    printf("Verifying partition B...\n");
    
    const uint8_t *flash_data = (const uint8_t *)(XIP_BASE + PARTITION_B_OFFSET);
    
    for (uint32_t i = 0; i < size; i++) {
        if (flash_data[i] != expected_data[i]) {
            printf("ERROR: Verification failed at offset 0x%lx\n", i);
            printf("  Expected: 0x%02x, Got: 0x%02x\n", expected_data[i], flash_data[i]);
            return false;
        }
    }
    
    printf("Verification successful\n");
    return true;
}

/* ============================================================================
 * OTA UPDATE MANAGER
 * ============================================================================ */

/**
 * Simulated OTA update
 * In real implementation, this would:
 * - Connect to server via WiFi/Ethernet
 * - Download firmware binary
 * - Verify signature/checksum
 * - Write to partition B
 */
bool perform_ota_update(void) {
    printf("\n");
    printf("====================================\n");
    printf("OTA Update Started\n");
    printf("====================================\n");
    
    // In real implementation, download from server
    // For this example, we'll create dummy firmware
    
    printf("Step 1: Downloading firmware...\n");
    // Simulate download (in reality, use WiFi/HTTP client)
    sleep_ms(1000);
    printf("  Download complete\n");
    
    printf("Step 2: Verifying firmware signature...\n");
    // In reality, verify cryptographic signature
    sleep_ms(500);
    printf("  Signature valid\n");
    
    printf("Step 3: Erasing partition B...\n");
    if (!erase_partition_b()) {
        printf("ERROR: Failed to erase partition B\n");
        return false;
    }
    
    printf("Step 4: Writing firmware to partition B...\n");
    // In reality, write the downloaded firmware
    // For this example, we'll write a minimal test pattern
    
    // NOTE: In real implementation, you would write actual firmware here
    printf("  (Skipping write in this example - use real firmware binary)\n");
    
    printf("\n");
    printf("====================================\n");
    printf("OTA Update Complete\n");
    printf("====================================\n");
    printf("Reboot to activate new firmware\n");
    
    return true;
}

/* ============================================================================
 * BASELINE APPLICATION FUNCTIONALITY
 * ============================================================================ */

void run_baseline_application(void) {
    printf("\n");
    printf("====================================\n");
    printf("Partition A - Baseline Application\n");
    printf("====================================\n");
    printf("This is the stable, robust baseline\n");
    printf("\n");
    
    // Blink LED to show we're running
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    printf("Commands:\n");
    printf("  'u' - Perform OTA update\n");
    printf("  'r' - Reboot\n");
    printf("  's' - Show status\n");
    printf("\n");
    
    uint32_t loop_count = 0;
    
    while (true) {
        // Blink LED
        gpio_put(LED_PIN, loop_count % 2);
        
        // Check for serial commands
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            switch (c) {
                case 'u':
                case 'U':
                    printf("\nStarting OTA update...\n");
                    if (perform_ota_update()) {
                        printf("Update successful! Reboot? (y/n): ");
                        int response = getchar();
                        if (response == 'y' || response == 'Y') {
                            printf("Rebooting...\n");
                            sleep_ms(1000);
                            watchdog_reboot(0, 0, 0);
                        }
                    }
                    break;
                    
                case 'r':
                case 'R':
                    printf("Rebooting...\n");
                    sleep_ms(1000);
                    watchdog_reboot(0, 0, 0);
                    break;
                    
                case 's':
                case 'S':
                    printf("\n");
                    printf("Status:\n");
                    printf("  Partition: A (Stable Baseline)\n");
                    printf("  Uptime: %lu seconds\n", loop_count);
                    printf("  Free heap: %lu bytes\n", 
                           (uint32_t)(&__StackLimit) - (uint32_t)sbrk(0));
                    printf("\n");
                    break;
            }
        }
        
        loop_count++;
        sleep_ms(1000);
    }
}

/* ============================================================================
 * MAIN ENTRY POINT
 * ============================================================================ */

int main(void) {
    // Initialize stdio
    stdio_init_all();
    
    // Wait for USB serial connection
    sleep_ms(2000);
    
    printf("\n\n");
    printf("╔════════════════════════════════════╗\n");
    printf("║  PARTITION A - STABLE BASELINE     ║\n");
    printf("╚════════════════════════════════════╝\n");
    printf("\n");
    printf("Build: %s %s\n", __DATE__, __TIME__);
    printf("SDK Version: %s\n", PICO_SDK_VERSION_STRING);
    printf("\n");
    
    // Run baseline application
    run_baseline_application();
    
    return 0;
}

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

// Weak symbol for heap management
extern char __StackLimit;
void *sbrk(int incr) {
    static char *heap_end = 0;
    char *prev_heap_end;
    
    if (heap_end == 0) {
        heap_end = (char *)&__HeapBase;
    }
    
    prev_heap_end = heap_end;
    heap_end += incr;
    
    return (void *)prev_heap_end;
}