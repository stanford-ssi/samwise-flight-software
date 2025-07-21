/**
 * @author  Thomas Haile
 * @date    2025-05-17
 *
 * Task to emit telemetry packet to the radio TX queue.
 */

#include "beacon_task.h"
#include "adcs_packet.h"
#include <stdlib.h>

#define MAX_DATA_SIZE 252
#define MAX_STR_LENGTH 64

typedef struct
{
    uint32_t reboot_counter;
    uint64_t time;
    uint32_t rx_bytes;
    uint32_t rx_packets;
    uint32_t rx_backpressure_drops;
    uint32_t rx_bad_packet_drops;
    uint32_t tx_bytes;
    uint32_t tx_packets;
    uint16_t battery_voltage; // in mV (to 0.001V)
    uint16_t battery_current; // in mA (to 0.001A)
    uint16_t solar_voltage;   // in mV (to 0.001V)
    uint16_t solar_current;   // in mA (to 0.001A)
    uint8_t device_status;    // 0 for off, 1 for on
} __attribute__((__packed__)) beacon_stats;

_Static_assert(sizeof(beacon_stats) + MAX_STR_LENGTH + 1 <= MAX_DATA_SIZE,
               "beacon packet too large");

uint8_t get_device_status(slate_t *slate)
{
    // Return the device status (0 for off, 1 for on)
    // Packed into a 8-bit int field
    return (slate->is_rbf_detected << 0) | (slate->fixed_solar_charge << 1) |
           (slate->fixed_solar_fault << 2) | (slate->panel_A_deployed << 3) |
           (slate->panel_B_deployed << 4) | (slate->is_payload_on << 5) |
           (slate->is_adcs_on << 6) | (slate->is_adcs_telem_valid << 7);
}

// Serialize the slate into a byte array and return its size.
size_t serialize_slate(slate_t *slate, uint8_t *data)
{
    // Copy null-terminated name to buffer (up to MAX_STR_LENGTH - 1)
    size_t name_len = strnlen(slate->current_state->name, MAX_STR_LENGTH);
    strlcpy((char *)data, slate->current_state->name, name_len + 1);

    beacon_stats stats = {.reboot_counter = slate->reboot_counter,
                          .time = slate->time_in_current_state_ms,
                          .rx_bytes = slate->rx_bytes,
                          .rx_packets = slate->rx_packets,
                          .rx_backpressure_drops = slate->rx_backpressure_drops,
                          .rx_bad_packet_drops = slate->rx_bad_packet_drops,
                          .tx_bytes = slate->tx_bytes,
                          .tx_packets = slate->tx_packets,
                          .battery_voltage = slate->battery_voltage,
                          .battery_current = slate->battery_current,
                          .solar_voltage = slate->solar_voltage,
                          .solar_current = slate->solar_current,
                          .device_status = get_device_status(slate)};

    // 2 Extra bytes: 1 for length of string, 1 for \0 terminator
    memcpy(data + name_len + 2, &stats, sizeof(beacon_stats));

    // Copy adcs packet - device status will indicate if this is invalid
    memcpy(data + name_len + 2 + sizeof(beacon_stats), &slate->adcs_telemetry,
           sizeof(adcs_packet_t));

    return name_len + 2 + sizeof(beacon_stats) + sizeof(adcs_packet_t);
}

void beacon_task_init(slate_t *slate)
{
    LOG_DEBUG("Beacon task is initializing...");
}

void beacon_task_dispatch(slate_t *slate)
{
    // Create a new packet for radio TX
    packet_t pkt;
    pkt.src = 0;   // TODO Put in Samwise's node ID
    pkt.dst = 255; // Broadcast address
    pkt.flags = 0;
    pkt.seq = 0;

    // Commit into serialized byte array
    pkt.len = serialize_slate(slate, pkt.data);

    // Write into tx_queue
    if (queue_try_add(&slate->tx_queue, &pkt))
    {
        LOG_INFO("Beacon pkt added to queue");
    }
    else
    {
        LOG_ERROR("Beacon pkt failed to commit to tx_queue");
    }
}

sched_task_t beacon_task = {.name = "beacon",
                            .dispatch_period_ms = 30000,
                            .task_init = &beacon_task_init,
                            .task_dispatch = &beacon_task_dispatch,
                            /* Set to an actual value on init */
                            .next_dispatch = 0};