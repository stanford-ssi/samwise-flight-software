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

    LOG_INFO("Expected integer 1: %i, received: %i", first.data_int_1, first_out.data_int_1);

    for(int i =240; i < 260; i++){
        LOG_INFO("Packet 1, Index %i: Expected: %i, Received %i", i, first.data_byteArr_1[i], first_out.data_byteArr_1[i]);
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
