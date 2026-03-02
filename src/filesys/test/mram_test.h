#pragma once

#include "error.h"
#include "logger.h"
#include "mram.h"
#include "pico/stdlib.h"
#include <stdbool.h>
#include <string.h>

#ifndef TEST
#include "hardware_test_assert.h" /* must be last — overrides ASSERT in BRINGUP */
#endif

int test_mram_write_read(void);
int test_mram_write_disable_enable(void);
int test_mram_preserve_data_on_sleep(void);
int test_mram_clear(void);
int test_mram_read_status(void);

int test_mram_write_max_length_boundary(void);
int test_mram_write_exceeds_max_length(void);
int test_mram_clear_exceeds_max_length(void);
int test_mram_write_overwrite(void);
int test_mram_multiple_independent_regions(void);
int test_mram_adjacent_regions_no_bleed(void);
int test_mram_full_byte_range(void);
int test_mram_single_byte_write_read(void);
