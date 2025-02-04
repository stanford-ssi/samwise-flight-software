/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 */

#pragma once

#if(TEST == 1)
#include "mock_pico.h"
#else
#include "pico/stdlib.h"
#endif

void fatal_error();