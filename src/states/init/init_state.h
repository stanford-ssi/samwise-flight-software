#pragma once

#include "macros.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

#include "burn_wire_state.h"
#include "running_state.h"

sched_state_t *init_get_next_state(slate_t *slate);

extern sched_state_t init_state;
