#pragma once

#include "error.h"
#include "logger.h"

#ifdef BRINGUP
#define IS_BRINGUP true
#else
#define IS_BRINGUP false
#endif

#ifdef FLIGHT
#define IS_FLIGHT true
#else
#define IS_FLIGHT false
#endif

#ifdef PICO
#define IS_PICO true
#else
#define IS_PICO false
#endif

#ifdef TEST
#define IS_TEST true
#else
#define IS_TEST false
#endif
