/* Generated header to expose beacon_stats to tests and other modules */
#ifndef SRC_TASKS_BEACON_BEACON_STATS_H
#define SRC_TASKS_BEACON_BEACON_STATS_H

#include <stdint.h>

// Definition of the packed beacon_stats struct used for over-the-air packets.
// Keep this file small and focused so tests can parse it easily.
// Fields must stay in the same order as the Kaitai `.ksy` seq (reboot_counter..device_status)
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
