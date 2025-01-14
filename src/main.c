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

#include "state_machine/tasks/command_switch_task.h"

/**
 * Statically allocate the slate.
 */
slate_t slate;

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
    // while (true)
    // {
    //     sched_dispatch(&slate);
    // }
    

    /* TESTS 2 function calls in one packet */
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

    data[0] = 1;        //simulate function code 1
    memcpy(&data[1], &t1ds, sizeof(t1ds));  // +1 for stop byte
    // memcpy(&data[1] + sizeof(t1ds) + 1, &stopByte ,1);   // uncomment for separete queues
    // check if reconstruction works?
    memcpy(&RECONt1ds, &data[1], sizeof(RECONt1ds)); //% PAYLOAD_SIZE);
    LOG_DEBUG("Reconstruction tested int: %i and byte at 14th: %i", RECONt1ds.data_int_1, RECONt1ds.data_byteArr_1[14]);




    // LOG_INFO("Successfully copied ds to byte arr");
    command_switch_task_init(&slate);  // init queue
    LOG_INFO("task init done");
    sleep_ms(1000);
    // queue_try_add(&slate.radio_packets_out, &data[0]); // add to radio queue
    // for(int i = 0; i < sizeof(t1ds); i++){
    //                     LOG_DEBUG("input data1: %i", data[i]);
    //                 }

    data2[0] = 1;
    memcpy(&data2[1], &SEC_t1ds, sizeof(SEC_t1ds)); 
    memcpy(&data2[1]+ sizeof(SEC_t1ds) + 1, &stopByte, 1);  

    memcpy(&data[1] + sizeof(t1ds) + 1, &data2, sizeof(SEC_t1ds)+1);

    LOG_DEBUG("whole packet data");
    for(int i = 0; i < sizeof(SEC_t1ds)*2 + 1; i++){
                            LOG_DEBUG("packet data at %i : %i", i, data2[i]);
    }

    queue_try_add(&slate.radio_packets_out, &data[0]); // add to radio queue



    // for(int i = 0; i < sizeof(SEC_t1ds); i++){
    //                         LOG_DEBUG("input data2: %i", data2[i]);
    //                     }
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

    LOG_DEBUG("first packet data");
    for(int i = 0; i < sizeof(FULLOUT); i++){
                            LOG_DEBUG("packet 1 data at %i : %i", i, dataout[i]);
    }
    LOG_DEBUG("Integer from packet 1 (229): %i and byte at 14 pos (249): %i", FULLOUT.data_int_1, FULLOUT.data_byteArr_1[14]);

    queue_try_remove(&slate.task1_data,&dataout);
    memcpy(&FULLOUT, &dataout, sizeof(FULLOUT)); //% PAYLOAD_SIZE);

    LOG_DEBUG("second packet data");
    for(int i = 0; i < sizeof(FULLOUT); i++){
                            LOG_DEBUG("packet 2 data at %i : %i", i, dataout[i]);
    }
    
    LOG_DEBUG("Integer from packet 2 (303): %i and byte at 9 pos (222): %i", FULLOUT.data_int_1, FULLOUT.data_byteArr_1[9]);

    
    sleep_ms(25000);




    // memcpy(&data_t2[1], &long_ds_d, sizeof(long_ds_d));

    
    // LOG_DEBUG("Integer from 1st data: %i and byte at 14 pos: %i", FULLOUT.data_int_1, FULLOUT.data_byteArr_1[14]);

    LOG_INFO("dequeued task data successfully");
    sleep_ms(1000);


    // for (int i = 0; i < sizeof(t1ds); i++){
    //     if (data[i+1] == dataout[i])
    //     {
    //        LOG_INFO("Correct coppying at %i", i);
    //         // correctly copied up until i
    //     }else{
    //         LOG_INFO("Incorrect coppying at %i", i);
    //         // incorectly copied at i 
    //         //break;
    //     }
    //     LOG_INFO("data: %hhx", data[i+1]);
    //     LOG_INFO("dataout: %hhx", dataout[i]);

    
    // }
    // queue_try_remove(&slate.task1_data,&data2out);
    // memcpy(&FULLOUT2, &data2out, sizeof(FULLOUT2)); //% PAYLOAD_SIZE);

    // LOG_INFO("dequeued task data2 successfully");
    // LOG_DEBUG("Integer (303, 111) from 2nd data: %i and byte at 0 pos: %i", FULLOUT2.data_int_1, FULLOUT2.data_byteArr_1[0]);



    /* TESTS 2. one long function call taking more than 1 packet 
    (potentially useless if no long data intended to be sent, a simple check on the server
     size to make sure data isnt cut in half would suffice) */





    struct long_ds{
        uint16_t data_int_1;
        uint8_t data_byteArr_1[260];
    };

    struct long_ds long_ds_d;
    long_ds_d.data_int_1 = 999;
    long_ds_d.data_byteArr_1[257] = 44;
    
    struct long_ds long_ds_rec;  // future reconstructed
    uint8_t long_out[sizeof(long_ds_rec)];

    uint8_t data_t2p1[251];   // first packet  (this is just for testing, later on it will be automaticly split)
    data_t2p1[0] = 1;      
    uint8_t data_t2p2[251];  // second packet


    int count = 0;


    memcpy(&data_t2p1[1], &long_ds_d, 250);
    memcpy(&data_t2p2[1], &long_ds_d, sizeof(long_ds_d) - 250);   // we know out ds will be only 2 packets here, again testing




    // print both packets to see the whole thing # todo
    LOG_DEBUG("whole packet data");
    for(int i = 0; i < sizeof(SEC_t1ds)*2 + 1; i++){
                            LOG_DEBUG("packet data at %i : %i", i, data2[i]);
    }

    // add both to radio queue #todo
    queue_try_add(&slate.radio_packets_out, &data[0]); // add to radio queue




    LOG_INFO("radio payload added test2");
    sleep_ms(1000);
    command_switch_dispatch(&slate);    // dequeue radio packet and add to task queue (should dequeu both packets?)
    LOG_INFO("dispatching done 1");
    sleep_ms(1000);
    queue_try_remove(&slate.task1_data,&long_out);  // should be read out in 2 packets maybe?
    memcpy(&long_ds_rec, &long_out, sizeof(long_ds_rec)); //% PAYLOAD_SIZE);

    LOG_DEBUG("test 2 packet data");
    for(int i = 0; i < sizeof(long_ds_rec); i++){
                            LOG_DEBUG("packet 1 data at %i : %i", i, long_out[i]);
    }
    
    LOG_DEBUG("Integer from packet 2 (999): %i and byte at 257 pos (44): %i", long_ds_rec.data_int_1, long_ds_rec.data_byteArr_1[14]);

    
    
    // LOG_DEBUG("Integer from 1st data: %i and byte at 14 pos: %i", FULLOUT.data_int_1, FULLOUT.data_byteArr_1[14]);

    LOG_INFO("dequeued task data successfully");
    sleep_ms(1000);
    









    /* End of TESTS */

    /*
     * We should NEVER be here so something bad has happened.
     * @todo reboot!
     */
    ERROR("We reached the end of the code - this is REALLY BAD!");
}
