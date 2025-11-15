/**
 * @author  Joziah Uribe-Lopez
 * @date    2025-11-01
 *
 * Writing, then power off and power on, then reading to verify data was saved.
 */
#pragma once

#include "command_parser.h"
#include "macros.h"
#include "scheduler.h"
#include "slate.h"
// #include "mram.h"

#define INTERRUPT_TASK_COLOR 255, 215, 0  // Gold Color

void interrupt_task_init(slate_t *slate);
void interrupt_task_dispatch(slate_t *slate);

extern sched_task_t interrupt_task;