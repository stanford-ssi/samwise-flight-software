#pragma once
#include "macros.h"

#ifdef BRINGUP

#include "scheduler/scheduler.h"
#include "slate.h"

void i2c_scan_task_init(slate_t *slate);
void i2c_scan_task_dispatch(slate_t *slate);

sched_task_t i2c_scan_task;

#endif
