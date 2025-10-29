/**
 * @author  Yao Yiheng
 * @date    2025-01-18
 */

#pragma once

#include <string.h>

#include "pico/util/queue.h"

#include "macros.h"
#include "packet.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

// LED Color for beacon task - Yellow
#define BEACON_TASK_COLOR 255, 255, 0

size_t serialize_slate(slate_t *slate, uint8_t *data, uint8_t tx_power);
void beacon_task_init(slate_t *slate);
void beacon_task_dispatch(slate_t *slate);

extern sched_task_t beacon_task;
