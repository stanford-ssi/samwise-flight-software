/**
 * @author  Marc Aaron Reyes
 * @date    2025-08-03
 *
 * Description:
 * Aide in providing a test suite for any Payload related tasks and bringups.
 */

#pragma once

#include "pico/stdio.h"
#include "pico/stdlib.h"

#include "macros.h"
#include "payload_uart.h"
#include "slate.h"

bool run_test(slate_t *slate, char *packet, int packet_len, bool verbose);

/* ASSOCIATED PAYLOAD TESTS
 * Types of tests
 *      - Singular Payload commands
 *      - Bringup
 *      - Performance
 *      - Functionality
 *      - Error Handling
 *      - Breadth
 */

/***        PAYLOAD COMMANDS TESTS          ***/
bool ping_command_test(slate_t *slate);
bool take_picture_command_test(slate_t *slate, char *file_name, char *args);
bool send_2400_command_test(slate_t *slate, char *file_path, char *args);

/***        BRINGUP TESTS       ***/
bool power_on_off_payload_test(slate_t *slate);

/***        BREADTH TESTS       ***/
/** Camera Related Breadth Tests **/
bool payload_camera_breadth_test(slate_t *slate);
