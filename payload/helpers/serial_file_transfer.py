"""
`serial_file_transfer`
====================================================

CircuitPython compatible file transfer protocol, designed
for transfering data between flight computer and payload
over UART

Similar to normal file transfer but uses SerialPacketHandler
interface, and has parameters changed as UART can support larger
packet sizes than radio

This is the raspberry pi specific version

* Author(s): 
 - Flynn Dreilinger
 - Niklas Vainio

Implementation Notes
--------------------

"""
import os
import math
import json

import time
import logging

from helpers.serial_packet_handler import SerialPacketHandler

# TODO 
#Â FIX INITIAL MISSING BUG IN MAIN FILE TRANSFER

# Constants for configuring file transfer
PACKET_SIZE = 1024       # Size of each packet
GROUP_SIZE = 32         # Number of packets to send before the sender requests an ACK

MAX_ACK_RETRIES = 3     # Number of times the sender will ask for an ACK before giving up
MAX_RESEND_CYCLES = 3   # Number of times the sender will resend missing packets before giving up
MAX_BAD_PACKETS = 3     # Number of consecutive bad packets before the receiver aborts

# Special strings for commands between sender and receiver
SEND_ACK = b"!SEND_ACK!"
ABORT_FILE_TRANSFER = b"!ABORT!"

_WRITE_BUF = bytearray(PACKET_SIZE)

log = logging.getLogger(__name__)

class SerialFileTransfer:
    write_buf_view = memoryview(_WRITE_BUF)

    def __init__(self, serial_port):
        # Take in reference to serial port hardware object
        self.packet_handler = SerialPacketHandler(serial_port)

    def receive_file(self, local_path):
        # Receive number of packets from sender
        log.info(f"Waiting to receive num packets...")

        response = self.packet_handler.read_packet()
        if response is None: return False

        num_packets = int.from_bytes(response[0], "big")

        log.info(f"Expecting to receive {num_packets} packets")

        # Initialize next GROUP_SIZE as missing 
        missing_packets = [i for i in range(0, min(GROUP_SIZE, num_packets))]
        log.info(f"Receiving group {missing_packets[0]}-{missing_packets[-1]}...")

        group_start = 0
        consecutive_bad_packets = 0
        received_last_packet = False

        with open(local_path, 'wb+') as f:

            while consecutive_bad_packets < MAX_BAD_PACKETS:
                # Receive a packet
                log.info(f"Waiting for packet...")
                response = self.packet_handler.read_packet()

                if response is None:
                    #Â Bad packet
                    consecutive_bad_packets += 1
                    log.info(f"Received a bad packet! ({consecutive_bad_packets} in a row)")
                    continue

                # Good packet - reset consecutive count and remove from missing
                consecutive_bad_packets = 0
                chunk, seq_num = response

                if seq_num in missing_packets: missing_packets.remove(seq_num)
                log.info(f"Received packet {seq_num}!")
                log.info(f"{missing_packets} are missing!")    

                # Special command to abort the transfer
                if chunk == ABORT_FILE_TRANSFER:
                    log.info(f"ABORTING FILE TRANSFER!")
                    return False

                # Special command to send an acknowledgement
                elif chunk == SEND_ACK:
                    # Send back missing packets
                    log.info(f"Sending ack...")
                    self.packet_handler.write_packet(json.dumps(missing_packets).encode("utf-8"))

                    # If no more packets are missing, move to the next group of packets
                    if missing_packets == []:
                        # If we have already reached the end of the file, transfer is complete
                        if received_last_packet:
                            log.info(f"File transfer is COMPLETE!")
                            return True

                        group_start += GROUP_SIZE
                        missing_packets = [i for i in range(group_start, min(group_start + GROUP_SIZE, num_packets))]
                        
                        log.info(f"Group was successfully received!")
                        log.info(f"New group is packets {missing_packets[0]}-{missing_packets[-1]}")

                else:
                    # Otherwise, this is a normal packet so write to the file
                    f.seek(seq_num * PACKET_SIZE)
                    f.write(chunk)
                
                    f.flush()
                    os.sync()
                    log.info(f"Wrote packet to file!")

                    # Flag if we just received the last packet
                    if seq_num == num_packets - 1:
                        received_last_packet = True

            # Return false if we escape the loop
            return False

 

    def send_file(self, filename):
        """
        Send a file. This should only be used as a callback when a request 
        is received

        Args:
            filename (str): path to file that will be sent
        """
        log.info(f"Sending file {filename}...")

        write_buf_view = self.write_buf_view
        if not os.path.exists(filename):
            raise FileNotFoundError(f"file does not exist {filename}")

        with open(filename, 'rb') as f:
            stats = os.stat(filename)
            filesize = stats[6]
            
            # Send number of packets to receiver
            num_full_packets = math.floor(filesize / PACKET_SIZE)
            num_packets = math.ceil(filesize / PACKET_SIZE)
            
            self.packet_handler.write_packet(num_packets.to_bytes(4, "big"))

            log.info(f"About to send {num_packets} packets...")

            # Give receiver time to catch up
            time.sleep(0.5)
        
            for seq_num in range(num_full_packets):

                # Read this packet
                f.seek(seq_num * PACKET_SIZE)
                write_buf_view[0:PACKET_SIZE] = f.read(PACKET_SIZE)

                # Send this packet
                log.info(f"Sending packet {seq_num}...")
                self.packet_handler.write_packet(
                    write_buf_view[0:PACKET_SIZE],
                    seq_num=seq_num
                )

                # If we have reached the end of a group
                if seq_num % GROUP_SIZE == GROUP_SIZE - 1:

                    # Handle missing packets - abort if receiver unresponsive, or too many retries
                    result = self._send_missing_packets(f)
                    if not result: return False
                        
                    
            # Send partial packet at the end
            if num_packets != num_full_packets:
                seq_num = num_full_packets

                log.info(f"Sending packet {seq_num} (partial last packet!)...")

                last_packet_size = filesize % PACKET_SIZE

                f.seek(seq_num * PACKET_SIZE)
                write_buf_view[0:last_packet_size] = f.read(last_packet_size)

                self.packet_handler.write_packet(
                    write_buf_view[0:last_packet_size],
                    seq_num=seq_num
                )

            # Handle receiving missing packets
            result = self._send_missing_packets(f)
            
            if result:
                log.info(f"File transfer completed successfully!")
            


    def _send_missing_packets(self, f):
        # Coordinate to re-send any missing packets from the last group
        write_buf_view = self.write_buf_view

        for _ in range(MAX_RESEND_CYCLES):

            missing_packets = self._request_missing_packets()

            if missing_packets is None:
                # Receiver did not respond - abort file transfer
                log.info(f"Receiver is not responsive - aborting file transfer!")
                self.packet_handler.write_packet(ABORT_FILE_TRANSFER)
                return False

            # If no packets were missing, return True
            if missing_packets == []:
                log.info(f"Receiver has received this group successfully!")
                return True
            
            # Otherwise, resend missing packets
            for seq_num in missing_packets:
                f.seek(seq_num * PACKET_SIZE)
                write_buf_view[0:PACKET_SIZE] = f.read(PACKET_SIZE)

                log.info(f"Re-sending packet {seq_num}...")
                self.packet_handler.write_packet(
                    write_buf_view,
                    seq_num=seq_num
                )

        # Packets are still missing - abort file transfer
        log.info(f"Packets are still missing - aborting file transfer!")
        self.packet_handler.write_packet(ABORT_FILE_TRANSFER)
        return False


    def _request_missing_packets(self):
        # Ask the receiver to send which packets it missed

        for _ in range(MAX_ACK_RETRIES):
            log.info(f"Requesting missing packets...")
            self.packet_handler.write_packet(SEND_ACK)
            
            response = self.packet_handler.read_packet()

            # If response is not none, break
            if response is not None:
                missing_packets = json.loads(response[0])
                log.info(f"Receiver is missing packets {missing_packets}!")
                return missing_packets

            log.info(f"Receiver did not respond, trying again...")
        
        return None
