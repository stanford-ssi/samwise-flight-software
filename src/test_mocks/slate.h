#pragma once

#include <stdint.h>
#include <stdbool.h>

// Mock absolute_time_t for tests
typedef uint64_t absolute_time_t;

// Forward declarations
typedef struct samwise_slate slate_t;
typedef struct sched_state sched_state_t;
typedef struct sched_task sched_task_t;

// Mock types for tests
typedef struct {
    uint8_t dummy;
} queue_t;

typedef struct {
    uint8_t dummy;
} adcs_packet_t;

typedef struct {
    uint8_t dummy;
} rfm9x_t;

typedef struct {
    uint8_t dummy;
} onboard_led_t;

typedef struct {
    uint8_t dummy;
} watchdog_t;

#define MAX_DATASTRUCTURE_SIZE 304

typedef struct samwise_slate
{
#ifdef BRINGUP
    uint32_t loop_counter;
#endif

    uint32_t reboot_counter;
    sched_state_t *current_state;
    absolute_time_t entered_current_state_time;
    uint64_t time_in_current_state_ms;
    sched_state_t *manual_override_state;

    uint16_t battery_voltage;
    uint16_t battery_current;
    uint16_t solar_voltage;
    uint16_t solar_current;
    bool fixed_solar_charge;
    bool fixed_solar_fault;

    bool is_rbf_detected;
    bool panel_A_deployed;
    bool panel_B_deployed;

    watchdog_t watchdog;
    onboard_led_t onboard_led;

    queue_t payload_command_data;
    uint8_t struct_buffer[MAX_DATASTRUCTURE_SIZE];

    uint16_t num_uploaded_bytes;
    uint16_t packet_buffer_index;
    uint16_t last_place_on_packet;
    uint8_t uploading_command_id;
    uint8_t number_commands_processed;

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

    queue_t rpi_uart_queue;
    absolute_time_t rpi_uart_last_byte_receive_time;
    int curr_command_seq_num;
    bool is_payload_on;
    bool is_uart_init;

    bool is_adcs_on;
    uint32_t adcs_num_failed_checks;
    adcs_packet_t adcs_telemetry;
    bool is_adcs_telem_valid;

} slate_t;

extern slate_t slate;
