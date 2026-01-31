import os
import time
import logging

# Set up standard Python logging for console and system logs
# This allows for structured logging level control (DEBUG, INFO, etc.)
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    datefmt='%Y-%m-%dT%H:%M:%SZ'
)
logger = logging.getLogger("GS")

# Attempt to get UTC time if possible, fallback to local monotonic
try:
    from rtc import RTC
    rtc = RTC()
except:
    rtc = None

LOG_DIR = "logs"
TELEMETRY_FILE = "{}/telemetry_log.csv".format(LOG_DIR)

class TelemetryLogger:
    """
    Handles persistent mission data logging for the Samwise Ground Station.
    
    This class manages:
    1. CSV persistence: Writing every decoded beacon to telemetry_log.csv for post-pass analysis.
    2. Console output: Structured printing to the terminal.
    3. Raw data capture: Storing hex payloads for forensic analysis.
    """
    
    def __init__(self, log_to_console=True, log_to_csv=True):
        self.initialized = False
        self.file_handle = None
        self.log_to_console = log_to_console
        self.log_to_csv = log_to_csv
        
        if self.log_to_csv:
            self._ensure_log_dir()

    def _ensure_log_dir(self):
        """Prepares the logs directory and CSV file with headers if they don't exist."""
        try:
            if not os.path.exists(LOG_DIR):
                os.mkdir(LOG_DIR)
            
            file_exists = os.path.exists(TELEMETRY_FILE)

            # Open file in append mode and keep it open for performance during a pass
            self.file_handle = open(TELEMETRY_FILE, "a")
            
            if not file_exists:
                header = [
                    "timestamp", "callsign", "state", "reboots", "time_in_state_ms",
                    "battery_mv", "battery_ma", "solar_legacy_mv", "solar_legacy_ma",
                    "panel_a_mv", "panel_a_ma", "panel_b_mv", "panel_b_ma",
                    "rx_pkts", "rx_bytes", "tx_pkts", "tx_bytes",
                    "rx_drops_bp", "rx_drops_bad", "status_hex",
                    "rssi_dbm", "snr", "adcs_state", "adcs_w", "raw_hex"
                ]
                self.file_handle.write(",".join(header) + "\n")
                self.file_handle.flush()
                
            self.initialized = True
            logger.info("Telemetry CSV Logger initialized at %s", TELEMETRY_FILE)
        except Exception as e:
            logger.error("Telemetry Logger initialization error: %s", e)

    def _get_timestamp(self):
        """Returns an ISO-8601 UTC timestamp string."""
        try:
            import datetime
            return datetime.datetime.utcnow().isoformat() + "Z"
        except ImportError:
            t = time.localtime()
            return "{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z".format(
                t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec
            )

    def log_beacon(self, beacon_data, rssi=None, snr=None, console_only=False):
        """
        Logs decoded telemetry data.
        
        Args:
            beacon_data (dict): The decoded beacon data dictionary (or Pydantic model dict).
            rssi (float, optional): Received Signal Strength Indicator.
            snr (float, optional): Signal-to-Noise Ratio.
            console_only (bool): If True, bypass the CSV file even if initialized.
        """
        ts = self._get_timestamp()
        stats = beacon_data.get('stats', {})
        adcs = beacon_data.get('adcs', {})
        
        # 1. Logic for Console Logging (using standard logging lib)
        if self.log_to_console:
            state = beacon_data.get('state_name', 'UNKNOWN')
            reboots = stats.get('reboot_counter', 0)
            batt = stats.get('battery_voltage', 0)
            logger.info("BEACON | State: %s | Reboots: %d | Battery: %d mV | Signal: RSSI %s, SNR %s", 
                        state, reboots, batt, str(rssi), str(snr))

        # 2. Logic for CSV Persistence
        if self.log_to_csv and not console_only and self.initialized and self.file_handle:
            try:
                row = [
                    ts,
                    str(beacon_data.get('callsign', 'N/A')),
                    str(beacon_data.get('state_name', 'N/A')),
                    str(stats.get('reboot_counter', 0)),
                    str(stats.get('time_in_state_ms', 0)),
                    str(stats.get('battery_voltage', 0)),
                    str(stats.get('battery_current', 0)),
                    str(stats.get('solar_voltage', 0)),
                    str(stats.get('solar_current', 0)),
                    str(stats.get('panel_A_voltage', 0)),
                    str(stats.get('panel_A_current', 0)),
                    str(stats.get('panel_B_voltage', 0)),
                    str(stats.get('panel_B_current', 0)),
                    str(stats.get('rx_packets', 0)),
                    str(stats.get('rx_bytes', 0)),
                    str(stats.get('tx_packets', 0)),
                    str(stats.get('tx_bytes', 0)),
                    str(stats.get('rx_backpressure_drops', 0)),
                    str(stats.get('rx_bad_packet_drops', 0)),
                    "0x{:02x}".format(stats.get('device_status', 0)),
                    str(rssi if rssi is not None else ""),
                    str(snr if snr is not None else ""),
                    str(adcs.get('state', "")),
                    "{:.6f}".format(adcs.get('angular_velocity', 0)) if adcs else "",
                    beacon_data.get('raw_hex', '')
                ]
                
                self.file_handle.write(",".join(row) + "\n")
                self.file_handle.flush() # Ensure it's on disk immediately during a pass
            except Exception as e:
                logger.error("Telemetry CSV write error: %s", e)

    def close(self):
        """Safely close the log file handle."""
        if self.file_handle:
            try:
                self.file_handle.close()
                self.file_handle = None
                logger.info("Telemetry CSV Logger closed.")
            except:
                pass

# Global singleton
telemetry_logger = TelemetryLogger()
