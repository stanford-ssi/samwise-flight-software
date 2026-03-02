import struct
import sys
from typing import Optional

import config
from models import ADCSData, ADCSQuaternion, BeaconData, BeaconStats
from models import Packet as ModelPacket
from state import state_manager

# On CPython (Raspberry Pi) always use stdlib hmac/hashlib — circuitpython_hmac has a
# name-mangling bug (_HMAC__translate) that causes NameError on Python 3.
# On CircuitPython, stdlib hmac is unavailable so we fall back to the CircuitPython libs.
_IS_CIRCUITPYTHON = sys.implementation.name == "circuitpython"
if _IS_CIRCUITPYTHON:
    import circuitpython_hmac as hmac
    from adafruit_hashlib import sha256
else:
    import hmac
    from hashlib import sha256


def create_cmd_payload(cmd_id, cmd_payload=""):
    if isinstance(cmd_payload, str):
        return bytes([cmd_id] + list(cmd_payload.encode("utf-8")))
    else:
        return bytes([cmd_id]) + cmd_payload


def safe_compare_digest(a, b):
    """Constant time comparison to avoid timing attacks"""
    if len(a) != len(b):
        return False
    result = 0
    for x, y in zip(a, b):
        result |= x ^ y
    return result == 0


class Packet:
    """Handles the standard radio protocol envelope: Headers, Auth, and Footers."""

    @staticmethod
    def create(dst, src, flags, seq, data) -> bytes:
        """Create a full authenticated packet."""
        if len(data) > config.PACKET_MAX_DATA_SIZE:
            raise ValueError(f"Data too large: {len(data)} bytes")

        # Create model representing the unsaved packet
        pkt = ModelPacket(
            dst=dst,
            src=src,
            flags=flags,
            seq=seq,
            data=data,
            boot_count=state_manager.boot_count,
            msg_id=state_manager.get_next_msg_id(),
        )

        # Build the payload for HMAC
        payload = pkt.header_bytes + pkt.data + pkt.footer_bytes

        # Add HMAC
        h = hmac.new(config.config["packet_hmac_psk"], msg=payload, digestmod=sha256)
        return payload + h.digest()

    @staticmethod
    def unpack(packet_bytes: bytes) -> ModelPacket:
        """Unpack raw bytes from an incoming packet into a Packet model.

        Note: HMAC is NOT verified here. Incoming packets from the satellite
        are not HMAC-signed. HMAC is only computed and appended by the ground
        station to outgoing command packets, so the satellite can verify them.
        """
        if len(packet_bytes) < config.PACKET_HEADER_SIZE:
            raise ValueError("Packet too short")

        # Unpack Header: dst, src, flags, seq, data_len
        dst, src, flags, seq, data_len = struct.unpack(
            "BBBBB", packet_bytes[: config.PACKET_HEADER_SIZE]
        )

        # Extract data field using length from header
        data_end = config.PACKET_HEADER_SIZE + data_len
        data = packet_bytes[config.PACKET_HEADER_SIZE : data_end]

        return ModelPacket(dst=dst, src=src, flags=flags, seq=seq, data=data)


class BeaconPacket(Packet):
    """Specialized packet for satellite beacons."""

    @classmethod
    def decode(cls, packet_bytes: bytes) -> BeaconData:
        # Standard beacon packets from radio have a 1-byte length header prefix
        # before the state name null-terminated string.
        if len(packet_bytes) < 1:
            return BeaconData(state_name="short_packet")

        data_len = packet_bytes[0]
        payload = packet_bytes[1 : 1 + data_len]

        # Raw hex for forensic logging
        raw_hex = packet_bytes.hex()

        # Find null terminator for state name
        null_pos = payload.find(b"\x00")
        if null_pos == -1:
            return BeaconData(state_name=payload.decode("utf-8", "ignore"), raw_hex=raw_hex)

        state_name = payload[:null_pos].decode("utf-8", "ignore")
        stats_start = null_pos + 1

        beacon_data = BeaconData(state_name=state_name, raw_hex=raw_hex)

        # 1. Decode Stats (53 bytes)
        if len(payload) >= stats_start + 53:
            stats_data = payload[stats_start : stats_start + 53]
            unpacked = struct.unpack("<LQ6L8HB", stats_data)
            beacon_data.stats = BeaconStats(
                reboot_counter=unpacked[0],
                time_in_state_ms=unpacked[1],
                rx_bytes=unpacked[2],
                rx_packets=unpacked[3],
                rx_backpressure_drops=unpacked[4],
                rx_bad_packet_drops=unpacked[5],
                tx_bytes=unpacked[6],
                tx_packets=unpacked[7],
                battery_voltage=unpacked[8],
                battery_current=unpacked[9],
                solar_voltage=unpacked[10],
                solar_current=unpacked[11],
                panel_A_voltage=unpacked[12],
                panel_A_current=unpacked[13],
                panel_B_voltage=unpacked[14],
                panel_B_current=unpacked[15],
                device_status=unpacked[16],
            )

            # 2. Decode ADCS if present (appended after stats)
            adcs_start = stats_start + 53
            if len(payload) >= adcs_start + 25:
                beacon_data.adcs = AdcsTelemetryPacket.decode_payload(
                    payload[adcs_start : adcs_start + 25]
                )

            # 3. Decode Callsign if present
            callsign_start = adcs_start + 25
            if len(payload) >= callsign_start + 6:
                beacon_data.callsign = (
                    payload[callsign_start : callsign_start + 6]
                    .decode("utf-8", "ignore")
                    .strip("\x00")
                )

        return beacon_data


class AdcsTelemetryPacket(Packet):
    """Specialized packet for ADCS telemetry."""

    @staticmethod
    def decode_payload(data: bytes) -> Optional[ADCSData]:
        if len(data) < 25:
            return None
        try:
            unpacked = struct.unpack("<fffffBL", data[:25])
            return ADCSData(
                angular_velocity=unpacked[0],
                quaternion=ADCSQuaternion(
                    q0=unpacked[1], q1=unpacked[2], q2=unpacked[3], q3=unpacked[4]
                ),
                state=unpacked[5],
                boot_count=unpacked[6],
            )
        except (struct.error, ValueError, TypeError):
            return None


# Backward compatibility wrappers
def decode_beacon_data(data):
    return BeaconPacket.decode(data)


def decode_adcs_data(data):
    return AdcsTelemetryPacket.decode_payload(data)


class LegacyPacketBuilder:
    def create_packet(self, dst, src, flags, seq, data):
        return Packet.create(dst, src, flags, seq, data)

    def unpack_packet(self, packet_bytes):
        pkt = Packet.unpack(packet_bytes)
        return pkt.dict()


packet_builder = LegacyPacketBuilder()
