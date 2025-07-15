/**
 * @author Niklas Vainio
 * @date 2025-05-24
 *
 * This file contains functions for receiving telemetry over UART from the ADCS
 * board
 */
#pragma once

#include "adcs_packet.h"
#include "slate.h"

void adcs_task_init(slate_t *slate);

void adcs_task_dispatch(slate_t *slate);
