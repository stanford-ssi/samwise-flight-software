#pragma once

#include "adm1176.h"
#include "hardware/i2c.h"
#include "logger.h"
#include "macros.h"
#include "mppt.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "pins.h"
#include "rfm9x.h"
#include "slate.h"
#include "state_machine.h"

extern sched_task_t telemetry_task;
