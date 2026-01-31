import sys
import time
import select
from .radio_commands import get_radio
from . import protocol
from . import config
from . import hardware
from .state import state_manager

def get_user_input(prompt, default=None):
    """Get user input with optional default value"""
    if default is not None:
        full_prompt = f"{prompt} [{default}]: "
    else:
        full_prompt = f"{prompt}: "
    
    try:
        response = input(full_prompt).strip()
        if not response and default is not None:
            return str(default)
        return response
    except KeyboardInterrupt:
        print("\nExiting...")
        sys.exit()

def get_yes_no(prompt, default=True):
    """Get yes/no input from user"""
    default_str = "Y/n" if default else "y/N"
    response = get_user_input(f"{prompt} ({default_str})", "y" if default else "n")
    return response.lower() in ['y', 'yes', '1', 'true']

def get_user_input_with_timeout(prompt, timeout=0.5):
    """Get user input with timeout, checking for packets while waiting"""
    
    print(prompt, end='', flush=True)
    start_time = time.monotonic()
    user_input = ""
    
    while True:
        # Check for packets every 100ms
        radio = get_radio()
        if radio.try_get_packet(timeout=0.1):
            # Packet received, redisplay prompt
            print(f"\r{prompt}", end='', flush=True)
        
        # Simple timeout mechanism (CircuitPython doesn't have select)
        if time.monotonic() - start_time > timeout:
            break
            
        time.sleep(0.1)
    
    return None

def configure_authentication():
    """Interactive authentication configuration"""
    print("\n=== Authentication Configuration ===")
    config.config['auth_enabled'] = get_yes_no("Enable HMAC authentication?", True)
    
    if config.config['auth_enabled']:
        new_psk = get_user_input("Enter PSK (leave blank for default)", None)
        if new_psk:
            config.config['packet_hmac_psk'] = new_psk.encode('utf-8')
        
        boot_count_str = get_user_input("Enter boot count", config.config['boot_count'])
        try:
            config.config['boot_count'] = int(boot_count_str)
        except ValueError:
            print("Invalid boot count, using default")
    else:
        print("Authentication disabled - packets will not be authenticated")

def configure_lora_settings():
    """Interactive LoRA configuration"""
    print("\n=== LoRA Configuration ===")
    print(f"Current settings match flight software defaults:")
    print(f"  Frequency: {config.config['frequency']} MHz")
    print(f"  Bandwidth: {config.config['bandwidth']} Hz")
    print(f"  Spreading Factor: {config.config['spreading_factor']}")
    print(f"  Coding Rate: {config.config['coding_rate']}")
    print(f"  CRC: {config.config['crc']}")
    print(
        """
        WARNING: Command mode requires valid authentication settings (PSK and Boot Count).
        The system will attempt to sync Boot Count automatically from received beacons.
        """
        )
    
    if get_yes_no("Modify LoRA settings?", False):
        freq_str = get_user_input("Frequency (MHz)", config.config['frequency'])
        try:
            config.config['frequency'] = float(freq_str)
        except ValueError:
            print("Invalid frequency, using default")
        
        bw_str = get_user_input("Bandwidth (Hz)", config.config['bandwidth'])
        try:
            config.config['bandwidth'] = int(bw_str)
        except ValueError:
            print("Invalid bandwidth, using default")
        
        sf_str = get_user_input("Spreading Factor (6-12)", config.config['spreading_factor'])
        try:
            sf = int(sf_str)
            if 6 <= sf <= 12:
                config.config['spreading_factor'] = sf
            else:
                print("Spreading factor must be 6-12, using default")
        except ValueError:
            print("Invalid spreading factor, using default")
        
        cr_str = get_user_input("Coding Rate (5-8)", config.config['coding_rate'])
        try:
            cr = int(cr_str)
            if 5 <= cr <= 8:
                config.config['coding_rate'] = cr
            else:
                print("Coding rate must be 5-8, using default")
        except ValueError:
            print("Invalid coding rate, using default")
        
        config.config['crc'] = get_yes_no("Enable CRC?", config.config['crc'])

def show_command_menu():
    """Display available commands"""
    print("\n=== Available Commands ===")
    print("1. Send NO_OP (ping)")
    print("2. Send Payload Execute")
    print("3. Send Payload Turn On")
    print("4. Send Payload Turn Off")
    print("5. Send Manual State Override")
    print("6. Send Payload Shutdown")
    print("r. Check for received packets")
    print("q. Quit")
    print("h. Show this help")

def interactive_command_loop():
    """Main interactive command loop with non-blocking packet monitoring"""
    show_command_menu()
    print("\nSystem is monitoring for packets. Press Enter to type a command.")
    
    last_status_time = time.monotonic()
    status_interval = 10.0  # Show status every 10 seconds
    
    while True:
        try:
            # 1. Continuously check for packets (most important)
            radio = get_radio()
            radio.try_get_packet(timeout=0.01)
            
            # 2. Check for user input without blocking
            # Linux/Pi compatible non-blocking stdin check
            rlist, _, _ = select.select([sys.stdin], [], [], 0)
            if rlist:
                line = sys.stdin.readline()
                cmd = line.strip().lower()
                
                if cmd == 'q':
                    print("Goodbye!")
                    state_manager.shutdown()
                    break
                elif cmd == 'h':
                    show_command_menu()
                elif cmd == '1':
                    print("Sending NO_OP...")
                    radio.send_no_op()
                elif cmd == '2':
                    payload_cmd = str(input("Enter payload command: ") or '["take_photo"]')
                    radio.send_payload_exec(payload_cmd)
                elif cmd == '3':
                    radio.send_payload_turn_on()
                elif cmd == '4':
                    radio.send_payload_turn_off()
                elif cmd == '5':
                    state_name = str(input("Enter state name: ") or "running_state")
                    radio.send_manual_state_override(state_name)
                elif cmd == '6':
                    print("Sending payload shutdown...")
                    radio.send_payload_shutdown()
                elif cmd == '':
                    # Pressed enter, show status and prompt
                    print(f"\n[BOOT:{state_manager.boot_count} MSG:{state_manager.msg_id}] Command (1-6, q, h): ", end="", flush=True)
                else:
                    print(f"Unknown command '{cmd}'. Type 'h' for help.")
            
            # 3. Periodic status line to show we're alive
            current_time = time.monotonic()
            if current_time - last_status_time > status_interval:
                print(f"\r[STATUS] Monitoring... Boot:{state_manager.boot_count} MsgID:{state_manager.msg_id}   ", end="", flush=True)
                last_status_time = current_time
                
            time.sleep(0.01) # Low latency
            
        except KeyboardInterrupt:
            print("\nGoodbye!")
            state_manager.shutdown()
            break
        except Exception as e:
            print(f"Error: {e}")
            time.sleep(0.1)

def debug_listen_mode():
    """Simple debug mode - just listen and report any packets"""
    print("\n=== DEBUG LISTENER MODE ===")
    print("Listening for ANY packets and attempting beacon decode...")
    print("Press Ctrl+C to stop\n")
    
    packet_count = 0
    
    # Try to import datetime for UTC timestamps

    try:
        from datetime import datetime
    except ImportError:
        datetime = None

    start_time = time.monotonic()
    last_status_time = start_time

    try:
        while True:
            # Blink LED to show we're alive
            if hardware.led is not None:
                hardware.led.value = True
            
            # Check for packets - radio handles decoding and printing
            radio = get_radio()
            if radio.try_get_packet(timeout=0.1):
                packet_count += 1
                
                # Get wall clock time
                if datetime:
                    ts = datetime.utcnow().strftime('%H:%M:%S')
                else:
                    ts = "{:.1f}s".format(time.monotonic() - start_time)
                
                print(f"\n*** PACKET #{packet_count} at {ts} UTC ***")
                
                # Show signal strength
                try:
                    rssi = hardware.rfm9x.last_rssi
                    snr = getattr(hardware.rfm9x, 'last_snr', None)
                    if snr is not None:
                        print(f"Signal: RSSI {rssi} dBm, SNR {snr}")
                    else:
                        print(f"Signal: RSSI {rssi} dBm")
                except:
                    pass
                print("*** END PACKET ***\n")
            
            if hardware.led is not None: 
                hardware.led.value = False
            
            # Periodic status
            current_time = time.monotonic()
            if current_time - last_status_time > 30:
                print("[{:.0f}s] Listening... ({} packets so far)".format(current_time - start_time, packet_count))
                last_status_time = current_time
            
            time.sleep(0.1)
                
    except KeyboardInterrupt:
        print("\nExiting listener mode...")
        state_manager.shutdown()
        print("\n=== SUMMARY ===")
        print("Total packets: {}".format(packet_count))
        print("Returning to main menu...")
