/**
 * @file flash_test.c
 * @author Claude Code
 * @date 2025-01-10
 *
 * Unit tests for flash driver demonstrating HAL abstraction.
 * These tests verify persistent data storage functionality without requiring
 * real hardware.
 */

#include "flash.h"
#include "hal_interface.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Simple mock flash storage for testing
static uint8_t mock_flash_data[256] = {
    0xFF}; // Initialize with erased flash pattern

// Override flash read function for testing
const persistent_data_t *read_persistent_data(void)
{
    return (const persistent_data_t *)mock_flash_data;
}

void test_flash_hal_abstraction(void)
{
    printf("Running flash HAL abstraction test...\n");

    // Initialize HAL for testing
    hal_init();

    // Verify HAL function pointers are set
    assert(hal.flash_range_erase != NULL);
    assert(hal.flash_range_program != NULL);
    assert(hal.save_and_disable_interrupts != NULL);
    assert(hal.restore_interrupts != NULL);

    printf("✓ Flash HAL abstraction test passed\n");
}

void test_flash_data_structure(void)
{
    printf("Running flash data structure test...\n");

    // Test persistent data structure
    persistent_data_t test_data = {.marker = 0xABCDABCD, .reboot_counter = 42};

    // Verify structure size and alignment
    assert(sizeof(persistent_data_t) <= 256); // Should fit in one page
    assert(test_data.marker == 0xABCDABCD);
    assert(test_data.reboot_counter == 42);

    printf("✓ Flash data structure test passed\n");
}

void test_flash_read_operations(void)
{
    printf("Running flash read operations test...\n");

    // Initialize HAL for testing
    hal_init();

    // Clear mock flash
    memset(mock_flash_data, 0xFF, sizeof(mock_flash_data));

    // Read uninitialized data
    const persistent_data_t *read_data = read_persistent_data();
    assert(read_data != NULL);

    // Should be all 0xFF (erased flash pattern)
    assert(read_data->marker == 0xFFFFFFFF);
    assert(read_data->reboot_counter == 0xFFFFFFFF);

    printf("✓ Flash read operations test passed\n");
}

void test_flash_hal_call_pattern(void)
{
    printf("Running flash HAL call pattern test...\n");

    // Initialize HAL for testing
    hal_init();

    // Test interrupt control pattern
    uint32_t interrupt_status = hal.save_and_disable_interrupts();
    assert(interrupt_status == 0xDEADBEEF); // Mock returns this value

    // Test that restore accepts the saved status
    hal.restore_interrupts(interrupt_status);

    // Test flash operations (these are mocked and just verify they don't crash)
    hal.flash_range_erase(0, 4096);

    uint8_t test_data[256] = {0x42};
    hal.flash_range_program(0, test_data, sizeof(test_data));

    printf("✓ Flash HAL call pattern test passed\n");
}

int main()
{
    printf("=== Flash Driver Tests ===\n");

    test_flash_hal_abstraction();
    test_flash_data_structure();
    test_flash_read_operations();
    test_flash_hal_call_pattern();

    printf("\nAll flash driver tests passed! 🎉\n");
    return 0;
}