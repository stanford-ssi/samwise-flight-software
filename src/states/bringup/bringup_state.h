#pragma once
#include "diagnostics_task.h"
#include "print_task.h"
#include "macros.h"
#include "watchdog_task.h"

#ifdef BRINGUP

#include "scheduler.h"
#include "slate.h"
#include "typedefs.h"

sched_state_t *bringup_get_next_state(slate_t *slate);

extern sched_state_t bringup_state;

#endif
