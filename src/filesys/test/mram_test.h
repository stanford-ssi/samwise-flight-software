#pragma once

#include "error.h"
#include "logger.h"
#include "mram.h"
#include "pico/stdlib.h"
#include "test_harness.h"

#include <string.h>

int read_write_helper(char *str, int length);
int test_mram_write_read(void);
int test_mram_write_disable_enable(void);
int test_mram_check_collision(void);
int test_mram_preserve_data_on_sleep(void);
int test_mram_clear(void);
int test_mram_read_status(void);
