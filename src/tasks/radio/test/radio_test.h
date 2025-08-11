/**
 * @file radio_test.h
 * @brief Test header for radio task race condition fix
 * @author Copilot
 * @date 2024
 *
 * This test would verify that the radio task doesn't attempt to start
 * a new transmission when one is already in progress, preventing the
 * race condition described in issue #74.
 */

#pragma once

#include "radio_task.h"

/**
 * Test that radio_task_dispatch doesn't interfere with ongoing transmissions
 *
 * Test scenario:
 * 1. Set up a radio with tx_queue containing packets
 * 2. Mock rfm9x_tx_done to return 0 (transmission in progress)
 * 3. Call radio_task_dispatch
 * 4. Verify that rfm9x_transmit and tx_done are NOT called
 * 5. Mock rfm9x_tx_done to return 1 (transmission complete)
 * 6. Call radio_task_dispatch again
 * 7. Verify that rfm9x_transmit and tx_done ARE called
 */
void test_radio_transmission_race_condition_fix(void);

/**
 * Test that the fix preserves existing behavior when no race condition exists
 */
void test_radio_normal_transmission_behavior(void);