#pragma once

#include "macros.h"
#include "ota_task.h"
#include "radio_task.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"
#include "watchdog_task.h"

state_id_t ota_get_next_state(slate_t *slate);

extern sched_state_t ota_state;
