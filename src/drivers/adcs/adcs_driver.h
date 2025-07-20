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
    ADCS_ERROR_INVALID_PARAM
} adcs_result_t;

/**
 * Initialize ADCS hardware interface
 * @param slate Pointer to slate structure
 * @return ADCS_SUCCESS on success, error code otherwise
 */
adcs_result_t adcs_driver_init(slate_t *slate);

/**
 * Check if ADCS telemetry is available and valid
 * @param slate Pointer to slate structure
 * @return true if valid telemetry is available
 */
bool adcs_driver_telemetry_available(slate_t *slate);

/**
 * Get the latest ADCS telemetry packet
 * @param slate Pointer to slate structure
 * @param packet Pointer to store the telemetry packet
 * @return ADCS_SUCCESS if telemetry retrieved successfully
 */
adcs_result_t adcs_driver_get_telemetry(slate_t *slate, adcs_packet_t *packet);

/**
 * Send command to ADCS hardware
 * @param slate Pointer to slate structure
 * @param command Command data to send
 * @param length Length of command data
 * @return ADCS_SUCCESS if command sent successfully
 */
adcs_result_t adcs_driver_send_command(slate_t *slate, const char *command, size_t length);

/**
 * Reset ADCS hardware and communication state
 * @param slate Pointer to slate structure
 * @return ADCS_SUCCESS if reset successful
 */
adcs_result_t adcs_driver_reset(slate_t *slate);

/**
 * Check if ADCS hardware is responding
 * @param slate Pointer to slate structure
 * @return true if ADCS is alive and responding
 */
bool adcs_driver_is_alive(slate_t *slate);