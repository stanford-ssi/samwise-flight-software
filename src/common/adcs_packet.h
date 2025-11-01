/**
 * @author Niklas Vainio, Lundeen Cahilly
 * @date 2025-10-31
 *
 * This file defines the ADCS telemetry struct.
 * IMPORTANT: KEEP UP TO DATE WITH THE ADCS BOARD
 *
 * Last updated: 10/31/2025
 *
 * (this must live in a separate file for C include reasons)
 */

#pragma once

#include <stdint.h>

typedef struct __attribute__((packed))
{
    // State
    char state;

    // Boot count
    uint8_t boot_count;

    // Quaternion (ECI to body, x,y,z,w order)
    float q_eci_to_body_x;
    float q_eci_to_body_y;
    float q_eci_to_body_z;
    float q_eci_to_body_w;

    // Angular velocity (body frame)
    float w_body_x;
    float w_body_y;
    float w_body_z;

    // Power
    float adcs_power;

    // Sun vector (body frame)
    float sun_vector_body_x;
    float sun_vector_body_y;
    float sun_vector_body_z;

    // Magnetic field (body frame, raw)
    float b_body_raw_x;
    float b_body_raw_y;
    float b_body_raw_z;

    // Magnetorquer moment
    float magnetorquer_moment_x;
    float magnetorquer_moment_y;
    float magnetorquer_moment_z;

    // Reaction wheels angular velocity
    float w_reaction_wheels_0;
    float w_reaction_wheels_1;
    float w_reaction_wheels_2;
    float w_reaction_wheels_3;

    // Validity flags
    bool magnetometer_data_valid;
    bool gps_data_valid;
    bool imu_data_valid;
    bool sun_vector_valid;

    // Sun sensor data validity (16 bools)
    bool sun_sensor_data_valid[16];

    // Covariance
    float P_log_frobenius;

    // GPS data
    float lat;
    float lon;
    float alt;

    // Time
    float mjd;
} adcs_packet_t;