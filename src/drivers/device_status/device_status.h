#pragma once
#include "hardware/gpio.h"
#include "pins.h"
#include <stdbool.h>
#include <stdio.h>

void device_status_init();
bool is_fixed_solar_charging();
bool is_fixed_solar_faulty();
bool is_flex_panel_A_deployed();
bool is_flex_panel_B_deployed();
