#pragma once

#include "state_machine.h"
#include "typedefs.h"

#include "adcs_task.h"
#include "beacon_task.h"
#include "blink_task.h"
#include "command_task.h"
#include "diagnostics_task.h"
#if defined(BRINGUP) || defined(PICO)
#include "hardware_test_task.h"
#endif
#include "payload_task.h"
#include "print_task.h"
#include "radio_task.h"
#include "telemetry_task.h"
#include "watchdog_task.h"

state_id_t running_get_next_state(slate_t *slate);

extern sched_state_t running_state;
