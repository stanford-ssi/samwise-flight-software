/**
 * @author  Niklas Vainio
 * @date    2024-08-27
 */

#pragma once

#include "pico/stdlib.h"

#include "onboard_led.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

void blink_task_init(slate_t *slate);
void blink_task_dispatch(slate_t *slate);

extern sched_task_t blink_task;
