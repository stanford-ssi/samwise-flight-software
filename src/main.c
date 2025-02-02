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
#include "rfm9x.h"
#include "scheduler.h"
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
     * Print commit hash
     */
#ifdef COMMIT_HASH
    LOG_INFO("main: Running samwise-flight-software %s", COMMIT_HASH);
#endif

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
    //queue_try_add(&slate.tx_queue, &p);

    while (true)
    {
        
        sleep_ms(100);
        sched_dispatch(&slate);
    }

    /*
     * We should NEVER be here so something bad has happened.
     * @todo reboot!
     */

    ERROR("We reached the end of the code - this is REALLY BAD!");
}

/*
int check_version(rfm9x_t radio_module)
{
    LOG_INFO("%d\n", rfm9x_version(&radio_module));
}

int send(rfm9x_t radio_module)
{
    char data[4];

    data[0] = 'm';
    data[1] = 'e';
    data[2] = 'o';
    data[3] = 'w';

    rfm9x_send(&radio_module, &data[0], 4, 0, 255, 0, 0, 0);
}

void interrupt_recieved(uint gpio, uint32_t events)
{
    printf("Interrupt received on pin %d\n", gpio);
    if (gpio == RADIO_INTERRUPT_PIN)
    {
        printf("Radio interrupt received\n");
        receive(radio_module);
    }
}

// screen /dev/tty.usbmodem1101
int receive(rfm9x_t radio_module)
{
    char data[256];
    uint8_t n = rfm9x_receive(&radio_module, &data[0], 1, 0, 0, 1);
    printf("Received %d\n", n);

    bool interruptPin = gpio_get(RADIO_INTERRUPT_PIN);
    printf("Interrupt pin: %d\n", interruptPin);
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
