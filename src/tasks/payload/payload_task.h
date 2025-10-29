/**
 * @author  Marc Aaron Reyes
 * @date    2025-05-03
 */

#pragma once

#include "command_parser.h"
#include "macros.h"
#include "payload_uart.h"
#include "scheduler.h"
#include "slate.h"

// LED Color for payload task - Purple
#define PAYLOAD_TASK_COLOR 128, 0, 128

#define MAX_PAYLOAD_COMMANDS_PER_DISPATCH 3
#define MAX_PAYLOAD_RETRY_COUNT 3

void payload_task_init(slate_t *slate);
void payload_task_dispatch(slate_t *slate);
void heartbeat_check(slate_t *slate);

static uint8_t RETRY_COUNT = 0;
extern sched_task_t payload_task;
