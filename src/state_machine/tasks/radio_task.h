/**
 * @author  Joseph Shetaye
 * @date    2024-11-18
 */

#pragma once

#include "scheduler/scheduler.h"
#include "slate.h"

#define TX_QUEUE_SIZE 16
#define RX_QUEUE_SIZE 16

typedef struct
{
    uint8_t src;
    uint8_t dst;
    uint8_t flags;
    uint8_t seq;
    uint8_t len;
    uint8_t data[252];
} packet_t;

void radio_task_init(slate_t *slate);
void radio_task_dispatch(slate_t *slate);

sched_task_t radio_task;
