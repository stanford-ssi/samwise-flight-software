/**
 * @file hal_test.c
 * @author Claude Code
 * @date 2025-01-10
 *
 * Basic HAL functionality test to verify the hardware abstraction layer works
 * correctly.
 */

#include "hal_interface.h"
#include <assert.h>
#include <stdio.h>

// Simple test for HAL initialization
void test_hal_init()
{
    // Initialize HAL
    hal_init();

    // Verify that function pointers are not NULL
    assert(hal.gpio_init != NULL);
    assert(hal.gpio_put != NULL);
    assert(hal.gpio_get != NULL);
    assert(hal.sleep_ms != NULL);
    assert(hal.sleep_us != NULL);
    assert(hal.get_absolute_time_us != NULL);

    printf("✓ HAL initialization successful\n");
}

// Test basic GPIO operations
void test_gpio_operations()
{
    hal_pin_t test_pin = 0;

    // Initialize GPIO pin
    hal.gpio_init(test_pin);

    // Set as output
    hal.gpio_set_dir(test_pin, HAL_GPIO_OUT);

    // Set pin high
    hal.gpio_put(test_pin, true);

    // Set pin low
    hal.gpio_put(test_pin, false);

    printf("✓ GPIO operations successful\n");
}

// Test timing operations
void test_timing_operations()
{
    uint64_t start_time = hal.get_absolute_time_us();

    // Small delay
    hal.sleep_ms(1);

    uint64_t end_time = hal.get_absolute_time_us();

    // Verify time has progressed
    assert(end_time >= start_time);

    printf("✓ Timing operations successful\n");
}

#ifdef TEST_MODE
// Test mock-specific functionality
void test_mock_functionality()
{
    // Reset mock state
    hal_mock_reset();

    // Test pin state manipulation
    hal_pin_t test_pin = 1;
    hal.gpio_init(test_pin);
    hal.gpio_set_dir(test_pin, HAL_GPIO_OUT);

    // Set pin value through HAL
    hal.gpio_put(test_pin, true);

    // Verify through mock interface
    assert(hal_mock_get_pin_value(test_pin) == true);

    // Test time advancement
    uint64_t initial_time = hal.get_absolute_time_us();
    hal_mock_advance_time(1000); // Advance by 1000ms
    uint64_t advanced_time = hal.get_absolute_time_us();

    assert(advanced_time >= initial_time + 1000000); // 1000ms = 1000000us

    printf("✓ Mock functionality successful\n");
}
#endif

int main()
{
    printf("Running HAL tests...\n");

    test_hal_init();
    test_gpio_operations();
    test_timing_operations();

#ifdef TEST_MODE
    test_mock_functionality();
#endif

    printf("All HAL tests passed!\n");
    return 0;
}