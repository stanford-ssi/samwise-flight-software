#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "slate.h"


int GROUP_SIZE = 32;       // Number of packets to send before the sender requests an ACK
int MAX_ACK_RETRIES = 6;   // Number of times the sender will ask for an ACK before giving up
int MAX_RESEND_CYCLES = 6; // Number of times the sender will resend missing packets before giving up
int MAX_BAD_PACKETS = 6;   //Number of consecutive bad packets before the receiver aborts

unsigned char ZERO_CHUNK[32] = {0};
unsigned char ZERO = 0;

char SEND_ACK[] = "!SEND_ACK!";
char ABORT_FILE_TRANSFER[] = "!ABORT!";

int CHUNK_SIZE = 250;

struct CDH {

};

struct PTP {

};

struct FileTransferProtocol {
   int id;
   int chunk_size;
   struct CDH cdh;
   struct PTP ptp;
};

struct responseStruct {
   int num_packets;
   int file_size;
};

struct packetStruct {
   int seq_num;
   char* chunk;
   int packet_size;
};

int min(int num1, int num2) {
   if (num1 <= num2) {
      return num1;
   }

   else {
      return num2;
   }
}

// Count number of missing_packets (false elements in packets_status array)
int count_missing_packets(bool* packets_status) {
   int num_missing_packets = 0;
   
   for (int i = 0; i < GROUP_SIZE; i++) {
      if (packets_status[i] == false) {
         num_missing_packets++;
      }
   }

   return num_missing_packets;
}

// Create new packet_status_array. Instead of changing length of array, simply set last elements of array to true if num_packets < GROUP_SIZE
bool* initialize_packet_status_arr(int num_packets) {
   bool* packets_status;
   packets_status = malloc(sizeof(bool)*GROUP_SIZE);

   for (int i = 0; i < num_packets; i++) {
      packets_status[i] = false;
   }

   for (int i = num_packets; i < GROUP_SIZE; i++) {
      packets_status[i] = true;
   }

   return packets_status;
}

// Convert bool* array to char* so it can be sent as a packet hi
char* bool_to_char(bool* bool_array, int length) {
   char* char_array = malloc(sizeof(char)*GROUP_SIZE + 1); // +1 for null terminator (if needed)
   
   if (char_array == NULL) {
      //printf("Memory allocation failed\n");
      return NULL;
   }

   // Convert each bool to a corresponding char
   for (size_t i = 0; i < length; i++) {
      char_array[i] = bool_array[i] ? '1' : '0'; // Use '1' for true and '0' for false
   }

   char_array[i+1] = '\0';

   return char_array;
}

// Convert char* array back to bool* array once received
bool* char_to_bool(char* char_array) {
   bool* bool_array = malloc(sizeof(bool)*GROUP_SIZE); // +1 for null terminator (if needed)
   
   if (bool_array == NULL) {
      //printf("Memory allocation failed\n");
      return NULL;
   }

    // Convert each bool to a corresponding char
   for (int i = 0; i < GROUP_SIZE; i++) {
      if (char_array[i] == '\0') {
         break;
      }

      bool_array[i] = (char_array[i] == '1'); // Use '1' for true and '0' for false
   }

   return bool_array;
}

bool add_data_to_packet(char* data, int data_size, struct packetStruct *packet) {
   (*packet).packet_size = data_size;
   (*packet).chunk = malloc(((*packet).packet_size + 1) * sizeof(char));

   if ((*packet).chunk == NULL) {
      return false;
   }

   memcpy(packet->chunk, data, data_size);
   
   packet->chunk[data_size] = '\0'; // Null-terminate for safety

   return true;
}

struct FileTransferProtocol FTP;

// Receive a file
bool receive_file(char* local_path, slate_t slate) {
   struct responseStruct* response = FTP.cdh.receive_response(timeout=15);
   
   int num_packets = response->num_packets;
   int file_size = response->file_size;

   // File may be so small that not even a single full group would be filled
   int num_packets_to_receive = min(num_packets, GROUP_SIZE);

   // As packets are received, elements of array are set to true
   bool* packets_status = initialize_packet_status_arr(num_packets_to_receive);

   int n_groups = 0;
   int consecutive_bad_packets = 0;
   bool received_last_packet = false;

   // Create file on local path to receive
   FILE* fptr;
   fptr = fopen(local_path, "wb+");
            
   if (fptr == NULL) {
      //printf("File could not be opened!");
      return false;
   }

   // Expand file preemptively
   for (int i = 0; i < file_size / sizeof(ZERO_CHUNK); i++) {
      fwrite(ZERO_CHUNK, sizeof(unsigned char), sizeof(ZERO_CHUNK), fptr); 
   }

   for (int i = 0; i < file_size % sizeof(ZERO_CHUNK); i++) {
      fwrite(ZERO, sizeof(unsigned char), 1, fptr);
   }

   fflush(fptr);

   // Start receiving packets
   while (consecutive_bad_packets < MAX_BAD_PACKETS) {
      struct packetStruct packet;

      char* data = malloc(sizeof(char) * (CHUNK_SIZE));

      queue_try_remove(slate.rx_queue, data);

      add_data_to_packet(data, CHUNK_SIZE, &packet);

      free(data);
      //struct packetStruct packet = FTP.ptp.receive_packet(timeout=10);

      // Bad packet
      if (packet.chunk == NULL) {
         consecutive_bad_packets++;
         //printf("Received a bad packet! Consecutive bad packets at: %d", consecutive_bad_packets);
         continue;
      }

      // Packet received successfully
      consecutive_bad_packets = 0;

      int seq_num = packet.seq_num;
      char* data = packet.chunk;

      packets_status[seq_num] = true;

      //printf("Received packet: %d!", seq_num);

      // If packet tells us to abort_file_transfer
      if (strcmp(data, &ABORT_FILE_TRANSFER[0]) == 0) {
         //printf("Aborting file transfer!");
         return false;
      }

      // Send acknowledgement of packets received
      else if (strcmp(data, &SEND_ACK[0]) == 0) {
         //printf("Sending ack...");

         char* file_status = bool_to_char(packets_status, num_packets_to_receive);

         // Send packets received
         struct packetStruct packet;
         add_data_to_packet(file_status, num_packets_to_receive, &packet);
         queue_try_add(slate.tx_queue, packet.chunk);

         //FTP.cdh.send_response(packets_status);

         // This group is complete
         if (count_missing_packets(packets_status) == 0) {
            if (received_last_packet) {
               //printf("File transfer is complete!");
               return true;
            }

            // We're on the last group, so the number of packets to receive is less than group_size
            if (n_groups == num_packets_to_receive / GROUP_SIZE && num_packets_to_receive % GROUP_SIZE > 0) {
               num_packets_to_receive = num_packets_to_receive % GROUP_SIZE;
            }

            // Not on the last group, so the number of packets we're receiving is just the group_size
            else {
               num_packets_to_receive = GROUP_SIZE;
            }

            n_groups++;

            free(packets_status);

            // Reinitialize packets_status array
            packets_status = initialize_packet_status_arr(num_packets_to_receive);

            //printf("Group %d successfully received", n_groups);
         }
      }

      // Normal packet, need to write to file
      else {
         fseek(fptr, seq_num * CHUNK_SIZE + n_groups * CHUNK_SIZE * GROUP_SIZE, SEEK_SET);

         fwrite(data, 1, CHUNK_SIZE, fptr);

         fflush(fptr);

         // If this was the last packet
         if (seq_num + n_groups * GROUP_SIZE == num_packets - 1) {
            //printf("Received last packet!");
            received_last_packet = true;
         }
      }
   }

   return false;
}

// Send a file
bool send_file(char* filename) {
   //printf("Sending file: %s", filename);

   FILE* fptr;
   fptr = fopen(filename, "rb");
            
   if (fptr == NULL) {
      //printf("File could not be opened!");
      return false;
   }

   fseek(fptr, 0L, SEEK_END); // Move the file pointer to the end
   long filesize = ftell(fptr);   // Get the current position (which is the file size)
   
   // Obtain information about this file, and send to file-receiving
   int num_full_packets = (int)(filesize / CHUNK_SIZE);
   int total_packets = filesize % CHUNK_SIZE > 0 ? num_full_packets + 1 : num_full_packets;

   FTP.cdh.send_response(num_full_packets, total_packets);

   // Go through all full packets in this file
   for (int packet_number = 0; packet_number < num_full_packets; packet_number++) {
      // Get sequence of this packet within the current group
      int seq = packet_number % GROUP_SIZE;
      
      // Find this packet within the file, and obtain its data
      fseek(fptr, packet_number * CHUNK_SIZE, SEEK_SET);
      char* data = malloc(sizeof(char) * CHUNK_SIZE);
      fgets(data, CHUNK_SIZE, fptr);

      // Send this packet
      struct packetStruct packet;
      packet.seq_num = seq;
      add_data_to_packet(data, CHUNK_SIZE, &packet);

      queue_try_add(slate.tx_queue, packet.chunk);
      //FTP.ptp.send_packet(data, seq);

      // If we reach the end of a group, send missing packets
      if (seq == GROUP_SIZE - 1) {
         bool result = send_missing_packets(fptr, packet_number / GROUP_SIZE);

         if (!result) {
            return false;
         }
      }

      free(data);
   }

   // If there is an additional partially filled packet to send
   if (num_full_packets != total_packets) {
      int seq = num_full_packets % GROUP_SIZE;

      int last_packet_size = filesize % CHUNK_SIZE;

      fseek(fptr, num_full_packets * CHUNK_SIZE, SEEK_SET);

      char* data = malloc(sizeof(char)*last_packet_size);

      fgets(data, last_packet_size, fptr); 

      struct packetStruct packet;
      packet.seq_num = seq;
      
      add_data_to_packet(data, last_packet_size, &packet);

      queue_try_add(slate.tx_queue, packet.chunk);

      free(data);

      //FTP.ptp.send_packet(data, seq);
   }

   bool result = send_missing_packets(fptr, num_full_packets / GROUP_SIZE);

   if (result) {
      //printf("File transfer complete!");
      return true;
   }

   return false;
}

// Send missing packets
bool send_missing_packets(FILE* fptr, int cur_group, slate_t slate) {
   for (int i = 0; i < MAX_RESEND_CYCLES; i++) {
      // Obtain missing packets
      bool* missing_packets = request_missing_packets(slate);

      // Packets status was not received
      if (missing_packets == NULL) {
         struct packetStruct abort;
         add_data_to_packet(ABORT_FILE_TRANSFER, strlen(ABORT_FILE_TRANSFER), &abort);

         //printf("Receiver not responding - aborting file transfer!");

         queue_try_add(slate.tx_queue, abort.chunk);

         //FTP.ptp.send_packet(ABORT_FILE_TRANSFER);

         return false;
      }

      // No missing packets, we're done
      if (count_missing_packets(missing_packets) == 0) {
         //printf("We have received this group successfully!");
         return true;
      }

      // Loop through packets status
      for (int j = 0; j < GROUP_SIZE; j++) {
         // If current packet is missing
         if (missing_packets[j] == false) {
            // Get necessary file data
            fseek(fptr, cur_group * GROUP_SIZE * CHUNK_SIZE + j * CHUNK_SIZE, SEEK_SET);

            char* data = malloc(sizeof(char)*CHUNK_SIZE);
   
            fgets(data, CHUNK_SIZE, fptr);

            struct packetStruct packet;
            packet.seq_num = j;
            add_data_to_packet(data, CHUNK_SIZE, &packet);

            //printf("Sending packet %d\n", j);

            queue_try_add(slate.tx_queue, packet.chunk);
            //FTP.ptp.send_packet(data);
            free(data);
         }
      }
   }

   //printf("Packets are still missing - aborting file transfer!");

   struct packetStruct abort;
   add_data_to_packet(&ABORT_FILE_TRANSFER[0], strlen(ABORT_FILE_TRANSFER), &abort);

   queue_try_add(slate.tx_queue, abort.chunk);

   return false;
}

// Obtain what packets are missing
bool* request_missing_packets(slate_t slate) {
   bool received_successfully = false;

   for (int i = 0; i < MAX_ACK_RETRIES; i++) {
      //printf("Requesting missing packets");

      // Request an acknowledgement, which will send packets status
      struct packetStruct ACK;
      add_data_to_packet(SEND_ACK, strlen(SEND_ACK), &ACK;)

      queue_try_add(slate.tx_queue, ACK.chunk);

      // Receive packets status and convert to bool
      struct packetStruct missing_packets_char;
      queue_try_remove(slate.rx_queue, missing_packets_char.chunk);

      bool* missing_packets = char_to_bool(missing_packets_char.chunk);

      if (missing_packets != NULL) {
         //printf("Recever is missing packets!");
         return missing_packets;
      }

      //printf("Receiver did not respond, trying again!");
   }

   return NULL;
}
