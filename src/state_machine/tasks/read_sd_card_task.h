
/**
 * @author Ashley Raigosa
 * @date 2024-11-11
 */

#pragma once

#include "scheduler/scheduler.h"
#include "slate.h"

void read_sd_card_task_init(slate_t *slate);
void read_sd_card_task_dispatch(slate_t *slate);

sched_task_t read_sd_card_task;
