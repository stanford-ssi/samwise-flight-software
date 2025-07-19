#pragma once
#include <stdbool.h>
#include <stdio.h>
#include "pins.h"
#include "hardware/gpio.h"

void init_panel_A();

bool read_panel_A();

void init_panel_B();

bool read_panel_B();