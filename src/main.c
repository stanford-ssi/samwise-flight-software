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

/**
 * Statically allocate the slate.
 */
slate_t slate;

int send();

void interrupt_recieved(uint gpio, uint32_t events);

int receive();

int check_version();

const uint RADIO_INTERRUPT_PIN = 28;

rfm9x_t radio_module;
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

    gpio_init(RADIO_INTERRUPT_PIN);
    gpio_set_dir(RADIO_INTERRUPT_PIN, GPIO_IN);
    gpio_pull_down(RADIO_INTERRUPT_PIN);

    // Set interrupt handler for the radio
    gpio_set_irq_enabled_with_callback(RADIO_INTERRUPT_PIN, GPIO_IRQ_EDGE_RISE,
                                       true, &interrupt_recieved);

    bool interruptPin = gpio_get(RADIO_INTERRUPT_PIN);
    printf("Interrupt pin (before): %d\n", interruptPin);

    /*
    while (true)
    {
        sleep_ms(100);
        sched_dispatch(&slate);
    }
    */
    // send();
    // receive();
    uint reset = 21;
    //uint reset = 15;
    uint cs = 20;
    //uint cs = 17;
    uint tx = 19;
    uint rx = 16;
    uint clk = 18;
    radio_module = rfm9x_mk(spi0, reset, cs, tx, rx, clk);

    radio_module.debug = 1;

    rfm9x_init(&radio_module);

    printf("Version: %d\r\n", rfm9x_version(&radio_module));

    while(1) {
        // send(radio_module);
        receive(radio_module);
        sleep_ms(1000);
    }

    // rfm9x_init(&radio_module);
    // check_version();
    /*
     * We should NEVER be here so something bad has happened.
     * @todo reboot!
     */

    ERROR("We reached the end of the code - this is REALLY BAD!");
}

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
