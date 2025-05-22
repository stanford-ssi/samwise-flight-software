/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 */

#pragma once

#include "pico/stdlib.h"

#ifndef PICO
// Neopixel only available on PICUBED boards
#include "neopixel.h"
#endif

void fatal_error();