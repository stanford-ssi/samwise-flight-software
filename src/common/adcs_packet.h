/**
 * @author Niklas Vainio
 * @date 2025-05-27
 *
 * This file defines the ADCS telemetry struct.
 * IMPORTANT: KEEP UP TO DATE WITH THE ADCS BOARD
 *
 * Last updated: 05/27/2025
 *
 * (this must live in a separate file for C include reasons)
 */

#pragma once

#include <stdint.h>

typedef struct __attribute__((packed))
{
    // Angular velocity
    float w;

    // Quaternion Estimate
    float q0, q1, q2, q3;

    // Misc Data
    char state;
    uint32_t boot_count;

} adcs_packet_t;