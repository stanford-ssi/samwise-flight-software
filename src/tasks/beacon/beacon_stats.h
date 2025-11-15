/* Generated header to expose beacon_stats to tests and other modules */
#ifndef SRC_TASKS_BEACON_BEACON_STATS_H
#define SRC_TASKS_BEACON_BEACON_STATS_H

#include <stdint.h>

// Packed struct used by beacon_task for telemetry -- keep in sync with
// the Kaitai .ksy `beacon_task` definition (reboot_counter .. device_status).
typedef struct {
    uint32_t reboot_counter;
    uint64_t time;
    uint32_t rx_bytes;
    uint32_t rx_packets;
    uint32_t rx_backpressure_drops;
    uint32_t rx_bad_packet_drops;
    uint32_t tx_bytes;
    uint32_t tx_packets;
    uint16_t battery_voltage;
    uint16_t battery_current;
    uint16_t solar_voltage;
    uint16_t solar_current;
    uint8_t device_status;
} __attribute__((__packed__)) beacon_stats;

#endif // SRC_TASKS_BEACON_BEACON_STATS_H
#ifndef SRC_TASKS_BEACON_BEACON_STATS_H
#define SRC_TASKS_BEACON_BEACON_STATS_H

#include <stdint.h>

// Definition of the packed beacon_stats struct used for over-the-air packets.
// Keep this file small and focused so tests can parse it easily.

typedef struct {
    uint32_t reboot_counter;
    uint64_t time;
    uint32_t rx_bytes;
    uint32_t rx_packets;
    uint32_t rx_backpressure_drops;
    uint32_t rx_bad_packet_drops;
    uint32_t tx_bytes;
    uint32_t tx_packets;
    uint16_t battery_voltage;
    uint16_t battery_current;
    uint16_t solar_voltage;
    uint16_t solar_current;
    uint8_t device_status;
} __attribute__((__packed__)) beacon_stats;

#endif // SRC_TASKS_BEACON_BEACON_STATS_H
