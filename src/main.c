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

queue_t mission_control_command_queue;
uint16_t first_open_byte_index = 0;

/* TESTS 2 function calls in one packet */
struct test_t1DS{
    int data_int_1;
    uint8_t data_byteArr_1[300];
};

void init_mission_control_command_queue(uint16_t payload_size){
    queue_init(&mission_control_command_queue, payload_size, 20);
}
void add_packet_to_send_queue(uint8_t* buffer){
    queue_try_add(&mission_control_command_queue, buffer);
}

void send_queue_to_satellite(slate_t* slate){
    uint8_t* payload;
    while(queue_try_remove(&mission_control_command_queue, payload)){
        // Then put it into the radio queue
        queue_try_add(&slate->radio_packets_out, payload);
    }

    first_open_byte_index = 0;

}


void add_commands_to_send_queue(slate_t* slate, uint8_t* buffer_location, uint8_t function_number, uint8_t* struct_buffer, uint16_t payload_size, uint16_t struct_buffer_size, bool add_stop_after){
    uint16_t struct_bytes_saved = 0;

    buffer_location[first_open_byte_index] = function_number;
    first_open_byte_index += 1;

    // While we have not saved all of the bytes
    while(struct_bytes_saved < struct_buffer_size){
        LOG_INFO("Number of saved bytes so far: %i", struct_bytes_saved);

        // predict where the last byte will be located
        uint16_t predicted_end_location = first_open_byte_index + (struct_buffer_size - struct_bytes_saved);

        // If it will end after the payload, we need another packet
        if(predicted_end_location >= payload_size){
            LOG_INFO("overflowing, need to save partial first");

            uint8_t* buffer_start = buffer_location + first_open_byte_index * sizeof(uint8_t);
            uint16_t* struct_buffer_start = struct_buffer + struct_bytes_saved * sizeof(uint8_t);
            uint16_t length = payload_size - first_open_byte_index;

            memcpy(buffer_start, struct_buffer_start, length);

            for(int i = 0; i < 10; i++){
                LOG_INFO("adding packet, index: %i, with byte: %i",i, buffer_location[i]);
            }

            // now we need to add whatever is in the buffer to be sent to the radio
            add_packet_to_send_queue(buffer_location);
            struct_bytes_saved += length;
            first_open_byte_index = 0;
        }
        else{
            uint16_t* buffer_start = buffer_location + first_open_byte_index * sizeof(uint8_t);
            uint16_t* struct_buffer_start = struct_buffer + struct_bytes_saved * sizeof(uint8_t);
            uint16_t length = struct_buffer_size - struct_bytes_saved;

            memcpy(buffer_start, struct_buffer_start, length);
            first_open_byte_index += length;
            
            if(add_stop_after){
                if(first_open_byte_index < payload_size){
                    buffer_location[first_open_byte_index] = 255;
                    first_open_byte_index = 0;
                }
                else if(first_open_byte_index == payload_size){
                    first_open_byte_index = 0;
                }
                else{
                    LOG_INFO("THIS SHOULD NOT BE HAPPENING WHEN LOADING COMMANDS TO SEND OVER");
                }

                for(int i = 0; i < 10; i++){
                    LOG_INFO("adding packet, index: %i, with byte: %i",i, buffer_location[i]);
                }

                add_packet_to_send_queue(buffer_location);
            }
            struct_bytes_saved += length;
        }
    }
}

void send_test_commands(slate_t* slate, struct test_t1DS* struct1){

    uint8_t* buffer[251];
    struct1->data_int_1 = 100;
    struct1->data_byteArr_1[0] = 0;
    struct1->data_byteArr_1[1] = 1;
    struct1->data_byteArr_1[2] = 2;
    struct1->data_byteArr_1[3] = 3;


    LOG_INFO("number of bytes in command: %i", sizeof(*struct1));

    uint8_t struct_buffer[sizeof(*struct1)];

    memcpy(struct_buffer, struct1, sizeof(*struct1));

    add_commands_to_send_queue(&slate, buffer, 1, struct_buffer, 251, sizeof(*struct1), true);
    send_queue_to_satellite(&slate);
}
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

    /*
    struct test_t1DS first;
    struct test_t1DS second;

    struct test_t1DS first_out;
    struct test_t1DS second_out;

    first.data_int_1 = 229;
    first.data_byteArr_1[0] = 1;
    first.data_byteArr_1[1] = 2;
    first.data_byteArr_1[2] = 3;
    first.data_byteArr_1[249] = 101;

    second.data_int_1 = 303;
    second.data_byteArr_1[0] = 111;
    second.data_byteArr_1[9] = 222;
    second.data_byteArr_1[249] = 101;


    uint8_t packet1[251];
    uint8_t packet2[251];

    packet1[0] = 1;

    LOG_INFO("PLEASE I SWEAR TO FUCKING OD, size is: %i", sizeof(first));

    memcpy(&packet1[1], &first, 250);
    
    
    LOG_INFO("meow");

    // (int*)((char*)first+250);
    sleep_ms(500);
    char *byte_pointer = (char *)&first;

    memcpy(packet2, byte_pointer + 250, 304 - 250);


    LOG_INFO("Packet before sending:");

    sleep_ms(500);
    */

    command_switch_task_init(&slate);  // init queue

    struct test_t1DS first;
    send_test_commands(&slate, &first);

    
    // meow
    //queue_try_add(&slate.radio_packets_out, packet1);
    //queue_try_add(&slate.radio_packets_out, packet2);

    LOG_INFO("Radio Payload added to queue");

    command_switch_dispatch(&slate);
    
    //command_switch_dispatch(&slate);

    // get the struct
    struct test_t1DS first_out;
    queue_try_remove(&slate.task1_data, &first_out);

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
