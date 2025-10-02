#pragma once

#include "adm1176.h"
#include "common/config.h"
#include "device_status.h"
#include "hardware/i2c.h"
#include "logger.h"
#include "macros.h"
#include "mppt.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/unique_id.h"
#include "pins.h"
#include "rfm9x.h"
#include "slate.h"
#include "state_machine.h"

// LED Color for telemetry task - Blue
#define TELEMETRY_TASK_COLOR 0, 0, 255

extern sched_task_t telemetry_task;
