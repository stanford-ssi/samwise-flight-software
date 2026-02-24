from pydantic import BaseModel, Field, validator
from typing import Optional, List
from abc import ABC

class ADCSQuaternion(BaseModel):
    q0: float
    q1: float
    q2: float
    q3: float

    @property
    def magnitude(self) -> float:
        return (self.q0**2 + self.q1**2 + self.q2**2 + self.q3**2)**0.5

class ADCSData(BaseModel):
    angular_velocity: float
    quaternion: ADCSQuaternion
    state: int
    boot_count: int

class BeaconStats(BaseModel):
    reboot_counter: int
    time_in_state_ms: int
    rx_bytes: int
    rx_packets: int
    rx_backpressure_drops: int
    rx_bad_packet_drops: int
    tx_bytes: int
    tx_packets: int
    battery_voltage: int = Field(..., description="mV")
    battery_current: int = Field(..., description="mA")
    solar_voltage: int = Field(..., description="mV")
    solar_current: int = Field(..., description="mA")
    panel_A_voltage: int = Field(..., description="mV")
    panel_A_current: int = Field(..., description="mA")
    panel_B_voltage: int = Field(..., description="mV")
    panel_B_current: int = Field(..., description="mA")
    device_status: int

    @property
    def device_status_flags(self) -> List[str]:
        status = self.device_status
        flags = []
        if status & 0x01: flags.append("RBF_detected")
        if status & 0x02: flags.append("solar_charge")
        if status & 0x04: flags.append("solar_fault")
        if status & 0x08: flags.append("panel_A_deployed")
        if status & 0x10: flags.append("panel_B_deployed")
        if status & 0x20: flags.append("payload_on")
        if status & 0x40: flags.append("adcs_on")
        if status & 0x80: flags.append("adcs_valid")
        return flags

class BeaconData(BaseModel):
    state_name: str
    stats: Optional[BeaconStats] = None
    adcs: Optional[ADCSData] = None
    callsign: Optional[str] = None
    raw_hex: Optional[str] = None

class Packet(BaseModel, ABC):
    """Base class for all satellite communication packets.
    
    Handles the radio-level protocol stuff like authentication and parsing
    out the data field. Specific packet types inherit from this class.
    """
    dst: int
    src: int
    flags: int
    seq: int
    data: bytes
    boot_count: Optional[int] = None
    msg_id: Optional[int] = None
    hmac_digest: Optional[bytes] = None

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
    beacon_data: Optional[BeaconData] = None
    
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
    adcs_data: Optional[ADCSData] = None
    
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
    command_id: int
    command_payload: str = ""
    
    @classmethod
    def create_command(cls, cmd_id: int, cmd_payload: str = "", **kwargs):
        """Create a properly formatted command packet"""
        # Implementation would use existing protocol.create_cmd_payload logic
        pass
