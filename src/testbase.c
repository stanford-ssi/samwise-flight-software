/**
 * @author  Darrow Hartman
 * @date    2024-10-28
 *
 * This file contains the main testing entry point for the SAMWISE flight code.
 * All testing code should import this file
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "init.h"
#include "macros.h"
#include "scheduler/scheduler.h"
#include "slate.h"


/**
 * Static allocation of the slate.
 */
slate_t test_slate;

int test_main()
{
    stdio_init_all();
    LOG_TEST("testbase: Slate uses %d bytes", sizeof(test_slate));
    LOG_TEST("testbase: Initializing everything...");
    ASSERT(init(&test_slate));
    LOG_TEST("testbase: We are in test mode!");
    
    return 0;
}