/**
 * @author  Thomas Haile
 * @date    2025-05-17
 *
 * Task to emit telemetry packet to the radio TX queue.
 */

#include "beacon_task.h"
#include "adcs_packet.h"
#include "neopixel.h"
#include <stdlib.h>

#define MAX_DATA_SIZE 252
#define MAX_STR_LENGTH 64
#define CALLSIGN "KC3WNY"
#define CALLSIGN_LENGTH (sizeof(CALLSIGN))

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

_Static_assert(sizeof(beacon_stats) + MAX_STR_LENGTH + 1 +
                       sizeof(adcs_packet_t) + CALLSIGN_LENGTH <=
                   MAX_DATA_SIZE,
               "beacon packet too large");

size_t send_boot_count(slate_t *slate, uint8_t *data)
{
    // Copy reboot counter into the data buffer
    uint32_t reboot_counter = slate->reboot_counter;
    memcpy(data, &reboot_counter, sizeof(reboot_counter));

    return sizeof(reboot_counter);
}

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
size_t serialize_slate(slate_t *slate, uint8_t *data, uint8_t tx_power)
{
    LOG_INFO("Serializing slate for beacon... %p -> %p", slate, data);
    LOG_INFO("State name: %s", slate->current_state->name);
    // Copy null-terminated name to buffer (up to MAX_STR_LENGTH - 1)
    size_t name_len = strnlen(slate->current_state->name, MAX_STR_LENGTH);
    strncpy((char *)data, slate->current_state->name, name_len);
    data[name_len] = '\0'; // Ensure null termination

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

    // 1 Extra byte: 1 for \0 terminator
    memcpy(data + name_len + 1, &stats, sizeof(beacon_stats));

    // Copy adcs packet - device status will indicate if this is invalid
    memcpy(data + name_len + 1 + sizeof(beacon_stats), &slate->adcs_telemetry,
           sizeof(adcs_packet_t));

    // Add callsign at the end of the packet
    memcpy(data + name_len + 1 + sizeof(beacon_stats) + sizeof(adcs_packet_t),
           CALLSIGN, CALLSIGN_LENGTH);

    // Add current TX power we're sending at
    memcpy(data + name_len + 1 + sizeof(beacon_stats) + sizeof(adcs_packet_t),
           &tx_power, sizeof(tx_power));

    return name_len + 1 + sizeof(beacon_stats) + sizeof(adcs_packet_t) +
           CALLSIGN_LENGTH + sizeof(tx_power);
}

void beacon_task_init(slate_t *slate)
{
    LOG_DEBUG("Beacon task is initializing...");
}

#define NUM_PACKETS_TO_SEND 1000

void beacon_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(BEACON_TASK_COLOR);
    // Create a new packet for radio TX
    packet_t pkt;
    pkt.src = 0;   // TODO Put in Samwise's node ID
    pkt.dst = 255; // Broadcast address
    pkt.flags = 0;
    pkt.seq = 0;

    // Commit into serialized byte array
    for (int tx_power = 0; tx_power <= 20; tx_power++)
    {

        LOG_INFO(" *** Setting rfm9x tx power to: %d ***\n", tx_power);
        rfm9x_set_tx_power(&slate->radio, tx_power);
        ASSERT(rfm9x_get_tx_power(&slate->radio) == tx_power);

        for (int i = 0; i < NUM_PACKETS_TO_SEND; i++)
        {
            pkt.len = serialize_slate(slate, pkt.data, tx_power);

            // LOG_INFO("[beacon_task] Boot count: %d", slate->reboot_counter);

            // Write into tx_queue
            if (queue_try_add(&slate->tx_queue, &pkt))
            {
                LOG_INFO("Beacon pkt added to queue");
            }
            else
            {
                LOG_ERROR("Beacon pkt failed to commit to tx_queue");
            }
            // neopixel_set_color_rgb(0, 0, 0);
        }

        LOG_INFO(" *** Finished sending packets at tx power: %d***\n",
                 tx_power);
    }
}

sched_task_t beacon_task = {.name = "beacon",
                            .dispatch_period_ms = 5000,
                            .task_init = &beacon_task_init,
                            .task_dispatch = &beacon_task_dispatch,
                            /* Set to an actual value on init */
                            .next_dispatch = 0};
