"""
Data models for ground station packets and telemetry.

This module provides CircuitPython-compatible data models without requiring Pydantic.
On CPython (Raspberry Pi), Pydantic validation is used if available.
On CircuitPython (Pico/Feather), simple classes are used instead.
"""

import sys
from typing import List, Optional

# Detect CircuitPython vs CPython
IS_CIRCUITPYTHON = sys.implementation.name == "circuitpython"

if not IS_CIRCUITPYTHON:
    try:
        from pydantic import BaseModel, Field

        USE_PYDANTIC = True
    except ImportError:
        USE_PYDANTIC = False
else:
    USE_PYDANTIC = False


# Base class for all models
if USE_PYDANTIC:
    _BaseModel = BaseModel
else:

    class _BaseModel:
        """Simple base model for CircuitPython compatibility"""

        def __init__(self, **kwargs):
            for key, value in kwargs.items():
                setattr(self, key, value)

        def dict(self):
            """Convert to dictionary (Pydantic-compatible method)"""
            return {k: v for k, v in self.__dict__.items() if not k.startswith("_")}


class ADCSQuaternion(_BaseModel):
    """ADCS quaternion representation"""

    if USE_PYDANTIC:
        q0: float = 0.0
        q1: float = 0.0
        q2: float = 0.0
        q3: float = 0.0
    else:

        def __init__(self, q0=0.0, q1=0.0, q2=0.0, q3=0.0, **kwargs):
            self.q0 = q0
            self.q1 = q1
            self.q2 = q2
            self.q3 = q3

    @property
    def magnitude(self) -> float:
        return (self.q0**2 + self.q1**2 + self.q2**2 + self.q3**2) ** 0.5


class ADCSData(_BaseModel):
    """ADCS telemetry data"""

    if USE_PYDANTIC:
        angular_velocity: float = 0.0
        quaternion: ADCSQuaternion = Field(default_factory=ADCSQuaternion)
        state: int = 0
        boot_count: int = 0
    else:

        def __init__(self, angular_velocity=0.0, quaternion=None, state=0, boot_count=0, **kwargs):
            self.angular_velocity = angular_velocity
            self.quaternion = quaternion if quaternion else ADCSQuaternion()
            self.state = state
            self.boot_count = boot_count


class BeaconStats(_BaseModel):
    """Beacon statistics and telemetry"""

    if USE_PYDANTIC:
        reboot_counter: int = 0
        time_in_state_ms: int = 0
        rx_bytes: int = 0
        rx_packets: int = 0
        rx_backpressure_drops: int = 0
        rx_bad_packet_drops: int = 0
        tx_bytes: int = 0
        tx_packets: int = 0
        battery_voltage: int = Field(default=0, description="mV")
        battery_current: int = Field(default=0, description="mA")
        solar_voltage: int = Field(default=0, description="mV")
        solar_current: int = Field(default=0, description="mA")
        panel_A_voltage: int = Field(default=0, description="mV")
        panel_A_current: int = Field(default=0, description="mA")
        panel_B_voltage: int = Field(default=0, description="mV")
        panel_B_current: int = Field(default=0, description="mA")
        device_status: int = 0
    else:

        def __init__(
            self,
            reboot_counter=0,
            time_in_state_ms=0,
            rx_bytes=0,
            rx_packets=0,
            rx_backpressure_drops=0,
            rx_bad_packet_drops=0,
            tx_bytes=0,
            tx_packets=0,
            battery_voltage=0,
            battery_current=0,
            solar_voltage=0,
            solar_current=0,
            panel_A_voltage=0,
            panel_A_current=0,
            panel_B_voltage=0,
            panel_B_current=0,
            device_status=0,
            **kwargs,
        ):
            self.reboot_counter = reboot_counter
            self.time_in_state_ms = time_in_state_ms
            self.rx_bytes = rx_bytes
            self.rx_packets = rx_packets
            self.rx_backpressure_drops = rx_backpressure_drops
            self.rx_bad_packet_drops = rx_bad_packet_drops
            self.tx_bytes = tx_bytes
            self.tx_packets = tx_packets
            self.battery_voltage = battery_voltage  # mV
            self.battery_current = battery_current  # mA
            self.solar_voltage = solar_voltage  # mV
            self.solar_current = solar_current  # mA
            self.panel_A_voltage = panel_A_voltage  # mV
            self.panel_A_current = panel_A_current  # mA
            self.panel_B_voltage = panel_B_voltage  # mV
            self.panel_B_current = panel_B_current  # mA
            self.device_status = device_status

    @property
    def device_status_flags(self) -> List[str]:
        status = self.device_status
        flags = []
        if status & 0x01:
            flags.append("RBF_detected")
        if status & 0x02:
            flags.append("solar_charge")
        if status & 0x04:
            flags.append("solar_fault")
        if status & 0x08:
            flags.append("panel_A_deployed")
        if status & 0x10:
            flags.append("panel_B_deployed")
        if status & 0x20:
            flags.append("payload_on")
        if status & 0x40:
            flags.append("adcs_on")
        if status & 0x80:
            flags.append("adcs_valid")
        return flags


class BeaconData(_BaseModel):
    """Beacon packet data"""

    if USE_PYDANTIC:
        state_name: str = "unknown"
        stats: Optional[BeaconStats] = None
        adcs: Optional[ADCSData] = None
        callsign: Optional[str] = None
        raw_hex: Optional[str] = None
    else:

        def __init__(
            self, state_name="unknown", stats=None, adcs=None, callsign=None, raw_hex=None, **kwargs
        ):
            self.state_name = state_name
            self.stats = stats
            self.adcs = adcs
            self.callsign = callsign
            self.raw_hex = raw_hex


class Packet(_BaseModel):
    """Base class for all satellite communication packets.

    Handles the radio-level protocol stuff like authentication and parsing
    out the data field. Specific packet types inherit from this class.
    """

    if USE_PYDANTIC:
        dst: int = 0
        src: int = 0
        flags: int = 0
        seq: int = 0
        data: bytes = b""
        boot_count: Optional[int] = None
        msg_id: Optional[int] = None
        hmac_digest: Optional[bytes] = None
    else:

        def __init__(
            self,
            dst=0,
            src=0,
            flags=0,
            seq=0,
            data=b"",
            boot_count=None,
            msg_id=None,
            hmac_digest=None,
            **kwargs,
        ):
            self.dst = dst
            self.src = src
            self.flags = flags
            self.seq = seq
            self.data = data
            self.boot_count = boot_count
            self.msg_id = msg_id
            self.hmac_digest = hmac_digest

    @property
    def header_bytes(self) -> bytes:
        import struct

        return struct.pack("BBBBB", self.dst, self.src, self.flags, self.seq, len(self.data))

    @property
    def footer_bytes(self) -> bytes:
        import struct

        if self.boot_count is not None and self.msg_id is not None:
            return struct.pack("<II", self.boot_count, self.msg_id)
        return b""

    @classmethod
    def from_raw_data(cls, raw_data: bytes):
        """Create packet instance from raw radio data"""
        # This would be implemented by subclasses
        raise NotImplementedError("Subclasses must implement from_raw_data")


class BeaconPacket(Packet):
    """Packet type that understands the data field as a beacon payload.

    This packet type knows how to parse beacon-specific data including
    telemetry stats, ADCS data, and mission state information.
    """

    if USE_PYDANTIC:
        beacon_data: Optional[BeaconData] = None
    else:

        def __init__(self, beacon_data=None, **kwargs):
            super().__init__(**kwargs)
            self.beacon_data = beacon_data

    @classmethod
    def from_raw_data(cls, raw_data: bytes, **kwargs):
        """Create BeaconPacket from raw radio data"""
        # Parse the raw packet structure first
        # Then decode the data field as beacon payload
        # Implementation would use existing protocol.decode_beacon_data logic
        pass


class AdcsTelemetryPacket(Packet):
    """Packet type that understands the data field as ADCS telemetry.

    This packet type knows how to parse ADCS-specific telemetry data
    including attitude, angular velocity, and control system status.
    """

    if USE_PYDANTIC:
        adcs_data: Optional[ADCSData] = None
    else:

        def __init__(self, adcs_data=None, **kwargs):
            super().__init__(**kwargs)
            self.adcs_data = adcs_data

    @classmethod
    def from_raw_data(cls, raw_data: bytes, **kwargs):
        """Create AdcsTelemetryPacket from raw radio data"""
        # Parse the raw packet structure first
        # Then decode the data field as ADCS telemetry
        pass


class CommandPacket(Packet):
    """Packet type for sending commands to the satellite.

    This packet type handles command encoding, authentication,
    and proper message ID sequencing.
    """

    if USE_PYDANTIC:
        command_id: int = 0
        command_payload: str = ""
    else:

        def __init__(self, command_id=0, command_payload="", **kwargs):
            super().__init__(**kwargs)
            self.command_id = command_id
            self.command_payload = command_payload

    @classmethod
    def create_command(cls, cmd_id: int, cmd_payload: str = "", **kwargs):
        """Create a properly formatted command packet"""
        # Implementation would use existing protocol.create_cmd_payload logic
        pass
