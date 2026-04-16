/**
 * @author Niklas Vainio
 * @date 2025-07-20
 *
 * ADCS hardware driver interface for UART communication
 */
#pragma once

#include "adcs_packet.h"
#include "slate.h"

typedef enum
{
    ADCS_SUCCESS = 0,
    ADCS_ERROR_INIT_FAILED,
    ADCS_ERROR_UART_FAILED,
    ADCS_ERROR_INVALID_PARAM,
    ADCS_WRITE_PACKET_TOO_BIG,
    ADCS_WRITE_SYN_UNSUCCESSFUL,
    ADCS_WRITE_TIMEDOUT,
    ADCS_HEADER_UNACKNOWLEDGED,
    ADCS_FINAL_WRITE_UNSUCCESSFUL,
    ADCS_WRITE_SUCCESS,
    ADCS_WRITE_CORRUPTED

} adcs_result_t;

// Sentinel bytes for commands
#define ADCS_SEND_TELEM ('T')
#define ADCS_HEALTH_CHECK ('?')
#define ADCS_HEALTH_CHECK_SUCCESS ('!')
#define ADCS_FLASH_ERASE ('E')
#define ADCS_FLASH_PROGRAM ('P')

#define MAX_DATA_BYTES (250)

typedef enum
{ // Add commands here.
    SEND_TELEM = ADCS_SEND_TELEM,
    HEALTH_CHECK = ADCS_HEALTH_CHECK

} adcs_tx_command;

typedef struct __attribute__((packed))
{
    adcs_tx_command command;
    uint8_t packet_length;

    uint8_t packet_data[MAX_DATA_BYTES]; // in the respective functions, queue
                                         // the data!
    // MAX_DATA_BYTES will have to be the field with the maximum required bytes.
} adcs_command_packet;

/**
 * Initialize ADCS hardware interface
 * @param slate Pointer to slate structure
 * @return ADCS_SUCCESS on success, error code otherwise
 */
adcs_result_t adcs_driver_init(slate_t *slate);

/**
 * Power on the ADCS board
 * @param slate Pointer to slate structure
 * @return ADCS_SUCCESS on success, error code otherwise
 */
adcs_result_t adcs_driver_power_on(slate_t *slate);

/**
 * Power off the ADCS board
 * @param slate Pointer to slate structure
 * @return ADCS_SUCCESS on success, error code otherwise
 */
adcs_result_t adcs_driver_power_off(slate_t *slate);

/**
 * Get the latest ADCS telemetry packet
 * @param slate Pointer to slate structure
 * @param packet Pointer to store the telemetry packet
 * @return ADCS_SUCCESS if telemetry retrieved successfully
 */
adcs_result_t adcs_driver_get_telemetry(slate_t *slate, adcs_packet_t *packet);

/**
 * Check if ADCS hardware is responding
 * @param slate Pointer to slate structure
 * @return true if ADCS is alive and responding
 */
bool adcs_driver_is_alive(slate_t *slate);

adcs_result_t write_adcs_tx_packet(slate_t *slate,
                                   adcs_command_packet *adcs_packet);
