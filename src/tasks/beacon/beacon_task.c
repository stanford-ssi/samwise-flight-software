/**
 * @author  Yao Yiheng
 * @date    2025-01-18
 *
 * Task to emit telemetry packet to the radio TX queue.
 */

#include "beacon_task.h"

// Some limits on the data types
// Total of 64 chars reserved for name field.
//   - 1 byte for initial length
#define MAX_STR_LEN 63
#define MAX_DATA_SIZE 252

// Statically allocat a local byte array to serialize the slate into.
uint8_t tmp_data[MAX_DATA_SIZE];

// Write a single uint64_t into a byte array from a starting byte
// and return the new length.
size_t write_uint64(uint8_t *data, size_t pkt_len, uint64_t value)
{
    memcpy(&data[pkt_len], &value, sizeof(uint64_t));
    return pkt_len + sizeof(uint64_t);
}

// Recover a single uint64_t from a byte array from a starting byte.
void read_uint64(uint8_t *data, size_t pkt_len, uint64_t *value)
{
    memcpy(value, &data[pkt_len], sizeof(uint64_t));
}

// Write a single uint32_t into a byte array from a starting byte
// and return the new length.
size_t write_uint32(uint8_t *data, size_t pkt_len, uint32_t value)
{
    memcpy(&data[pkt_len], &value, sizeof(uint32_t));
    return pkt_len + sizeof(uint32_t);
}

// Recover a single uint64_t from a byte array from a starting byte.
void read_uint32(uint8_t *data, size_t pkt_len, uint32_t *value)
{
    memcpy(value, &data[pkt_len], sizeof(uint32_t));
}

// Write a string as a series of single-characters into a byte array.
// First uint8_t is the length of the encoded string.
size_t write_string(uint8_t *data, size_t pkt_len, const char *str)
{
    size_t str_len = strlen(str);
    if (str_len > MAX_STR_LEN)
    {
        LOG_ERROR("String: %s too long to encode", str);
        return pkt_len;
    }
    data[pkt_len++] = (uint8_t)str_len;
    for (size_t i = 0; i < str_len; ++i)
    {
        data[pkt_len++] = str[i];
    }
    return pkt_len;
}

// Serialize the slate into a byte array and return its size.
size_t serialize_slate(slate_t *slate, uint8_t *data)
{
    size_t pkt_len = 0;
    memset(data, 0, sizeof(data));

    // [64] Write the current state name
    pkt_len = write_string(data, pkt_len, slate->current_state->name);

    // [64 + 8 = 72] Write the current time
    pkt_len = write_uint64(data, pkt_len, slate->time_in_current_state_ms);

    // [72 + 4 = 76] Write the current rx_bytes
    pkt_len = write_uint32(data, pkt_len, slate->rx_bytes);

    // [76 + 4 = 80] Write the current rx_packets
    pkt_len = write_uint32(data, pkt_len, slate->rx_packets);

    // [80 + 4 = 84] Write the current rx_backpressure_drops
    pkt_len = write_uint32(data, pkt_len, slate->rx_backpressure_drops);

    // [84 + 4 = 88] Write the current rx_bad_packet_drops
    pkt_len = write_uint32(data, pkt_len, slate->rx_bad_packet_drops);

    // [88 + 4 = 92] Write the current tx_bytes
    pkt_len = write_uint32(data, pkt_len, slate->tx_bytes);

    // [92 + 4 = 96] Write the current tx_packets
    pkt_len = write_uint32(data, pkt_len, slate->tx_packets);

    if (pkt_len > MAX_DATA_SIZE)
    {
        LOG_ERROR("Serialized data too long: %d", pkt_len);
        return 0;
    }

    return pkt_len;
}

void beacon_task_init(slate_t *slate)
{
    memset(tmp_data, 0, sizeof(tmp_data));
}

void beacon_task_dispatch(slate_t *slate)
{
    // Create a new packet for radio TX
    packet_t pkt;
    pkt.src = 0;
    pkt.dst = 0;
    pkt.flags = 0;
    pkt.seq = 0;

    // Serialize data from slate variables
    // Current state name
    size_t pkt_len = serialize_slate(slate, tmp_data);

    // Commit into serialized byte array
    pkt.len = pkt_len;
    memcpy(pkt.data, tmp_data, pkt.len);

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
