/**
 * @author  Darrow Hartman
 * @date    2024-11-09
 *
 * This file contains the main entry point for the SAMWISE flight code.
 */


#include "testbase.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h> 
#include "state_machine/tasks/blink_task.h"

slate_t test_slate;


// Initialize the test environment
static int suite_init(void) {
    // Initialize Pico hardware
    stdio_init_all();
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return 0;
}

// Cleanup after tests
static int suite_cleanup(void) {
    // Reset LED state
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    return 0;
}

int testbase(void) {
    // Initialize the CUnit test registry
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }
    
    // Run all test suites
    printf("Running test_blink_task\n");
    test_blink_task(&test_slate);

    // Run tests using basic interface
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    // Get the number of failures
    unsigned int num_failures = CU_get_number_of_failures();

    // Cleanup
    CU_cleanup_registry();
    printf("Testbase completed with %d failures\n", num_failures);
    // Return 0 if all tests passed, 1 if any failed
    return (num_failures == 0) ? 0 : 1;
}