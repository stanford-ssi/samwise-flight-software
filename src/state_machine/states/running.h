#pragma once

#include "slate.h"
#include "scheduler/scheduler.h"

sm_state_t running_get_next_state(slate_t *slate);

sched_state_info_t running_state_info;
