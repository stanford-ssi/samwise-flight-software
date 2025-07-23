/**
 * @author  Marc Aaron Reyes
 * @date    2025-07-21
 */

#pragma once

#include "command_parser.h"
#include "file_transfer.h"
#include "scheduler.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

void file_transfer_task_init(slate_t *slate);
void file_transfer_task_dispatch(slate_t *slate);

extern sched_task_t file_transfer_task;
