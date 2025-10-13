#pragma once

#include "macros.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

// Forward declaration
extern sched_state_t running_state;

sched_state_t *burn_wire_reset_get_next_state(slate_t *slate);

extern sched_state_t burn_wire_reset_state;
