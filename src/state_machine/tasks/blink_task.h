/**
 * @author  Niklas Vainio
 * @date    2024-08-27
 */

#pragma once

#include "slate.h"
#include "scheduler/scheduler.h"

void blink_task_init(slate_t *slate);
void blink_task_dispatch(slate_t *slate);

sched_task_t blink_task;