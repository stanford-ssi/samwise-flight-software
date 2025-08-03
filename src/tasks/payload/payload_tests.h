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

typedef bool (*payload_task_fn_t)(slate_t *slate, char *p_args, char *s_args,
                                  char *flags);
typedef struct
{
    const char *fn_name;
    char *p_args;
    char *s_args;
    char *flags;
    payload_task_fn_t fn;
} payload_unit_test_t;

/* Macros to load function into test array */
#define ENTER_TEST(fn, p, s, f)                                                \
    (payload_unit_test_t)                                                      \
    {                                                                          \
        .fn_name = #fn, .p_arg = (p), .s_args = (s), .flags = (s), .fn = (fn)  \
    }
#define GET_ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

bool run_test_sequence(size_t n, const payload_unit_test_t *fns,
                       char *seq_name);
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
bool ping_command_test(slate_t *slate, char *p, char *s, char *f);
bool take_picture_command_test(slate_t *slate, char *p, char *s, char *f);
bool send_2400_command_test(slate_t *slate, char *p, char *s, char *f);

/***        BRINGUP TESTS       ***/
bool power_on_off_payload_test(slate_t *slate);

/***        BREADTH TESTS       ***/
/** Camera Related Breadth Tests **/
bool payload_camera_breadth_test(slate_t *slate, char *file_name,
                                 char *photo_args, char *downlink_args);
