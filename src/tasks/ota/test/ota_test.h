#pragma once

#include "slate.h"
#include "test_harness.h"

int ota_test_setup(slate_t *slate);
int ota_test_success(slate_t *slate);
int ota_test_on_b_reboots_to_a(slate_t *slate);
int ota_test_boot_info_failure(slate_t *slate);
int ota_test_file_not_found(slate_t *slate);
int ota_test_file_too_large(slate_t *slate);
int ota_test_partition_table_failure(slate_t *slate);
int ota_test_tbyb_state_after_ota(slate_t *slate);
int ota_test_tbyb_rollback_to_a(slate_t *slate);
int ota_test_tbyb_full_cycle(slate_t *slate);

extern const test_harness_case_t ota_tests[];
extern const size_t ota_tests_len;
