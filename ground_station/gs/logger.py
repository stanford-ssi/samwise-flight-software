import os
import time

# Attempt to get UTC time if possible, fallback to local monotonic
try:
    from rtc import RTC
    rtc = RTC()
except:
    rtc = None

LOG_DIR = "logs"
TELEMETRY_FILE = "{}/telemetry_log.csv".format(LOG_DIR)

class TelemetryLogger:
    def __init__(self):
        self.initialized = False
        self.file_handle = None
        self._ensure_log_dir()

    def _ensure_log_dir(self):
        try:
            try:
                os.mkdir(LOG_DIR)
            except OSError:
                pass
            
            file_exists = False
            try:
                os.stat(TELEMETRY_FILE)
                file_exists = True
            except OSError:
                file_exists = False

            # Open file in append mode and keep it open
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
        except Exception as e:
            print("[Logger] Initialization error: {}".format(e))

    def _get_timestamp(self):
        """Get a UTC-style timestamp string"""
        try:
            import datetime
            return datetime.datetime.utcnow().isoformat() + "Z"
        except ImportError:
            t = time.localtime()
            return "{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z".format(
                t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec
            )

    def log_beacon(self, beacon_data, rssi=None, snr=None):
        """Log decoded beacon data to CSV"""
        if not self.initialized or not self.file_handle:
            return

        try:
            ts = self._get_timestamp()
            stats = beacon_data.get('stats', {})
            adcs = beacon_data.get('adcs', {})
            
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
            print("[Logger] Logging error: {}".format(e))

    def close(self):
        """Safely close the log file"""
        if self.file_handle:
            try:
                self.file_handle.close()
                self.file_handle = None
            except:
                pass

# Global singleton
telemetry_logger = TelemetryLogger()
