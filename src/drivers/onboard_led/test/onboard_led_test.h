/**
 * @author  Claude Code
 * @date    2025-01-10
 *
 * Unit tests for the onboard LED driver.
 */

#pragma once

#ifdef TEST_MODE

// Test function declarations
void test_onboard_led_init(void);
void test_onboard_led_set_get(void);
void test_onboard_led_toggle(void);

#endif // TEST_MODE