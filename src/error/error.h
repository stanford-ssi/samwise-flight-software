/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 */

#pragma once

#ifdef TEST_MODE
// In test mode, we don't need hardware dependencies
#else
#include "neopixel.h"
#include "pico/stdlib.h"
#endif

void fatal_error(char *msg);