#pragma once

#include "error.h"
#include "logger.h"
#include "mram.h"
#include "pico/stdlib.h"
#include "test_harness.h"
#include <stdbool.h>
#include <string.h>

int mram_test_setup(slate_t *slate);

int test_mram_write_read(slate_t *slate);
int test_mram_write_disable_enable(slate_t *slate);
int test_mram_preserve_data_on_sleep(slate_t *slate);
int test_mram_clear(slate_t *slate);
int test_mram_read_status(slate_t *slate);

int test_mram_write_max_length_boundary(slate_t *slate);
int test_mram_write_exceeds_max_length(slate_t *slate);
int test_mram_clear_exceeds_max_length(slate_t *slate);
int test_mram_write_overwrite(slate_t *slate);
int test_mram_multiple_independent_regions(slate_t *slate);
int test_mram_adjacent_regions_no_bleed(slate_t *slate);
int test_mram_full_byte_range(slate_t *slate);
int test_mram_single_byte_write_read(slate_t *slate);
int test_mram_large_write_read(slate_t *slate);

int test_flash_wrap_prog_read(slate_t *slate);
int test_flash_wrap_erase(slate_t *slate);
int test_flash_wrap_sync(slate_t *slate);
int test_flash_wrap_offset_within_block(slate_t *slate);
int test_flash_wrap_lfs_format_mount(slate_t *slate);
int test_flash_wrap_lfs_write_read_file(slate_t *slate);

extern const test_harness_case_t mram_tests[];
extern const size_t mram_tests_len;
