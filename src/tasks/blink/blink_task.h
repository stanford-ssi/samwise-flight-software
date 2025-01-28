/**
 * @author  Niklas Vainio
 * @date    2024-08-27
 */

#pragma once

#include "pico/stdlib.h"

// The following ordering needs to be preserved
#include "typedefs.h"
#include "slate.h"
#include "state_machine.h"

void blink_task_init(slate_t *slate);
void blink_task_dispatch(slate_t *slate);

extern sched_task_t blink_task;
