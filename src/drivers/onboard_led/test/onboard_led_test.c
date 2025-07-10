/**
 * @author  Claude Code
 * @date    2025-01-10
 *
 * Unit tests for the onboard LED driver using HAL abstraction.
 */

#include "onboard_led_test.h"
#include "onboard_led.h"
#include "hal_interface.h"
#include <stdio.h>
#include <assert.h>

void test_onboard_led_init(void)
{
    struct onboard_led led = onboard_led_mk();
    
    // Verify initial state
    assert(led.pin == PICO_DEFAULT_LED_PIN);
    assert(led.on == false);
    
    // Initialize the LED (this should call HAL functions)
    onboard_led_init(&led);
    
    // Verify that pin state can be checked through HAL
    // This tests that the HAL mock is working correctly
    assert(hal_mock_get_pin_value(led.pin) == false);
    
    printf("✓ Onboard LED initialization test passed\n");
}

void test_onboard_led_set_get(void)
{
    struct onboard_led led = onboard_led_mk();
    onboard_led_init(&led);
    
    // Test setting LED on
    onboard_led_set(&led, true);
    assert(onboard_led_get(&led) == true);
    assert(hal_mock_get_pin_value(led.pin) == true);
    
    // Test setting LED off
    onboard_led_set(&led, false);
    assert(onboard_led_get(&led) == false);
    assert(hal_mock_get_pin_value(led.pin) == false);
    
    printf("✓ Onboard LED set/get test passed\n");
}

void test_onboard_led_toggle(void)
{
    struct onboard_led led = onboard_led_mk();
    onboard_led_init(&led);
    
    // Start with LED off
    assert(onboard_led_get(&led) == false);
    
    // Toggle to on
    onboard_led_toggle(&led);
    assert(onboard_led_get(&led) == true);
    assert(hal_mock_get_pin_value(led.pin) == true);
    
    // Toggle back to off
    onboard_led_toggle(&led);
    assert(onboard_led_get(&led) == false);
    assert(hal_mock_get_pin_value(led.pin) == false);
    
    printf("✓ Onboard LED toggle test passed\n");
}

int main(void)
{
    printf("Running onboard LED tests...\n");
    
    // Initialize HAL for testing
    hal_init();
    
    test_onboard_led_init();
    test_onboard_led_set_get();
    test_onboard_led_toggle();
    
    printf("All onboard LED tests passed!\n");
    return 0;
}