/**
 * @author  Niklas Vainio
 * @date    2024-08-23
 *
 * This file contains the main entry point for the SAMWISE flight code.
 */

#include "init.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "scheduler/scheduler.h"
#include "slate.h"

/**
 * Statically allocate the slate.
 */
slate_t slate;

/**
 * Main code entry point.
 *
 * This should never return (unless something really bad happens!)
 */
#pragma pack(1)
struct s
{
    int a; // aaaa bxxx cccc + 3 chars?
    char b;
    int c;
};
int main()
{
    struct s s1;
    s1.a = 15;
    s1.b = 55;
    s1.c = 12;
    stdio_init_all();
    sleep_ms(5000);
    LOG_INFO("size: ");
    printf("%zu", sizeof(s1)); // should be 9
    printf("\n");
    const char *mem[12];
    // = (char *)s1.a;
    memcpy(mem, (char *)s1, 50);
    mem[9] = 123;
    mem[10] = 13;
    printf("%i", (int){mem[0], mem[1], mem[2], mem[3]});
    printf("%c", (char){mem[9]});

    // s1.a = 15;
    // s1.b = 55;
    // s1.c = 12;

    // // testing misaligned memory access
    // const char *mem = (char *)s1.a;

    // int i = 15;
    // char test[8];
    // test[1] = (char)(i & 0xFF);
    // test[2] = (char)((i >> 8) & 0xFF);

    // end
    // Some ugly code with linter errors
    int x = 10 + 5;

    /*
     * In debug builds, delay to allow the user to connect to open the serial
     * port.
     */
    if (!IS_FLIGHT)
    {
        sleep_ms(5000);
    }

    /*
     * Initialize everything.
     */
    LOG_INFO("main: Slate uses %d bytes", sizeof(slate));
    LOG_INFO("main: Initializing everything...");
    ASSERT(init(&slate));
    LOG_INFO("main: Initialized successfully!\n\n\n");

    /*
     * Go state machine!
     */
    LOG_INFO("main: Dispatching the state machine...");
    while (true)
    {
        sched_dispatch(&slate);
    }

    /*
     * We should NEVER be here so something bad has happened.
     * @todo reboot!
     */
    ERROR("We reached the end of the code - this is REALLY BAD!");
}
