#pragma once
#include "hardware/gpio.h"
#include "pins.h"
#include <stdbool.h>
#include <stdio.h>

void init_fixed_solar_charge();

bool read_fixed_solar_charge();

void init_fixed_solar_fault();

bool read_fixed_solar_fault();
