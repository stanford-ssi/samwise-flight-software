#pragma once

#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

#include "blink_task.h"
#include "command_task.h"
#include "print_task.h"
#include "radio_task.h"
#include "diagnostics_task.h"

sched_state_t *running_get_next_state(slate_t *slate);

extern sched_state_t running_state;
