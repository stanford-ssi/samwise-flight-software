/**
 * @author Niklas Vainio
 * @date 2025-05-24
 *
 * ADCS task for high-level ADCS control and command logic
 */
#pragma once

#include "adcs_packet.h"
#include "slate.h"

#define ADCS_TASK_COLOR 128, 255, 0

void adcs_task_init(slate_t *slate);

void adcs_task_dispatch(slate_t *slate);
