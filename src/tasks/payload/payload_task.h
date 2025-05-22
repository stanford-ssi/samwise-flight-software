/**
 * @author  Marc Aaron Reyes
 * @date    2025-05-03
 */

#pragma once

#include "payload_uart.h"
#include "payload_command.h"
#include "macros.h"
#include "scheduler.h"
#include "slate.h"

void payload_task_init(slate_t *slate);
void payload_task_dispatch(slate_t *slate);

extern sched_task_t payload_task;
