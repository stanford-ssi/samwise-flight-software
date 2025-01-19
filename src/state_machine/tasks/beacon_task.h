/**
 * @author  Yao Yiheng
 * @date    2025-01-18
 */

#pragma once

#include "macros.h"
#include "pico/stdlib.h"
#include "scheduler/scheduler.h"
#include "slate.h"
#include "state_machine/tasks/radio_packet.h"

void beacon_task_init(slate_t *slate);
void beacon_task_dispatch(slate_t *slate);

sched_task_t beacon_task;
