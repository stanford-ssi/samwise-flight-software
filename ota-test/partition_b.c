/**
 * Partition B - New Features Application
 * 
 * This contains the latest features and updates.
 * If this crashes, the bootloader will revert to partition A.
 */

#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include <stdio.h>

/* ============================================================================
 * CRASH DETECTION
 * ============================================================================ */

#define CRASH_FLAG_REGISTER 4
#define BOOT_COUNT_REGISTER 5
#define PARTITION_B_RUNNING_REGISTER 6

/**
 * Clear crash detection flags
 * MUST be called immediately after successful initialization!
 */
void partition_b_initialized(void) {
    // Clear initialization crash flag
    watchdog_hw->scratch[CRASH_FLAG_REGISTER] = 0;
    watchdog_hw->scratch[BOOT_COUNT_REGISTER] = 0;
    
    // Keep PARTITION_B_RUNNING_REGISTER set - we're still running!
    // This is set by bootloader and helps detect runtime crashes
    
    printf("✓ Partition B initialized successfully\n");
    printf("✓ Crash detection flags cleared\n");
}

/**
 * Enable hardware watchdog for runtime crash detection
 */
void enable_watchdog_protection(uint32_t timeout_ms) {
    watchdog_enable(timeout_ms, true);
    printf("✓ Watchdog enabled (%lu ms timeout)\n", timeout_ms);
    printf("  Must call watchdog_update() every %lu ms\n", timeout_ms);
}

/* ============================================================================
 * NEW FEATURES (Example)
 * ============================================================================ */

/**
 * Example new feature: Advanced sensor processing
 */
void advanced_sensor_processing(void) {
    // Simulate complex sensor processing
    static uint32_t sensor_value = 0;
    
    sensor_value = (sensor_value + 17) % 1000;
    
    if (sensor_value % 100 == 0) {
        printf("  Sensor processing: value=%lu\n", sensor_value);
    }
}

/**
 * Example new feature: Network communications
 */
void network_communications(void) {
    // Simulate network activity
    static uint32_t packets_sent = 0;
    
    packets_sent++;
    
    if (packets_sent % 60 == 0) {
        printf("  Network: %lu packets sent\n", packets_sent);
    }
}

/**
 * Example new feature: Machine learning inference
 */
void ml_inference(void) {
    // Simulate ML processing
    static float confidence = 0.0f;
    
    confidence = (confidence + 0.01f);
    if (confidence > 1.0f) confidence = 0.0f;
    
    if ((int)(confidence * 100) % 25 == 0) {
        printf("  ML inference: confidence=%.2f\n", confidence);
    }
}

/* ============================================================================
 * APPLICATION LOOP
 * ============================================================================ */

void run_partition_b_application(void) {
    printf("\n");
    printf("====================================\n");
    printf("Partition B - New Features\n");
    printf("====================================\n");
    printf("Running latest experimental code\n");
    printf("\n");
    
    // Setup LED
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    printf("Commands:\n");
    printf("  'r' - Reboot\n");
    printf("  's' - Show status\n");
    printf("  'c' - Simulate crash (for testing)\n");
    printf("\n");
    
    uint32_t loop_count = 0;
    
    while (true) {
        // Blink LED faster to show we're partition B
        gpio_put(LED_PIN, (loop_count % 4) < 2);
        
        // Run new features
        advanced_sensor_processing();
        network_communications();
        ml_inference();
        
        // CRITICAL: Feed the watchdog!
        // Without this, watchdog will timeout and force reboot
        watchdog_update();
        
        // Check for serial commands
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            switch (c) {
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
                    printf("  Partition: B (New Features)\n");
                    printf("  Uptime: %lu seconds\n", loop_count / 10);
                    printf("  Watchdog: Active\n");
                    printf("\n");
                    break;
                    
                case 'c':
                case 'C':
                    printf("\n");
                    printf("SIMULATING CRASH FOR TESTING\n");
                    printf("Device will reboot to bootloader...\n");
                    printf("Bootloader should detect crash and boot partition A\n");
                    sleep_ms(2000);
                    
                    // Simulate crash by entering infinite loop
                    // Watchdog will timeout and force reboot
                    printf("Entering infinite loop (watchdog will trigger)...\n");
                    while (1) {
                        tight_loop_contents();
                        // NOT calling watchdog_update() - watchdog will timeout!
                    }
                    break;
            }
        }
        
        loop_count++;
        sleep_ms(100);  // 10 Hz update rate
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
    printf("║  PARTITION B - NEW FEATURES        ║\n");
    printf("╚════════════════════════════════════╝\n");
    printf("\n");
    printf("Build: %s %s\n", __DATE__, __TIME__);
    printf("SDK Version: %s\n", PICO_SDK_VERSION_STRING);
    printf("\n");
    
    printf("Initializing hardware...\n");
    // Initialize your hardware here
    sleep_ms(500);
    
    printf("Hardware initialization complete\n");
    printf("\n");
    
    // CRITICAL: Clear crash detection flags ASAP
    // This tells bootloader we successfully initialized
    partition_b_initialized();
    
    // CRITICAL: Enable watchdog for runtime crash detection
    // Must call watchdog_update() at least every 8 seconds
    enable_watchdog_protection(8000);
    
    printf("\n");
    
    // Run application
    run_partition_b_application();
    
    return 0;
}

/* ============================================================================
 * OPTIONAL: Custom Crash Handlers
 * ============================================================================ */

/**
 * Hard fault handler
 * Called when a serious error occurs (null pointer, illegal instruction, etc.)
 */
void __attribute__((naked)) HardFault_Handler(void) {
    // Save crash information to watchdog scratch if needed
    // Then let watchdog reboot us
    
    // Simple implementation: just hang and let watchdog timeout
    while (1) {
        tight_loop_contents();
    }
}

/**
 * Example: Catch assertion failures
 */
void __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("\n");
    printf("====================================\n");
    printf("ASSERTION FAILED\n");
    printf("====================================\n");
    printf("File: %s\n", file);
    printf("Line: %d\n", line);
    printf("Function: %s\n", func);
    printf("Expression: %s\n", expr);
    printf("\n");
    printf("System will reboot via watchdog...\n");
    
    // Don't feed watchdog - let it timeout and reboot
    while (1) {
        tight_loop_contents();
    }
}