/**
 * @author Ayush Garg
 * @date  2025-10-29
 */
#pragma once

#include "command_parser.h"
#include "macros.h"
#include "payload_uart.h"
#include "scheduler.h"
#include "slate.h"

// LED Color for ftp task - Hot Pink
#define FTP_TASK_COLOR 255, 105, 180

#define MAX_PAYLOAD_COMMANDS_PER_DISPATCH 3
#define MAX_PAYLOAD_RETRY_COUNT 3

void payload_task_init(slate_t *slate);
void payload_task_dispatch(slate_t *slate);

extern sched_task_t ftp_task;
