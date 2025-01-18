#pragma once
#include "macros.h"

#ifdef BRINGUP

#include "scheduler/scheduler.h"
#include "slate.h"

void diagnostics_task_init(slate_t *slate);
void diagnostics_task_dispatch(slate_t *slate);

sched_task_t diagnostics_task;

#endif
