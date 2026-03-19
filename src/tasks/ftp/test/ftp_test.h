#pragma once

#include "config.h"
#include "filesys.h"
#include "ftp_task.h"
#include "logger.h"
#include "test_harness.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Setup */
int ftp_test_setup(slate_t *slate);

/* A. Packet Tracker Tests */
int ftp_test_tracker_clear(slate_t *slate);
int ftp_test_tracker_set_and_check(slate_t *slate);
int ftp_test_tracker_check_mask_full_bytes(slate_t *slate);
int ftp_test_tracker_check_mask_partial_byte(slate_t *slate);
int ftp_test_tracker_check_mask_incomplete(slate_t *slate);

/* B. Reformat Tests */
int ftp_test_reformat_success(slate_t *slate);
int ftp_test_reformat_clears_state(slate_t *slate);

/* C. Start File Write Tests */
int ftp_test_start_write_success(slate_t *slate);
int ftp_test_start_write_already_writing(slate_t *slate);
int ftp_test_start_write_sets_slate_state(slate_t *slate);

/* D. Write File Data Tests */
int ftp_test_write_data_no_file(slate_t *slate);
int ftp_test_write_data_single_packet_file(slate_t *slate);
int ftp_test_write_data_mid_cycle_no_response(slate_t *slate);
int ftp_test_write_data_complete_cycle_not_final(slate_t *slate);
int ftp_test_write_data_complete_final_cycle(slate_t *slate);
int ftp_test_write_data_out_of_range_too_low(slate_t *slate);
int ftp_test_write_data_out_of_range_too_high(slate_t *slate);
int ftp_test_write_data_duplicate_ignored(slate_t *slate);
int ftp_test_write_data_out_of_order(slate_t *slate);
int ftp_test_write_data_last_packet_partial_size(slate_t *slate);

/* E. Cancel File Write Tests */
int ftp_test_cancel_success(slate_t *slate);
int ftp_test_cancel_clears_state(slate_t *slate);

/* F. End-to-End Workflow Tests */
int ftp_test_e2e_single_cycle_file(slate_t *slate);
int ftp_test_e2e_multi_cycle_file(slate_t *slate);
int ftp_test_e2e_cancel_then_new_file(slate_t *slate);
int ftp_test_e2e_crc_mismatch(slate_t *slate);

/* G. Init Tests */
int ftp_test_init_success(slate_t *slate);
int ftp_test_init_clears_state(slate_t *slate);

extern const test_harness_case_t ftp_tests[];
extern const size_t ftp_tests_len;
