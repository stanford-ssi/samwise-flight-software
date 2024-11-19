/**
 * @author  Darrow Hartman
 * @date    2024-11-19
 *
 * Test base for the satellite.
 */

#include "init.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "scheduler/scheduler.h"
#include "slate.h"

// Test initialization and cleanup functions
static int suite_init(void);
static int suite_cleanup(void);

// Main test entry point
int testbase(void);

