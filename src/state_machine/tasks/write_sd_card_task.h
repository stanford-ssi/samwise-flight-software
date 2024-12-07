/**
 * @author Ashley Raigosa
 * @date 2024-11-11
 */

#pragma once

#include "scheduler/scheduler.h"
#include "slate.h"

void write_sd_card_task_init(slate_t *slate);
void write_sd_card_task_dispatch(slate_t *slate);

sched_task_t write_sd_card_task;