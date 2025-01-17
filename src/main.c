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
struct function1_struct{
    int data_int_1;
    uint8_t data_byteArr_1[300];
};

struct function2_struct{
    bool yes_or_no;
    uint16_t testing;
};

/// @brief Initializes the mission control command queue, which holds commands before they are sent off to the satellite.
/// @param payload_size - The amount of bytes of CONTENT in a packet, excluding special information. Usually 251.
void init_mission_control_command_queue(uint16_t payload_size){
    queue_init(&mission_control_command_queue, payload_size * sizeof(uint8_t), 32);
}


void add_packet_to_send_queue(uint8_t* buffer){
    LOG_INFO("queueing command");
    bool added = queue_try_add(&mission_control_command_queue, buffer);
}

void send_queue_to_satellite(slate_t *s){
    uint8_t payload[251];

    LOG_INFO("sending entire queue to radio");

    while(queue_try_remove(&mission_control_command_queue, payload)){

        // Then put it into the radio queue
        queue_try_add(&(s->radio_packets_out), payload);


        LOG_INFO("Sent a command to the radio");
    }

    LOG_INFO("finished sending queue to radio");

    first_open_byte_index = 0;

}


void add_commands_to_send_queue(slate_t* s, uint8_t* buffer_location, uint8_t function_number, uint8_t* struct_buffer, uint16_t payload_size, uint16_t struct_buffer_size, bool add_stop_after){
    uint16_t struct_bytes_saved = 0;

    buffer_location[first_open_byte_index] = function_number;
    first_open_byte_index += 1;

    // While we have not saved all of the bytes
    while(struct_bytes_saved < struct_buffer_size){

        // predict where the last byte will be located
        uint16_t predicted_end_location = first_open_byte_index + (struct_buffer_size - struct_bytes_saved);

        // If it will end after the payload, we need another packet
        if(predicted_end_location >= payload_size){

            uint8_t* buffer_start = buffer_location + first_open_byte_index * sizeof(uint8_t);
            uint8_t* struct_buffer_start = struct_buffer + struct_bytes_saved * sizeof(uint8_t);
            uint16_t length = payload_size - first_open_byte_index;

            memcpy(buffer_start, struct_buffer_start, length);

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

                add_packet_to_send_queue(buffer_location);
            }
            struct_bytes_saved += length;
        }
    }
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

    command_switch_task_init(&slate);  // init queues
    init_mission_control_command_queue(251);

    uint8_t buffer[251];

    struct function1_struct first;
    first.data_int_1 = 100;
    first.data_byteArr_1[0] = 0;
    first.data_byteArr_1[1] = 1;
    first.data_byteArr_1[2] = 2;
    first.data_byteArr_1[3] = 3;

    struct function2_struct struct2;
    struct2.yes_or_no = 1;
    struct2.testing = 25;


    uint8_t struct_buffer[sizeof(first)];

    memcpy(struct_buffer, &first, sizeof(first));

    add_commands_to_send_queue(&slate, buffer, 1, struct_buffer, 251, sizeof(first), false);

    uint8_t new_struct_buffer[sizeof(struct2)];

    memcpy(struct_buffer, &struct2, sizeof(struct2));

    add_commands_to_send_queue(&slate, buffer, 2, struct_buffer, 251, sizeof(struct2), true);

    send_queue_to_satellite(&slate);
    
    // meow
    //queue_try_add(&slate.radio_packets_out, packet1);
    //queue_try_add(&slate.radio_packets_out, packet2);

    LOG_INFO("Radio Payload added to queue");

    command_switch_dispatch(&slate);
    command_switch_dispatch(&slate);
    
    //command_switch_dispatch(&slate);

    // get the struct
    struct function1_struct first_out;
    struct function2_struct second_out;

    queue_try_remove(&slate.task1_data, &first_out);
    queue_try_remove(&slate.task2_data, &second_out);

    bool same = true;
    for(int i = 0; i < 300; i++){
        if(first.data_byteArr_1[i] != first_out.data_byteArr_1[i]){
            same = false;
        }
    }

    LOG_INFO("True or false, the byte arrays are the same: %i", same);

    LOG_INFO("bool: %i, number: %i", second_out.yes_or_no, second_out.testing);
    

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
