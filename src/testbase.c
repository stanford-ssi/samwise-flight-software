/**
 * @author  Darrow Hartman
 * @date    2024-10-28
 *
 * This file contains the main testing entry point for the SAMWISE flight code.
 * All testing code should import this file
 */

#include "testbase.h"

// // Helps set up environment for exeuting task
// int test_main(sched_task_t *task)
// {
//     // Initis
//     stdio_init_all();
//     LOG_TEST("testbase: Slate uses %d bytes", sizeof(test_slate));
//     LOG_TEST("testbase: Initializing everything...");
//     // Initialize test slate
//     ASSERT(init(&test_slate));
//     LOG_TEST("testbase: We are in test mode!");

//     // loop through /tasks folder and run all tests
//     task->task_dispatch(&test_slate);
    
//     return 0;
// }
// postpone 
// int test_all_cases(){
    // // go into filesystem, look for _test.c files, and run them
    // for file in files:
    //     if _test in file.name:
    //         _test.test_cases();

    //         for func in extract_functions(file):
    //             type(func) == String
    //             // (compile code)
    //             // (String --> code pointer in compiled code) 
    //             // eval(function_name)
    // // needs to be written and the .c files included and compiled as part of testbase
    // function_to_test();
// }