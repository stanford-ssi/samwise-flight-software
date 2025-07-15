#include "packet.h"
#include "slate.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int GROUP_SIZE =
    32; // Number of packets to send before the sender requests an ACK
int MAX_ACK_RETRIES =
    6; // Number of times the sender will ask for an ACK before giving up
int MAX_RESEND_CYCLES = 6; // Number of times the sender will resend missing
                           // packets before giving up
int MAX_BAD_PACKETS =
    6; // Number of consecutive bad packets before the receiver aborts

uint8_t ZERO_CHUNK[32] = {0};
uint8_t ZERO = 0;

char SEND_ACK[] = "!SEND_ACK!";
char ABORT_FILE_TRANSFER[] = "!ABORT!";

enum packet_type
{
    DATA,
    ACK,
    ABORT
};

int CHUNK_SIZE = 252;

struct CDH
{
};

struct PTP
{
};

struct FileTransferProtocol
{
    int id;
    int chunk_size;
    struct CDH cdh;
    struct PTP ptp;
};

struct responseStruct
{
    int num_packets;
    int file_size;
};

int min(int num1, int num2);

// Create new bool packet_status_array
bool *initialize_packet_status_bool_arr(int length);

// Create new packet_status packet.
packet_t initialize_packet_status_arr(int num_packets);

// Count number of missing_packets (zero bits elements in packet status data)
int count_missing_packets(packet_t status);

// Count number of missing_packets in a bool arr
int count_missing_packets_bool_arr(bool *packets_status, int length);

// Convert bool* array to a packet_t to send
packet_t bool_to_packet(bool *bool_array, int length);

// Convert packet_t back to bool* array once received
bool *packet_to_bool(packet_t status);

// Convert string to uint8_t* to send ACKNOWLEDGEMENT and ABORT packets
uint8_t *string_to_uint8_t(const char *str);

// Check if received packet is ABORT or ACKNOWLEDGEMENT
enum packet_type check_packet_type(packet_t pkt);

// Add uint8_t* data to packet
bool add_data_to_packet(uint8_t *data, int data_size, packet_t *packet);

// Receive a file
bool receive_file(char *local_path, slate_t *slate);

// Send a file
bool send_file(char *filename);

// Send missing packets
bool send_missing_packets(FILE *fptr, int cur_group, slate_t *slate);

// Obtain what packets are missing
bool *request_missing_packets(slate_t *slate);
