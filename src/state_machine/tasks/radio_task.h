/**
 * @author  Joseph Shetaye
 * @date    2024-11-18
 */

#pragma once

#include "scheduler/scheduler.h"
#include "slate.h"
#include "state_machine/tasks/radio_packet.h"

#define TX_QUEUE_SIZE 16
#define RX_QUEUE_SIZE 16

void radio_task_init(slate_t *slate);
void radio_task_dispatch(slate_t *slate);

sched_task_t radio_task;
