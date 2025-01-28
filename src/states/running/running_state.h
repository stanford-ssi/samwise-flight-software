#pragma once

// The following ordering needs to be preserved
#include "typedefs.h"
#include "slate.h"
#include "state_machine.h"

#include "print_task.h"
#include "blink_task.h"
#include "radio_task.h"

sched_state_t *running_get_next_state(slate_t *slate);

extern sched_state_t running_state;
