/**
 * @author  Niklas Vainio
 * @date    2024-08-23
 *
 * This file contains the main entry point for the SAMWISE flight code.
 */

#include "drivers/rfm9x.h"
#include "init.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "scheduler/scheduler.h"
#include "slate.h"

typedef struct
{
    uint8_t src;
    uint8_t dst;
    uint8_t flags;
    uint8_t seq;
    uint8_t len;
    uint8_t data[252];
} packet_t;

/**
 * Statically allocate the slate.
 */
slate_t slate;

struct TASK2_DATA_STRUCT_FORMAT
{
    bool yes_no;
    uint16_t number;
};


/**
 * Main code entry point.
 *
 * This should never return (unless something really bad happens!)
 */
int main()
{   
    // Some ugly code with linter errors
    int x = 10 + 5;
    stdio_init_all();
    
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

    struct TASK2_DATA_STRUCT_FORMAT struct2;
    struct2.yes_no = true;
    struct2.number = 15;

    packet_t p;

    p.len = sizeof(struct2) + 1;
    p.dst = 255;
    p.src = 0;
    p.seq;
    p.flags;
    p.data[0] = 2;
    memcpy(p.data + 1, &struct2, sizeof(struct2));

    
    //queue_try_add(&slate.tx_queue, &p);

    while (true)
    {
        
        sleep_ms(100);
        sched_dispatch(&slate);
    }

    ERROR("We reached the end of the code - this is REALLY BAD!");
}
