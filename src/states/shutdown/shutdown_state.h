#pragma once

#include "state_machine.h"
#include "typedefs.h"
#include "watchdog_task.h"

state_id_t shutdown_get_next_state(slate_t *slate);

extern sched_state_t shutdown_state;
