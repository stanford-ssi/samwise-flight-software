#pragma once
#include "diagnostics_task.h"
#include "hardware_test_task.h"
#include "macros.h"
#include "print_task.h"
#include "watchdog_task.h"

#ifdef BRINGUP

#include "state_machine.h"
#include "typedefs.h"

state_id_t bringup_get_next_state(slate_t *slate);

extern sched_state_t bringup_state;

#endif
