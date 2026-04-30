from typing import List, Optional

from . import USE_PYDANTIC, _BaseModel
from .adcs_model import ADCSData

if USE_PYDANTIC:
    from pydantic import Field


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
