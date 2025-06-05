import board
import busio
import digitalio
import time
import random
import struct
from adafruit_hashlib import sha256
import circuitpython_hmac as hmac


led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT

MOSI_PIN = board.GP15
MISO_PIN = board.GP12
SCK_PIN = board.GP14
CS_PIN = board.GP13
RESET_PIN = board.GP11

spi = busio.SPI(SCK_PIN, MOSI=MOSI_PIN, MISO=MISO_PIN)
cs = digitalio.DigitalInOut(CS_PIN)
reset = digitalio.DigitalInOut(RESET_PIN)

import adafruit_rfm9x

rfm9x = adafruit_rfm9x.RFM9x(spi, cs, reset, 438.1, crc=True)
rfm9x.signal_bandwidth = 125000
rfm9x.spreading_factor = 7
rfm9x.coding_rate = 5

# Packet constants
PACKET_HMAC_PSK = b""
CURRENT_BOOT_COUNT = 474
STARTING_MSG_ID = 1
PACKET_HEADER_SIZE = 5  # dst, src, flags, seq, len
PACKET_HMAC_SIZE = 32  # SHA256 size
PACKET_FOOTER_SIZE = 8 + PACKET_HMAC_SIZE  # boot_count (4) + msg_id (4) + hmac
PACKET_MAX_DATA_SIZE = 255 - PACKET_HEADER_SIZE - PACKET_FOOTER_SIZE


class PacketBuilder:
    def __init__(self):
        self.msg_id = STARTING_MSG_ID

    def create_packet(self, dst, src, flags, seq, data):
        if len(data) > PACKET_MAX_DATA_SIZE:
            raise ValueError(
                f"Data too large: {len(data)} bytes (max {PACKET_MAX_DATA_SIZE})"
            )

        # Pack header fields
        packet = struct.pack("BBBBB", dst, src, flags, seq, len(data))

        print(len(data), data)

        # Add data
        packet += data

        # Add boot count and msg_id
        packet += struct.pack("<II", CURRENT_BOOT_COUNT, self.msg_id)

        # Calculate HMAC
        h = hmac.new(PACKET_HMAC_PSK, msg=packet, digestmod=sha256)

        # print packet_with_padding, %02x each byte on a new line starting with [DEBUG]
        for i, byte in enumerate(packet):
            print(f"[DEBUG] {byte:02x}")

        packet += h.digest()

        self.msg_id += 1
        return packet[
            4:
        ]  # First 4 bytes are inserted by the RFM9x library, so we skip them

    def unpack_packet(self, packet_bytes):
        if len(packet_bytes) < PACKET_HEADER_SIZE:  # + PACKET_FOOTER_SIZE:
            raise ValueError("Packet too small")

        # Extract HMAC
        # received_hmac = packet_bytes[-PACKET_HMAC_SIZE:]
        # packet_without_hmac = packet_bytes[:-PACKET_HMAC_SIZE]

        # Verify HMAC
        # h = hmac.new(PACKET_HMAC_PSK, packet_without_hmac, hashlib.sha256)
        # if not hmac.compare_digest(h.digest(), received_hmac):
        #     raise ValueError("HMAC verification failed")

        # Unpack header
        dst, src, flags, seq, data_len = struct.unpack(
            "BBBBB", packet_bytes[:PACKET_HEADER_SIZE]
        )

        # Extract data
        data = packet_bytes[data_len:]

        # Unpack footer
        # footer_start = len(packet_bytes) - PACKET_FOOTER_SIZE
        # boot_count, msg_id = struct.unpack('<II', packet_bytes[footer_start:footer_start+8])

        return {
            "dst": dst,
            "src": src,
            "flags": flags,
            "seq": seq,
            "data": data,
            # 'boot_count': boot_count,
            # 'msg_id': msg_id
        }


packet_builder = PacketBuilder()


def try_get_packet():
    packet = rfm9x.receive(
        timeout=5.0
    )  # Wait for a packet to be received (up to 5 seconds)
    if packet is not None:
        try:
            unpacked = packet_builder.unpack_packet(packet)
            print(f"Received valid packet:")
            print(f"  Destination: {unpacked['dst']}")
            print(f"  Source: {unpacked['src']}")
            print(f"  Flags: {unpacked['flags']}")
            print(f"  Sequence: {unpacked['seq']}")
            print(f"  Data: {unpacked['data']}")
            return True
        except Exception as e:
            print(f"Error unpacking packet: {e}")
            return False
    else:
        print("No packet received.")
        return False


def create_cmd_payload(cmd_id, cmd_payload):
    return bytes([cmd_id] + list(cmd_payload.encode("utf-8")))


def send_packet_ping():
    # Create a NO_OP packet (command 0x04)
    data = create_cmd_payload(0, "")
    packet = packet_builder.create_packet(
        dst=0xFF,  # Destination address
        src=0xFF,  # Source address (ground station)
        flags=0x00,  # No special flags
        seq=0x00,  # Sequence number
        data=data,  # Command data
    )
    print(f"Packet dump:" + " ".join(f"{byte:02x}" for byte in packet))
    rfm9x.send(packet)
    print(
        f"Sent packet with message ID: {packet_builder.msg_id - 1}, {len(packet) + 4=}"
    )


def send_payload_pkt():
    data = create_cmd_payload(
        0x01, '["take_photo", ["test1"], {"w": 1024, "h": 768, "cell": 128}]'
    )
    packet = packet_builder.create_packet(dst=255, src=0, flags=0, seq=0, data=data)
    print("Packet dump:" + " ".join(f"{byte:02x}" for byte in packet))
    rfm9x.send(packet, destination=0xFF, node=0x00, flags=0x00, identifier=0x00)
    print(
        f"Sent packet with message ID: {packet_builder.msg_id - 1}, {len(packet) + 4=}"
    )


def main():
    while True:
        led.value = True
        time.sleep(0.5)
        led.value = False
        time.sleep(4.5)
        print(f"Sending packet")
        # send_packet_ping()
        send_payload_pkt()
        try_get_packet()


main()
