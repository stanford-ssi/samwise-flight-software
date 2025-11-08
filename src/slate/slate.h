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

#include "adcs_packet.h"
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
    uint32_t reboot_counter;
    sched_state_t *current_state;
    absolute_time_t entered_current_state_time;
    uint64_t time_in_current_state_ms;
    // Manually set next state to transition to
    sched_state_t *manual_override_state;

    /*
     * Power Telemetry
     */
    uint16_t battery_voltage; // in mV (to 0.001V)
    uint16_t battery_current; // in mA (to 0.001A)
    uint16_t solar_voltage;   // in mV (to 0.001V)
    uint16_t solar_current;   // in mA (to 0.001A)
    bool fixed_solar_charge;  // 0 for off status, 1 for on status
    bool fixed_solar_fault;   // 0 for no fault, 1 for faulty

    /*
     * Structure status readouts
     */
    bool is_rbf_detected; // true if the RBF is still attached, false if it has
                          // been removed

    /* c
     * Solar panels A and B
     */
    bool panel_A_deployed; // 1 for deployed
    bool panel_B_deployed; // 1 for deployed

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
    queue_t payload_command_data;

    uint8_t struct_buffer[MAX_DATASTRUCTURE_SIZE];

    uint16_t num_uploaded_bytes;
    uint16_t packet_buffer_index;
    uint16_t last_place_on_packet;
    uint8_t uploading_command_id;
    uint8_t number_commands_processed;

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
    int curr_command_seq_num;
    bool is_payload_on;
    bool is_uart_init;

    /*
     * ADCS board status and telemetry
     */
    bool is_adcs_on;
    uint32_t adcs_num_failed_checks;
    adcs_packet_t adcs_telemetry;
    bool is_adcs_telem_valid;

    /**
     * Filesystem API variables
     */
    // NOTE: A buffer ("cache") is provided by little-fs, but it is more meant
    // for efficiency on reads/writes rather than buffering like we want. Since
    // FILESYS_BUFFER_SIZE is not that pretty, we will create an extra buffer
    // that allows little-fs to write much cleaner numbers to MRAM for
    // efficiency gains.
    bool filesys_is_writing_file;
    bool filesys_buffer_is_dirty;
    uint8_t filesys_buffer[FILESYS_BUFFER_SIZE];
    FILESYS_BUFFERED_FNAME_T filesys_buffered_fname;
    FILESYS_BUFFERED_FILE_LEN_T filesys_buffered_file_len;
    FILESYS_BUFFERED_FILE_CRC_T filesys_buffered_file_crc;

    /**
     * File transfer protocol task
     */
    // A bit being set indicates that the corresponding packet has been recieved
    FTP_PACKET_TRACKER_T ftp_packets_recieved;
    queue_t ftp_start_file_write_data;
    queue_t ftp_write_to_file_data; // Realistically this should be an array of
                                    // size FTP_NUM_PACKETS_PER_CYCLE, but for
                                    // simplicity we will use a queue.
    queue_t ftp_cancel_file_write_data;
    queue_t ftp_format_filesystem_data;

} slate_t;

extern slate_t slate;
