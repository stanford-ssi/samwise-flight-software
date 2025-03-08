#pragma once

#include "pico/stdlib.h"

#include "slate.h"
#include "sd-card.h"
#include <stdio.h>

void sdcard_task_init();
void sdcard_task_test_rw();

extern sched_task_t sdcard_task;
