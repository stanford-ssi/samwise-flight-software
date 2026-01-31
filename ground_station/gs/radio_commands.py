from . import hardware
from . import protocol
from . import config
from .state import state_manager
from .logger import telemetry_logger, logger
import time

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
                rh_destination = getattr(self.radio, 'destination', None)
                rh_node = getattr(self.radio, 'node', None) 
                rh_identifier = getattr(self.radio, 'identifier', None)
                rh_flags = getattr(self.radio, 'flags', None)
                
                logger.info("PACKET RECEIVED | TO: %s | FROM: %s | ID: %s | FLAGS: %s", 
                            rh_destination, rh_node, rh_identifier, rh_flags)
                
                # Check if this is from satellite (beacon source)
                if rh_node == 0:  # FROM satellite
                    print(f"  Type: BEACON PACKET")
                    
                    if len(packet) >= 1:
                        data_len = packet[0]
                        
                        if len(packet) >= 1 + data_len:
                            beacon_payload = packet[1:1+data_len]
                            beacon_data = protocol.decode_beacon_data(beacon_payload)
                            beacon_data.raw_hex = packet.hex() if hasattr(packet, 'hex') else ''.join('{:02x}'.format(b) for b in packet)
                            
                            print(f"  State: {beacon_data.state_name}")
                            
                            if beacon_data.stats:
                                stats = beacon_data.stats
                                print(f"  === Telemetry ===")
                                print(f"Reboots: {stats.reboot_counter}")
                                
                                # Sync our local state with satellite's boot count
                                state_manager.update_from_beacon(stats.reboot_counter)
                                
                                print(f"Time in State: {stats.time_in_state_ms} ms ({stats.time_in_state_ms/1000:.1f} sec)")
                                print(f"RX: {stats.rx_packets} pkts, TX: {stats.tx_packets} pkts")
                                print(f"Battery: {stats.battery_voltage} mV, {stats.battery_current} mA")
                                
                                # Use Pydantic helper for status flags
                                status_flags = stats.device_status_flags
                                print(f"    Status: 0x{stats.device_status:02x} ({', '.join(status_flags) if status_flags else 'none'})")
                                
                                if beacon_data.adcs:
                                    adcs = beacon_data.adcs
                                    print(f"  === ADCS Telemetry ===")
                                    print(f"Angular Velocity: {adcs.angular_velocity:.6f} rad/s")
                                    print(f"Quaternion Mag: {adcs.quaternion.magnitude:.6f}")
                                
                                # Log to persistent storage
                                try:
                                    rssi = getattr(self.radio, 'last_rssi', None)
                                    snr = getattr(self.radio, 'last_snr', None)
                                    telemetry_logger.log_beacon(beacon_data.dict(), rssi=rssi, snr=snr)
                                except Exception as log_err:
                                    logger.error("Failed to log beacon: %s", log_err)

                            else:
                                logger.warning("Beacon decode failed")
                else:
                    print(f"Type: RESPONSE from node {rh_node}")
                    try:
                        unpacked = protocol.Packet.unpack(packet)
                        print(f"  Data: {unpacked.data}")
                    except:
                        print(f"  Raw Data: {packet[:20]}")
                
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
        self.radio.send(
            packet_payload,
            destination=dst,
            node=0x00,
            identifier=0x00,
            flags=0x00
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
