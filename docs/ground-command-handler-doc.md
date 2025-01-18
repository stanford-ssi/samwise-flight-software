
## Ground Control

### This function splits the command calls (command_id +  data structure for this command)  into packets and adds it to the queue

###### See example usage below:
```c
/* Defining some struct for the command data */

struct command1_struct{
	int data_int_1;
	
	uint8_t data_byteArr_1[300];
};

  
// create struct and populate data for some command1
struct command1_struct first;
first.data_int_1 = 100;
first.data_byteArr_1[0] = 0;
first.data_byteArr_1[1] = 1;
first.data_byteArr_1[2] = 2;
first.data_byteArr_1[3] = 3;

// create a byte array with all our data 
uint8_t struct_buffer[sizeof(first)];  
memcpy(struct_buffer, &first, sizeof(first));

uint8_t command_id = 1; // set the command id that we are alling
uint8_t packet_size = 251;
bool last_command_in_packet = False;

// splits the command call into packets (if needed) and adds it to the command queue
// **Note that using this function allows us to pack multiple command calls into one packet if the size allows

add_commands_to_send_queue(&slate, buffer, command_id, struct_buffer, packet_size, sizeof(first), last_command_in_packet);

```


###### function definition:
```c
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
```


### Sending queue with packets to the satellite radio
- NOTE: `queue_try_add(&(s->radio_packets_out), payload);`  should be replace with the code for sending data to the radio

```c

void send_queue_to_satellite(slate_t *s){
	
	uint8_t payload[PACKET_SIZE];
	
	LOG_INFO("sending entire queue to radio");
	
	while(queue_try_remove(&mission_control_command_queue, payload)){
		// Then put it into the radio queue
		
		// This should be replaced with the radio code to 
		queue_try_add(&(s->radio_packets_out), payload);
		LOG_INFO("Sent a command to the radio");
	
	}

	LOG_INFO("finished sending queue to radio");
	first_open_byte_index = 0;

}
```



# Main.c testing code that can be run on the pico simulating ground station operations

```c
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

```
