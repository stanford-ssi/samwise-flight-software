/**
 * @author  Niklas Vainio
 * @date    2024-08-23
 *
 * This file contains the main entry point for the SAMWISE flight code.
 */

#include "init.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "rfm9x.h"
#include "scheduler.h"
#include "slate.h"
#include "drivers/rfm9x.h"

/**
 * Statically allocate the slate.
 */
slate_t slate;

int send();

int receive();

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
     * Print commit hash
     */
#ifdef COMMIT_HASH
    LOG_INFO("main: Running samwise-flight-software %s", COMMIT_HASH);
#endif

    /*
     * Go state machine!
     */
    LOG_INFO("main: Dispatching the state machine...");

    /*
    while (true)
    {
        sleep_ms(100);
        sched_dispatch(&slate);
    }
    */
   send();
   receive();

    /*
     * We should NEVER be here so something bad has happened.
     * @todo reboot!
     */

    ERROR("We reached the end of the code - this is REALLY BAD!");
}

int send(){

    // MAKE SURE THE RESET UINT IS ACTUALLY GOOD
    rfm9x_t radio_module = rfm9x_mk(16);

    // Initialize the radio module
    // The & returns the address of the radio module, so the funciton receives a "pointer" to the object
    rfm9x_init(&radio_module);

    // Send a transmission

    char data[4];

    data[0] = 'm';
    data[1] = 'e';
    data[2] = 'o';
    data[3] = 'w';

    rfm9x_send(&radio_module, &data[0], 4, 1,1,1,1,1);

    printf("sending:");
    printf(data);
}

int receive(){

    // MAKE SURE THE RESET UINT IS ACTUALLY GOOD
    rfm9x_t radio_module = rfm9x_mk(16);

    // Initialize the radio module
    // The & returns the address of the radio module, so the funciton receives a "pointer" to the object
    rfm9x_init(&radio_module);

    // Receive a transmission

    char data[4];
    rfm9x_receive(&radio_module, &data[0], 4, 1,1);

    printf(data);
}

/*

uint8_t rfm9x_send(rfm9x_t *r, char *data, uint32_t l, uint8_t keep_listening,
                   uint8_t destination, uint8_t node, uint8_t identifier,
                   uint8_t flags);

uint8_t rfm9x_send_ack(rfm9x_t *r, char *data, uint32_t l, uint8_t destination,
                       uint8_t node, uint8_t max_retries);

uint8_t rfm9x_receive(rfm9x_t *r, char *packet, uint8_t node,
                      uint8_t keep_listening, uint8_t with_ack);

*/