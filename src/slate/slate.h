/**
 * @author  Niklas Vainio
 * @date    2024-08-25
 *
 * This file defines the slate struct, a static struct which stores all data on
 * the satellite. At init time, a single instance of this struct gets statically
 * allocated, and it is referenced by all tasks and functions.
 *
 * Look up "blackboard pattern" for more info.
 */

#pragma once

#include "pico/types.h"
#include "pico/util/queue.h"

#include "state_machine.h"
#include "typedefs.h"

#include "onboard_led.h"
#include "rfm9x.h"
#include "watchdog.h"

// Largest possible command data structure
#define MAX_DATASTRUCTURE_SIZE 304

typedef struct samwise_slate
{
#ifdef BRINGUP
    /*
     * Bringup script
     */
    uint32_t loop_counter;
#endif

    /*
     * State machine info.
     */
    sched_state_t *current_state;
    absolute_time_t entered_current_state_time;
    uint64_t time_in_current_state_ms;

    /*
     * Watchdog
     */
    watchdog_t watchdog;

    /*
     * LED
     */
    onboard_led_t onboard_led;

    /*
     * Command switch
     */
    queue_t task1_data; // queues of this kind will exist for each task called
                        // from radio com
    queue_t task2_data;

    uint8_t struct_buffer[MAX_DATASTRUCTURE_SIZE];

    uint16_t num_uploaded_bytes;
    uint16_t packet_buffer_index;
    uint16_t last_place_on_packet;
    uint8_t uploading_command_id;

    /*
     * Radio
     */
    rfm9x_t radio;
    uint8_t radio_node;
    queue_t tx_queue;
    queue_t rx_queue;
    uint32_t rx_bytes;
    uint32_t rx_packets;
    uint32_t rx_backpressure_drops;
    uint32_t rx_bad_packet_drops;
    uint32_t tx_bytes;
    uint32_t tx_packets;

    /*
     * RPi UART Communication
     */ 
        queue_t rpi_uart_queue;
        absolute_time_t rpi_uart_last_byte_receive_time;

} slate_t;

extern slate_t slate;
