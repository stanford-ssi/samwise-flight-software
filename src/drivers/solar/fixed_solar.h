#pragma once
#include <stdbool.h>
#include <stdio.h>
#include "pins.h"
#include "hardware/gpio.h"

// BCM 13 (physical header 33?)
#define GPIO13_SHIFT 9        

void init_fixed_solar_charge();

bool read_fixed_solar_charge();

void init_fixed_solar_fault();

bool read_fixed_solar_fault();

