#pragma once

#include "pico/stdlib.h"

#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"
#include "sd_card.h"
#include "ff.h"
#include <stdio.h>

void sd-card_task_init();
void sd-card_task_test_rw();

extern sched_task_t sd-card_task;
