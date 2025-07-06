/**
 * @author  Joseph Shetaye
 * @date    2024-11-18
 */

#pragma once

#include <string.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"

#include "macros.h"
#include "pins.h"
#include "slate.h"
#include "state_machine.h"
#include "typedefs.h"

#include "packet.h"
#include "rfm9x.h"

// LED Color for radio task - Magenta
#define RADIO_TASK_COLOR 255, 0, 255

#define TX_QUEUE_SIZE 16
#define RX_QUEUE_SIZE 16

void radio_task_init(slate_t *slate);
void radio_task_dispatch(slate_t *slate);

extern sched_task_t radio_task;
