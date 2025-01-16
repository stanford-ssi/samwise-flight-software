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

queue_t mission_control_command_queue;
uint16_t first_open_byte_index = 0;

/* TESTS 2 function calls in one packet */
struct test_t1DS{
    int data_int_1;
    uint8_t data_byteArr_1[300];
};

void init_mission_control_command_queue(uint16_t payload_size){
    queue_init(&mission_control_command_queue, payload_size * sizeof(uint8_t), 32);
}
void add_packet_to_send_queue(uint8_t* buffer){
    LOG_INFO("ADDING THINGS TO THE SEND QUEUE");
    bool added = queue_try_add(&mission_control_command_queue, buffer);
    if(added){
        LOG_INFO("SUCCESSFULLY ADDED TO THE QUEUE");
    }
    else{
        LOG_INFO("there was an error in adding to the queue");
    }
}

void send_queue_to_satellite(slate_t *s){
    uint8_t payload[251];

    LOG_INFO("ATTEMPTING TO SEND THINGS TO THE RADIO");

    while(queue_try_remove(&mission_control_command_queue, payload)){
        sleep_ms(500);
        LOG_INFO("Sending a command to the radio");

        // Then put it into the radio queue
        queue_try_add(&(s->radio_packets_out), payload);
        sleep_ms(500);
        LOG_INFO("hopefully it was added?");
        sleep_ms(500);
    }

    first_open_byte_index = 0;

}


void add_commands_to_send_queue(slate_t* s, uint8_t* buffer_location, uint8_t function_number, uint8_t* struct_buffer, uint16_t payload_size, uint16_t struct_buffer_size, bool add_stop_after){
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
            uint8_t* struct_buffer_start = struct_buffer + struct_bytes_saved * sizeof(uint8_t);
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
            uint8_t* buffer_start = buffer_location + first_open_byte_index * sizeof(uint8_t);
            uint8_t* struct_buffer_start = struct_buffer + struct_bytes_saved * sizeof(uint8_t);
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

void send_test_commands(slate_t* s, struct test_t1DS* struct1){

    uint8_t buffer[251];
    struct1->data_int_1 = 100;
    struct1->data_byteArr_1[0] = 0;
    struct1->data_byteArr_1[1] = 1;
    struct1->data_byteArr_1[2] = 2;
    struct1->data_byteArr_1[3] = 3;


    LOG_INFO("number of bytes in command: %i", sizeof(*struct1));

    uint8_t struct_buffer[sizeof(*struct1)];

    memcpy(struct_buffer, struct1, sizeof(*struct1));

    add_commands_to_send_queue(s, buffer, 1, struct_buffer, 251, sizeof(*struct1), true);
    send_queue_to_satellite(s);
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
    first.data_byteArr_1[256] = 199;

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
    init_mission_control_command_queue(251);

    struct test_t1DS first;
    uint8_t* ptr;

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

    LOG_INFO("Expected integer 1: %i, received: %i", first.data_int_1, first_out.data_int_1);

    for(int i =0; i < 10; i++){
        LOG_INFO("Struct 1, Index %i: Expected: %i, Received %i", i, first.data_byteArr_1[i], first_out.data_byteArr_1[i]);
    }

/*
    for(int i =250 ; i < 300; i++){
        LOG_INFO("Packet 2, Index %i: Expected: %i, Received %i", i, first.data_byteArr_1[i], first_out.data_byteArr_1[i]);
    }
    */

    /* End of TESTS */

    /*
     * We should NEVER be here so something bad has happened.
     * @todo reboot!
     */
    ERROR("We reached the end of the code - this is REALLY BAD!");
}
