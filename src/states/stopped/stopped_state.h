#pragma once

#include "macros.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"


extern sched_state_t stop_state;

sched_state_t *stop_get_next_state(slate_t *slate);
