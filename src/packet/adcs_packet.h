/**
 * @author Niklas Vainio
 * @date 2025-05-27
 *
 * This file defines the ADCS telemetry struct.
 * IMPORTANT: KEEP UP TO DATE WITH THE ADCS BOARD
 *
 * Last updated: 07/24/2026
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

    // Time
    float mjd;
    float UTC_time;

    // Power
    float voltage;
    float current;

    // Body frame sensor measurements
    //
    // We use these to validate the attitude estimate from the MEKF by running
    // an independent TRIAD attitude estimate
    //
    // We can also use these to validate the sensor health by comparing the
    // measured values to the expected values, given the EKF's attitude estimate
    // * sun_resid = angle( q ⊗ sun_eci(mjd, utc_time) ,  sun_body )
    // * mag_resid = angle( q ⊗ mag_eci(lat, lon, alt) ,  mag_body )
    float sun_body_x, sun_body_y, sun_body_z;
    float mag_body_x, mag_body_y, mag_body_z;

    // Longitude, latitude, and altitude of the satellite
    float lon, lat, alt;

    // Misc Data
    char state;
    uint32_t boot_count;

} adcs_packet_t;
