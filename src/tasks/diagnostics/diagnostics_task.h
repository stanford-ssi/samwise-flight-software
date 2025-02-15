#pragma once

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "pins.h"
#include "rfm9x.h"
#include "slate.h"
#include "state_machine.h"
#include "macros.h"
#include "logger.h"

extern sched_task_t diagnostics_task;
