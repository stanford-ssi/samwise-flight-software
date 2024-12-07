#pragma once
#include "macros.h"

#ifdef BRINGUP

#include "scheduler/scheduler.h"
#include "slate.h"

void scan_task_init(slate_t *slate);
void scan_task_dispatch(slate_t *slate);

sched_task_t scan_task;

#endif
