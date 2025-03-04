/**
 * @author  Niklas Vainio
 * @date    2024-08-27
 */

#pragma once

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "macros.h"
#include "pins.h"
#include "scheduler/scheduler.h"


bool init(slate_t *slate);