#pragma once

#include <stdint.h>

#include "filesys.h"
#include "test_harness.h"

int filesys_test_setup(slate_t *slate);
int filesys_test_write_readback_success(slate_t *slate);
int filesys_test_initialize_reformat_success(slate_t *slate);
int filesys_test_start_file_write_already_writing_should_fail(slate_t *slate);
int filesys_test_write_data_to_buffer_bounds_should_fail(slate_t *slate);
int filesys_test_write_data_to_buffer_when_no_file_started_should_fail(
    slate_t *slate);
int filesys_test_write_buffer_to_mram_when_no_file_started_should_fail(
    slate_t *slate);
int filesys_test_write_buffer_to_mram_clean_buffer_success(slate_t *slate);
int filesys_test_complete_file_write_dirty_buffer_success(slate_t *slate);
int filesys_test_cancel_file_write_success(slate_t *slate);
int filesys_test_cancel_file_write_no_file_should_fail(slate_t *slate);
int filesys_test_clear_buffer_success(slate_t *slate);
int filesys_test_crc_correct_success(slate_t *slate);
int filesys_test_crc_incorrect_should_fail(slate_t *slate);
int filesys_test_crc_no_file_should_fail(slate_t *slate);
int filesys_test_multiple_files_success(slate_t *slate);
int filesys_test_blocks_left_calculation_success(slate_t *slate);
int filesys_test_multi_chunk_write_success(slate_t *slate);
int filesys_test_write_at_offset_success(slate_t *slate);
int filesys_test_multiple_files_commit_success(slate_t *slate);
int filesys_test_write_long_file_crc32_success(slate_t *slate);
int filesys_test_file_too_large_should_fail(slate_t *slate);
int filesys_test_second_file_out_of_space_should_fail(slate_t *slate);
int filesys_test_raw_lfs_write_large_file_success(slate_t *slate);
int filesys_test_list_files_empty_filesystem_success(slate_t *slate);
int filesys_test_list_files_multiple_files_success(slate_t *slate);
int filesys_test_list_files_max_files_limit_success(slate_t *slate);
int filesys_test_list_files_after_cancel_success(slate_t *slate);
int filesys_test_list_files_crc_mismatch_success(slate_t *slate);
int filesys_test_open_file_read_success(slate_t *slate);
int filesys_test_open_file_read_crc_mismatch_should_fail(slate_t *slate);
int filesys_test_open_file_read_nonexistent_should_fail(slate_t *slate);
int filesys_test_read_data_full_readback_success(slate_t *slate);
int filesys_test_read_data_chunked_success(slate_t *slate);
int filesys_test_read_data_past_eof_success(slate_t *slate);
int filesys_test_read_file_seek_success(slate_t *slate);
int filesys_test_read_file_tell_success(slate_t *slate);
int filesys_test_read_file_size_success(slate_t *slate);
int filesys_test_close_file_read_success(slate_t *slate);
int filesys_test_read_full_workflow_success(slate_t *slate);
int filesys_test_read_multi_chunk_file_success(slate_t *slate);
int filesys_test_probe_max_file_capacity(void);

extern const test_harness_case_t filesys_tests[];
extern const size_t filesys_tests_len;
