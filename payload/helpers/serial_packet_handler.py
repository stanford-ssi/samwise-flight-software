from helpers.serial_port_pi import SerialPort

import time
import binascii
import logging

# Constants defining header
HEADER_LEN = 8

#Â Maxmimum packet size (in bytes)
MAX_PACKET_SIZE = 4096

# Constants for what is sent for acnowledgement
ACK_BYTE = b"!"
SYN_BYTE = b"$"
SYN_COUNT = 3
SYN_RETRIES = 3
PRIM_BYTE = b"@"

# Byte order for large numbers
BYTE_ORDER = "little"

log = logging.getLogger(__name__)
# Simple low level protocol that handles sending and packets
class SerialPacketHandler():

    def __init__(self, serial):
        # Init method that binds to a serial port
        self.serial_port = SerialPort(serial)

    
    def _calculate_header(self, packet: bytes, seq_num: int = 0) -> bytes:
        # Returns the header for a given packet
        header = bytearray(HEADER_LEN)

        # Bytes 0-1         Length
        # Bytes 2-3         Packet number (default to 0 if unset) 
        # Bytes 4-7         CRC 32 checksum (4 bytes)
        header[0:2] = len(packet).to_bytes(2, BYTE_ORDER)
        header[2:4] = seq_num.to_bytes(2, BYTE_ORDER)
        header[4:8] = binascii.crc32(packet).to_bytes(4, BYTE_ORDER)

        return bytes(header)
    
    
    def _decode_header(self, header: bytes) -> tuple[int, int, bytes]:
        # Take in a header and decode into length, packet num, and crc32 checksum
        length = int.from_bytes(header[0:2], BYTE_ORDER)
        seq_num = int.from_bytes(header[2:4], BYTE_ORDER)
        crc32 = header[4:8]

        return length, seq_num, crc32
    

    def _wait_for_sync(self):
        # Read bytes from the serial port until three $ have been sent
        count = 0

        while True:
            data = self.serial_port.read(1)
            if len(data) == 0:
                return False
            
            if data[0] == ord(SYN_BYTE):
                count += 1
            else:
                count = 0

            if count == SYN_COUNT: return True
    
    
    def _send_ack(self):
        # Send an acknowledgement
        self.serial_port.write(ACK_BYTE)

    
    def _receive_ack(self) -> bool:
        # Wait for an ack - return true if we received it
        result = self.serial_port.read(1)
        return result == ACK_BYTE

    
    def write_packet(self, packet: bytes, seq_num: int = 0) -> bool:
        # Method to write a packet - returns a boolean representing whether it was successfully acknowledged
        if len(packet) > MAX_PACKET_SIZE:
            raise Exception(f"Attempt to send packet too big (size {len(packet)} exceeds maximum of {MAX_PACKET_SIZE})")
        
        header = self._calculate_header(packet, seq_num=seq_num)

        #Send header (try a few times)
        for _ in range(SYN_RETRIES):
            log.debug("Sending sync...")
            self.serial_port.write(SYN_BYTE * SYN_COUNT)
            if self._receive_ack(): break

            time.sleep(0.1)
        else:
            # (strange syntax) - this runs if we do not receive the ack
            log.debug("Header was not acknowledged!")
            return False

        self.serial_port.write(PRIM_BYTE)
        
        log.debug("Writing header...")
        self.serial_port.write(header)

        if not self._receive_ack():
            log.debug("Header was not acknowledged!")
            return False

        # Send actual packet
        log.debug("Writing actual packet...")
        self.serial_port.write(packet)

        # Wait for ack from receiver
        log.debug("Awaiting ACK...")
        return self._receive_ack()

   
    def read_packet(self) -> tuple[bytes, int]:
        # Method to read a packet
        log.debug("Waiting for sync...")
        if not self._wait_for_sync():
            print("No sync arrived!")
            return None
        
        self._send_ack()

        # Wait for the start byte to come in
        count = 0
        while(self.serial_port.read(1) != PRIM_BYTE):
            log.debug("Waiting for the start byte, flushing sync bytes")

            if (count == 10):
                log.debug("Waiting for the start byte timeout!")
                return None

            count = count + 1

        # Read and decode header
        log.debug("Waiting for header...")
        header = self.serial_port.read(HEADER_LEN)

        if header is None or len(header) < HEADER_LEN:
            log.debug("Did not receive header!")
            return None
        
        packet_length, seq_num, expected_crc32 = self._decode_header(header)

        if packet_length > MAX_PACKET_SIZE:
            log.debug(f"Invalid packet length! ({packet_length} > {MAX_PACKET_SIZE})")
            return None
        
        # Acknowledge header
        self._send_ack()

        # Read packet
        log.debug(f"Reading {packet_length} bytes... (#{seq_num}, crc = {hex(int.from_bytes(expected_crc32, BYTE_ORDER))})")
        packet = self.serial_port.read(packet_length)

        if packet is None or len(packet) < packet_length:
            log.debug("Reading packet timed out!")
            return None

        # Verify checksum
        received_crc32 = binascii.crc32(packet).to_bytes(4, BYTE_ORDER)
        
        if expected_crc32 != received_crc32:
            log.debug("Invalid crc checksum!")
            return None
        
        # Send ack
        log.debug("Received valid packet! Sending ack...")
        self._send_ack()

        return packet, seq_num
    
