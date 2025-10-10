#pragma once

#include "macros.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

// Forward declarations
extern sched_state_t running_state;
extern sched_state_t bringup_state;

sched_state_t *init_get_next_state(slate_t *slate);

extern sched_state_t init_state;
