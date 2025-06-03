/**
 * @author  Thomas Haile
 * @date    2025-05-17
 *
 * Task to emit telemetry packet to the radio TX queue.
 */

#include "beacon_task.h"
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
} __attribute__((__packed__)) beacon_stats;

_Static_assert(sizeof(beacon_stats) + MAX_STR_LENGTH + 1 <= MAX_DATA_SIZE, "beacon packet too large");

// Serialize the slate into a byte array and return its size.
size_t serialize_slate(slate_t *slate, uint8_t *data)
{
    // Copy null-terminated name to buffer (up to MAX_STR_LENGTH - 1)
    size_t name_len = strnlen(slate->current_state->name, MAX_STR_LENGTH);
    strlcpy((char *)data, slate->current_state->name, name_len);

    beacon_stats stats = {
        .reboot_counter = slate->reboot_counter,
        .time = slate->time_in_current_state_ms,
        .rx_bytes = slate->rx_bytes,
        .rx_packets = slate->rx_packets,
        .rx_backpressure_drops = slate->rx_backpressure_drops,
        .rx_bad_packet_drops = slate->rx_bad_packet_drops,
        .tx_bytes = slate->tx_bytes,
        .tx_packets = slate->tx_packets,
    };

    memcpy(data + name_len + 1, &stats, sizeof(beacon_stats));
    return name_len + 1 + sizeof(beacon_stats);
}

void beacon_task_init(slate_t *slate)
{
    LOG_DEBUG("Beacon task is initializing...");
}

void beacon_task_dispatch(slate_t *slate)
{
    // Create a new packet for radio TX
    packet_t pkt;
    pkt.src = 0;
    pkt.dst = 0;
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