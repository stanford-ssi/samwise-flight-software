#pragma once
#include "hardware/gpio.h"
#include "pins.h"
#include <stdbool.h>
#include <stdio.h>

void init_device_status_drivers();
bool is_fixed_solar_charging();
bool is_fixed_solar_faulty();

