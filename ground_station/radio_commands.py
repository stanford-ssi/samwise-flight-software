import struct

import config
import protocol
import radio_initialization as hardware
from logger import logger, telemetry_logger
from state import state_manager


class LoraRadio:
    """
    Object-oriented interface for the RFM9x LoRA radio.
    Encapsulates both low-level hardware access and high-level mission commands.
    """

    def __init__(self, rfm9x_instance):
        self.radio = rfm9x_instance

    def try_get_packet(self, timeout=0.1):
        """Check for incoming packets with short timeout"""
        if self.radio is None:
            return False

        packet = self.radio.receive(timeout=timeout)
        if packet is not None:
            try:
                # Get RadioHead header fields
                rh_destination = getattr(self.radio, "destination", None)
                rh_node = getattr(self.radio, "node", None)
                rh_identifier = getattr(self.radio, "identifier", None)
                rh_flags = getattr(self.radio, "flags", None)

                # Get signal strength for filtering
                rssi = getattr(self.radio, "last_rssi", None)
                snr = getattr(self.radio, "last_snr", None)

                logger.info(
                    "PACKET RECEIVED | TO: %s | FROM: %s | ID: %s | FLAGS: %s | RSSI: %s dBm",
                    rh_destination,
                    rh_node,
                    rh_identifier,
                    rh_flags,
                    rssi,
                )

                # FILTER 1: RSSI threshold check (reject weak/noisy signals)
                if config.config.get("enable_rssi_filter", False) and rssi is not None:
                    rssi_threshold = config.config.get("rssi_threshold", -120)
                    if rssi < rssi_threshold:
                        logger.warning(
                            "PACKET DROPPED | RSSI too low: %d dBm < %d dBm threshold",
                            rssi,
                            rssi_threshold,
                        )
                        return False

                # Check if this is from satellite (beacon source)
                if rh_node == 0:  # FROM satellite
                    if len(packet) >= 1:
                        data_len = packet[0]
                        if len(packet) >= 1 + data_len:
                            beacon_payload = packet[1 : 1 + data_len]
                            beacon_data = protocol.decode_beacon_data(beacon_payload)
                            beacon_data.raw_hex = (
                                packet.hex()
                                if hasattr(packet, "hex")
                                else "".join("{:02x}".format(b) for b in packet)
                            )

                            # FILTER 2: Callsign verification (reject packets not from our satellite)
                            if config.config.get("enable_callsign_filter", False):
                                expected_callsign = config.config.get("expected_callsign", "KC3WNY")

                                # Check if beacon has callsign field
                                if beacon_data.callsign:
                                    # Verify callsign matches (case-insensitive, strip whitespace)
                                    actual_callsign = beacon_data.callsign.strip().upper()
                                    if expected_callsign.upper() not in actual_callsign:
                                        logger.warning(
                                            "PACKET DROPPED | Callsign mismatch: '%s' (expected '%s')",
                                            beacon_data.callsign,
                                            expected_callsign,
                                        )
                                        return False
                                    else:
                                        logger.debug("Callsign verified: %s", beacon_data.callsign)
                                else:
                                    # No callsign in packet - log warning but allow (might be old format)
                                    logger.warning(
                                        "PACKET WARNING | No callsign present (expected '%s')",
                                        expected_callsign,
                                    )

                            # Packet passed all filters - process it

                            # 1. Sync local mission state (Critical operational step)
                            if beacon_data.stats:
                                state_manager.update_from_beacon(beacon_data.stats.reboot_counter)

                            # 2. Comprehensive Logging & Reporting
                            # This handles both CSV storage and the detailed console report
                            try:
                                telemetry_logger.log_beacon(
                                    beacon_data, rssi=rssi, snr=snr, detailed=True
                                )
                            except Exception as log_err:
                                logger.error("Failed to process beacon telemetry: %s", log_err)
                        else:
                            logger.warning(
                                "Truncated beacon payload: expected %d bytes, got %d",
                                data_len,
                                len(packet) - 1,
                            )
                    else:
                        logger.warning("Empty packet from satellite")
                else:
                    # Non-beacon traffic (command responses, etc.)
                    logger.info("RESPONSE | FROM: %d", rh_node)
                    try:
                        unpacked = protocol.Packet.unpack(packet)
                        print(f"  Data: {unpacked.data}")
                    except (struct.error, ValueError, AttributeError):
                        logger.debug("Raw Payload: %s", packet.hex())

                print(">>> END PACKET <<<\n")
                return True
            except Exception as e:
                logger.error("ERROR processing packet: %s", e)
                return False
        return False

    def send_command(self, cmd_id, cmd_payload="", dst=0xFF):
        """Send a signed command packet to the flight software"""
        if self.radio is None:
            logger.error("Radio not available for sending")
            return

        data = protocol.create_cmd_payload(cmd_id, cmd_payload)

        # Create authenticated packet payload
        packet_payload = protocol.Packet.create(
            dst=dst,
            src=0xFF,
            flags=0x00,
            seq=0x00,
            data=data,
        )

        # Send using low-level library
        # The Adafruit RFM9x library prepends a 4-byte RadioHead header
        # (destination, node, identifier, flags) to every packet. Map the
        # first 4 bytes of the samwise packet (dst, src, flags, seq) into
        # those RadioHead fields so the flight code sees a contiguous packet.
        self.radio.send(
            packet_payload[4:],
            destination=packet_payload[0],
            node=packet_payload[1],
            identifier=packet_payload[2],
            flags=packet_payload[3],
        )

        logger.info("COMMAND SENT | ID: %d | Payload: %s", cmd_id, cmd_payload)

    # --- High-level command abstractions ---

    def send_no_op(self):
        """Send a NO_OP ping command"""
        self.send_command(config.NO_OP)

    def send_payload_exec(self, command_str):
        """Send a payload execution command"""
        self.send_command(config.PAYLOAD_EXEC, command_str)

    def send_payload_turn_on(self):
        """Send payload turn on command"""
        self.send_command(config.PAYLOAD_TURN_ON)

    def send_payload_turn_off(self):
        """Send payload turn off command"""
        self.send_command(config.PAYLOAD_TURN_OFF)

    def send_payload_shutdown(self):
        """Send payload shutdown command"""
        self.send_command(config.PAYLOAD_SHUTDOWN)

    def send_manual_state_override(self, state_name):
        """Send manual state override command"""
        self.send_command(config.MANUAL_STATE_OVERRIDE, state_name)


# Singleton management handled during initialization
radio = None


def get_radio():
    global radio
    if radio is None:
        rfm9x = hardware.initialize()
        radio = LoraRadio(rfm9x)
    return radio
