/**
 * @author  Darrow Hartman
 * @date    2024-11-19
 *
 * Test for the blink task.
 */

#include "blink_task.h"
#include "testbase.h"
#include <CUnit/CUnit.h>


// Test initialization
static void test_blink_task_init(slate_t *test_slate) {
    blink_task_init(&test_slate);
    CU_ASSERT_FALSE(test_slate->led_state);
    CU_ASSERT_FALSE(gpio_get(PICO_DEFAULT_LED_PIN));
}

// Test LED toggle functionality
static void test_blink_task_dispatch(slate_t *test_slate) {
    // Initial state
    blink_task_init(&test_slate);
    CU_ASSERT_FALSE(test_slate->led_state);
    
    // First dispatch - should turn LED on
    blink_task_dispatch(&test_slate);
    CU_ASSERT_TRUE(test_slate->led_state);
    CU_ASSERT_TRUE(gpio_get(PICO_DEFAULT_LED_PIN));
    
    // Second dispatch - should turn LED off
    blink_task_dispatch(&test_slate);
    CU_ASSERT_FALSE(test_slate->led_state);
    CU_ASSERT_FALSE(gpio_get(PICO_DEFAULT_LED_PIN));
}

void test_blink_task(slate_t *test_slate) {
    CU_pSuite suite = CU_add_suite("Blink Task Tests", NULL, NULL);
    
    CU_add_test(suite, "test blink task initialization", test_blink_task_init);
    CU_add_test(suite, "test blink task dispatch", test_blink_task_dispatch);
}