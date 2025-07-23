#include "file_transfer.h"

ft_file_t lookup_file(char *file_name)
{
}

bool record_file(char *file_name, int file_index)
{
}

bool ft_initialize_file(char *local_path, FILE *fptr)
{
}

bool ft_write_packet(char *local_path, slate_t *slate)
{
}

void ft_send_packet(slate_t *slate)
{
}

bool ft_check_file_states(char *file_name)
{
}

bool ft_send_initial_resp_packet(char *file_name, bool file_status)
{
}

/*
int min(int num1, int num2)
{
    if (num1 <= num2)
    {
        return num1;
    }

    else
    {
        return num2;
    }
}

bool *initialize_packet_status_bool_arr(int length)
{
    bool *packet_status = malloc(sizeof(bool) * length);

    for (int i = 0; i < length; i++)
    {
        packet_status[i] = false;
    }

    return packet_status;
}

// Create new packet_status_array. Instead of changing length of array, simply
// set last elements of array to true if num_packets < GROUP_SIZE
packet_t initialize_packet_status_arr(int num_packets)
{
    packet_t status;

    status.len = num_packets;
    for (int i = 0; i < 4; i++)
    {
        status.data[i] = 0;
    }

    return status;
}

// Count number of missing_packets (false elements in packets_status array)
int count_missing_packets(packet_t status)
{
    bool *missing_packets_bool_arr = packet_to_bool(status);
    return count_missing_packets_bool_arr(missing_packets_bool_arr, status.len);
}

// Count number of missing_packets in a bool arr
int count_missing_packets_bool_arr(bool *packets_status, int length)
{
    int missing_packets = 0;

    for (int i = 0; i < length; i++)
    {
        if (packets_status[i] == false)
        {
            missing_packets++;
        }
    }

    return missing_packets;
}


// Convert bool* array to char* so it can be sent as a packet
packet_t bool_to_packet(bool *bool_array, int length)
{
    packet_t status = initialize_packet_status_arr(length);

    for (int i = 0; i < length; i++)
    {
        if (bool_array)
        {
            status.data[i / 8] |= (1 << (i % 8));
        }
    }

    return status;
}

// Convert char* array back to bool* array once received
bool *packet_to_bool(packet_t status)
{
    bool *bool_array = malloc(sizeof(bool) * GROUP_SIZE);

    if (bool_array == NULL)
    {
        return NULL;
    }

    // Convert each bool to a corresponding char
    for (int i = 0; i < status.len; i++)
    {
        bool_array[i] = (status.data[i / 8] & (1 << (i % 8))) != 1;
    }

    return bool_array;
}

uint8_t *string_to_uint8_t(const char *str)
{
    uint8_t *to_return = malloc(strlen(str) * sizeof(uint8_t));

    for (int i = 0; i < strlen(str); i++)
    {
        to_return[i] = (uint8_t)(str[i]);
    }

    return to_return;
}

enum packet_type check_packet_type(packet_t pkt)
{
    uint8_t *ACKNOWLEDGEMENT = string_to_uint8_t(SEND_ACK);
    uint8_t *ABORT_TRANSFER = string_to_uint8_t(ABORT_FILE_TRANSFER);

    if (pkt.len == strlen(SEND_ACK))
    {
        if (memcmp(pkt.data, ACKNOWLEDGEMENT, pkt.len) == 0)
        {
            return ACK;
        }
    }

    if (pkt.len == strlen(ABORT_FILE_TRANSFER))
    {
        if (memcmp(pkt.data, ABORT_TRANSFER, pkt.len) == 0)
        {
            return ABORT;
        }
    }

    return DATA;
}

bool add_data_to_packet(uint8_t *data, int data_size, packet_t *packet)
{
    packet->len = data_size;

    for (int i = 0; i < data_size; i++)
    {
        packet->data[i] = data[i];
    }

    return true;
}

struct FileTransferProtocol FTP;
*/

/*
// Receive a file
bool receive_file(char *local_path, slate_t *slate)
{
    // struct responseStruct* response = FTP.cdh.receive_response(timeout=15);

    int num_packets = response->num_packets;
    int file_size = response->file_size;

    // File may be so small that not even a single full group would be filled
    int num_packets_to_receive = min(num_packets, GROUP_SIZE);

    // As packets are received, elements of array are set to true
    bool *packet_status_arr =
        initialize_packet_status_bool_arr(num_packets_to_receive);

    int n_groups = 0;
    int consecutive_bad_packets = 0;
    bool received_last_packet = false;

    // Create file on local path to receive
    FILE *fptr;
    fptr = fopen(local_path, "wb+");

    if (fptr == NULL)
    {
        // printf("File could not be opened!");
        return false;
    }

    // Expand file preemptively
    for (int i = 0; i < file_size / sizeof(ZERO_CHUNK); i++)
    {
        fwrite(ZERO_CHUNK, sizeof(uint8_t), sizeof(ZERO_CHUNK), fptr);
    }

    // TODO: Is ZERO_CHUNK being passed into fwrite the correct implementation?
    // What is ZERO?
    for (int i = 0; i < file_size % sizeof(ZERO_CHUNK); i++)
    {
        fwrite(ZERO_CHUNK, sizeof(uint8_t), 1, fptr);
    }

    fflush(fptr);

    // Start receiving packets
    while (consecutive_bad_packets < MAX_BAD_PACKETS)
    {
        packet_t packet;

        uint8_t *data = malloc(sizeof(uint8_t) * (CHUNK_SIZE));

        queue_try_remove(&slate->rx_queue, &packet);

        add_data_to_packet(data, CHUNK_SIZE, &packet);

        free(data);
        // packet_t packet = FTP.ptp.receive_packet(timeout=10);

        // Bad packet
        if (packet.data == NULL)
        {
            consecutive_bad_packets++;
            // printf("Received a bad packet! Consecutive bad packets at: %d",
            // consecutive_bad_packets);
            continue;
        }

        // Packet received successfully
        consecutive_bad_packets = 0;

        packet_status_arr[packet.seq] = true;

        // printf("Received packet: %d!", seq_num);

        // If packet tells us to abort_file_transfer
        if (check_packet_type(packet) == ABORT)
        {
            // printf("Aborting file transfer!");
            return false;
        }

        // Send acknowledgement of packets received
        else if (check_packet_type(packet) == ACK)
        {
            packet_t packets_status =
                bool_to_packet(packet_status_arr, num_packets_to_receive);

            queue_try_add(&slate->ft_send_queue, &packet);

            // FTP.cdh.send_response(packets_status);

            // This group is complete
            if (count_missing_packets(packets_status) == 0)
            {
                if (received_last_packet)
                {
                    // printf("File transfer is complete!");
                    return true;
                }

                // We're on the last group, so the number of packets to receive
                // is less than group_size
                if (n_groups == num_packets_to_receive / GROUP_SIZE &&
                    num_packets_to_receive % GROUP_SIZE > 0)
                {
                    num_packets_to_receive =
                        num_packets_to_receive % GROUP_SIZE;
                }

                // Not on the last group, so the number of packets we're
                // receiving is just the group_size
                else
                {
                    num_packets_to_receive = GROUP_SIZE;
                }

                n_groups++;

                free(packet_status_arr);

                // Reinitialize packets_status array
                packet_status_arr =
                    initialize_packet_status_bool_arr(num_packets_to_receive);

                // printf("Group %d successfully received", n_groups);
            }
        }

        // Normal packet, need to write to file
        else
        {
            fseek(fptr,
                  seq_num * CHUNK_SIZE + n_groups * CHUNK_SIZE * GROUP_SIZE,
                  SEEK_SET);

            fwrite(packet.data, sizeof(uint8_t), packet.len, fptr);

            fflush(fptr);

            // If this was the last packet
            if (seq_num + n_groups * GROUP_SIZE == num_packets - 1)
            {
                // printf("Received last packet!");
                received_last_packet = true;
            }
        }
    }

    return false;
}
*/

// Send a file
/*
bool send_file(char *filename)
{
    // printf("Sending file: %s", filename);

    FILE *fptr;
    fptr = fopen(filename, "rb");

    if (fptr == NULL)
    {
        // printf("File could not be opened!");
        return false;
    }

    fseek(fptr, 0L, SEEK_END); // Move the file pointer to the end
    long filesize =
        ftell(fptr); // Get the current position (which is the file size)

    // Obtain information about this file, and send to file-receiving
    int num_full_packets = (int)(filesize / CHUNK_SIZE);
    int total_packets =
        filesize % CHUNK_SIZE > 0 ? num_full_packets + 1 : num_full_packets;

    FTP.cdh.send_response(num_full_packets, total_packets);

    // Go through all full packets in this file
    for (int packet_number = 0; packet_number < num_full_packets;
         packet_number++)
    {
        // Get sequence of this packet within the current group
        int seq = packet_number % GROUP_SIZE;

        // Find this packet within the file, and obtain its data
        fseek(fptr, packet_number * CHUNK_SIZE, SEEK_SET);
        uint8_t *data = malloc(sizeof(char) * CHUNK_SIZE);
        fread(data, sizeof(uint8_t), CHUNK_SIZE, fptr);

        // Send this packet
        packet_t packet;
        packet.seq = seq;
        add_data_to_packet(data, CHUNK_SIZE, &packet);

        queue_try_add(&slate->ft_send_queue, &packet.data);
        // FTP.ptp.send_packet(data, seq);

        // If we reach the end of a group, send missing packets
        if (seq == GROUP_SIZE - 1)
        {
            bool result =
                send_missing_packets(fptr, packet_number / GROUP_SIZE, slate);

            if (!result)
            {
                return false;
            }
        }

        free(data);
    }

    // If there is an additional partially filled packet to send
    if (num_full_packets != total_packets)
    {
        int seq = num_full_packets % GROUP_SIZE;

        int last_packet_size = filesize % CHUNK_SIZE;

        fseek(fptr, num_full_packets * CHUNK_SIZE, SEEK_SET);

        uint8_t *data = malloc(sizeof(char) * last_packet_size);

        fread(data, sizeof(uint8_t), last_packet_size, fptr);

        packet_t packet;
        packet.seq = seq;

        add_data_to_packet(data, last_packet_size, &packet);

        queue_try_add(&slate->ft_send_queue, &packet);

        free(data);

        // FTP.ptp.send_packet(data, seq);
    }

    bool result = send_missing_packets(fptr, num_full_packets / GROUP_SIZE);

    if (result)
    {
        // printf("File transfer complete!");
        return true;
    }

    return false;
}
*/

// Send missing packets
/*
bool send_missing_packets(FILE *fptr, int cur_group, slate_t *slate)
{
    for (int i = 0; i < MAX_RESEND_CYCLES; i++)
    {
        // Obtain missing packets
        bool *missing_packets = request_missing_packets(slate);

        // Packets status was not received
        if (missing_packets == NULL)
        {
            packet_t abort;
            add_data_to_packet(string_to_uint8_t(ABORT_FILE_TRANSFER),
                               strlen(ABORT_FILE_TRANSFER), &abort);

            // printf("Receiver not responding - aborting file transfer!");

            queue_try_add(&slate->ft_send_queue, &abort);

            // FTP.ptp.send_packet(ABORT_FILE_TRANSFER);

            return false;
        }

        // No missing packets, we're done
        if (count_missing_packets_bool_arr(missing_packets) == 0)
        {
            // printf("We have received this group successfully!");
            return true;
        }

        // Loop through packets status
        for (int j = 0; j < GROUP_SIZE; j++)
        {
            // If current packet is missing
            if (missing_packets[j] == false)
            {
                // Get necessary file data
                fseek(fptr,
                      cur_group * GROUP_SIZE * CHUNK_SIZE + j * CHUNK_SIZE,
                      SEEK_SET);

                char *data = malloc(sizeof(char) * CHUNK_SIZE);

                fgets(data, CHUNK_SIZE, fptr);

                packet_t packet;
                packet.seq = j;
                add_data_to_packet(data, CHUNK_SIZE, &packet);

                // printf("Sending packet %d\n", j);

                queue_try_add(&slate->ft_send_queue, &packet);
                // FTP.ptp.send_packet(data);
                free(data);
            }
        }
    }

    // printf("Packets are still missing - aborting file transfer!");

    packet_t abort;
    add_data_to_packet(string_to_uint8_t(ABORT_FILE_TRANSFER),
                       strlen(ABORT_FILE_TRANSFER), &abort);

    queue_try_add(&slate->ft_send_queue, &abort);

    return false;
}
*/

// Obtain what packets are missing
/*
bool *request_missing_packets(slate_t *slate)
{
    bool received_successfully = false;

    for (int i = 0; i < MAX_ACK_RETRIES; i++)
    {
        // printf("Requesting missing packets");

        // Request an acknowledgement, which will send packets status
        packet_t ACK;
        add_data_to_packet(string_to_uint8_t(SEND_ACK), strlen(SEND_ACK), &ACK);

        queue_try_add(&slate->ft_send_queue, &ACK);

        // Receive packets status and convert to bool
        packet_t missing_packets_pkt;
        queue_try_remove(&slate->rx_queue, &missing_packets_pkt);

        bool *missing_packets = packet_to_bool(missing_packets_pkt);

        if (missing_packets != NULL)
        {
            // printf("Recever is missing packets!");
            return missing_packets;
        }

        // printf("Receiver did not respond, trying again!");
    }

    return NULL;
}
*/
