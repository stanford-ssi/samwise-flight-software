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

#include "state_machine/tasks/command_switch_task.h"

/**
 * Statically allocate the slate.
 */
slate_t slate;

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
    // while (true)
    // {
    //     sched_dispatch(&slate);
    // }
    

    /* TESTS */
    struct test_t1DS{
        int data_int_1;
        uint8_t data_byteArr_1[16];
    };
    struct test_t1DS t1ds;
    struct test_t1DS SEC_t1ds;
    struct test_t1DS RECONt1ds;

    struct test_t1DS FULLOUT;
    struct test_t1DS FULLOUT2;

    t1ds.data_int_1 = 229;
    t1ds.data_byteArr_1[0] = 123;
    t1ds.data_byteArr_1[14] = 249;
    SEC_t1ds.data_int_1 = 303;
    SEC_t1ds.data_byteArr_1[0] = 111;
    SEC_t1ds.data_byteArr_1[9] = 222;

    uint8_t data[251];
    uint8_t data2[251];
    uint8_t dataout[251];
    uint8_t data2out[251];
    uint8_t stopByte = 255;

    data[0] = 1;
    memcpy(&data[1], &t1ds, sizeof(t1ds));  // +1 for stop byte
    // memcpy(&data[1] + sizeof(t1ds) + 1, &stopByte ,1);   // uncomment for separete queues
    // check if reconstruction works?
    memcpy(&RECONt1ds, &data[1], sizeof(RECONt1ds)); //% PAYLOAD_SIZE);
    LOG_DEBUG("Reconstruction tested int: %i and byte at 14th: %i", RECONt1ds.data_int_1, RECONt1ds.data_byteArr_1[14]);




    LOG_INFO("Successfully copied ds to byte arr");
    command_switch_task_init(&slate);  // init queue
    LOG_INFO("task init done");
    sleep_ms(1000);
    // queue_try_add(&slate.radio_packets_out, &data[0]); // add to radio queue
    for(int i = 0; i < sizeof(t1ds); i++){
                        LOG_DEBUG("input data1: %i", data[i]);
                    }

    data2[0] = 1;
    memcpy(&data2[1], &SEC_t1ds, sizeof(SEC_t1ds)); 
    memcpy(&data2[1]+ sizeof(SEC_t1ds) + 1, &stopByte, 1);

    memcpy(&data[1] + sizeof(t1ds) + 1, &data2, sizeof(SEC_t1ds)+1);
    queue_try_add(&slate.radio_packets_out, &data[0]); // add to radio queue

for(int i = 0; i < sizeof(SEC_t1ds); i++){
                        LOG_DEBUG("input data2: %i", data2[i]);
                    }
    // queue_try_add(&slate.radio_packets_out, &data2[0]); 
    LOG_INFO("radio payload added");
    sleep_ms(1000);
    command_switch_dispatch(&slate);    // dequeue radio packet and add to task queue
    LOG_INFO("dispatching done 1");
    // command_switch_dispatch(&slate);    // dequeue radio packet and add to task queue
    // LOG_INFO("dispatching done 2");
    sleep_ms(1000);
    queue_try_remove(&slate.task1_data,&dataout);
    memcpy(&FULLOUT, &dataout, sizeof(FULLOUT)); //% PAYLOAD_SIZE);
    LOG_DEBUG("Integer from 1st data: %i and byte at 14 pos: %i", FULLOUT.data_int_1, FULLOUT.data_byteArr_1[14]);

    LOG_INFO("dequeued task data successfully");
    sleep_ms(1000);


    for (int i = 0; i < sizeof(t1ds); i++){
        if (data[i+1] == dataout[i])
        {
           LOG_INFO("Correct coppying at %i", i);
            // correctly copied up until i
        }else{
            LOG_INFO("Incorrect coppying at %i", i);
            // incorectly copied at i 
            //break;
        }
        LOG_INFO("data: %hhx", data[i+1]);
        LOG_INFO("dataout: %hhx", dataout[i]);

    
    }
    queue_try_remove(&slate.task1_data,&data2out);
    memcpy(&FULLOUT2, &data2out, sizeof(FULLOUT2)); //% PAYLOAD_SIZE);

    LOG_INFO("dequeued task data2 successfully");
    LOG_DEBUG("Integer (303, 111) from 2nd data: %i and byte at 0 pos: %i", FULLOUT2.data_int_1, FULLOUT2.data_byteArr_1[0]);


    /* End of TESTS */

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
