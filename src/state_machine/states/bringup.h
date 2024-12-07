#pragma once
#include "macros.h"
#ifdef BRINGUP

#include "scheduler/scheduler.h"
#include "slate.h"

sched_state_t *bringup_get_next_state(slate_t *slate);

#endif
