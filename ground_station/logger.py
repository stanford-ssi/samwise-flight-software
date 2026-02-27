import logging
import os
import time

# Set up standard Python logging for console and system logs
# This allows for structured logging level control (DEBUG, INFO, etc.)
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%Y-%m-%dT%H:%M:%SZ",
)
logger = logging.getLogger("GS")

# Attempt to get UTC time if possible, fallback to local monotonic
try:
    from rtc import RTC

    rtc = RTC()
except (ImportError, AttributeError):
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
                    "timestamp",
                    "callsign",
                    "state",
                    "reboots",
                    "time_in_state_ms",
                    "battery_mv",
                    "battery_ma",
                    "solar_legacy_mv",
                    "solar_legacy_ma",
                    "panel_a_mv",
                    "panel_a_ma",
                    "panel_b_mv",
                    "panel_b_ma",
                    "rx_pkts",
                    "rx_bytes",
                    "tx_pkts",
                    "tx_bytes",
                    "rx_drops_bp",
                    "rx_drops_bad",
                    "status_hex",
                    "rssi_dbm",
                    "snr",
                    "adcs_state",
                    "adcs_w",
                    "raw_hex",
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

    def log_beacon(self, beacon_data, rssi=None, snr=None, console_only=False, detailed=False):
        """
        Logs decoded telemetry data.

        Args:
            beacon_data (dict): The decoded beacon data dictionary (or Pydantic model dict).
            rssi (float, optional): Received Signal Strength Indicator.
            snr (float, optional): Signal-to-Noise Ratio.
            console_only (bool): If True, bypass the CSV file even if initialized.
            detailed (bool): If True, prints comprehensive telemetry to the console.
        """
        ts = self._get_timestamp()

        # Access model attributes directly — avoids the dict().get() chain which breaks
        # when _BaseModel.dict() doesn't recursively convert nested model objects.
        state = beacon_data.state_name
        stats = beacon_data.stats    # BeaconStats object or None
        adcs = beacon_data.adcs      # ADCSData object or None
        callsign = beacon_data.callsign or "N/A"

        # 1. Logic for Console Logging
        if self.log_to_console:
            if detailed:
                print(f"\n--- MISSION TELEMETRY REPORT ({ts}) ---")
                print(f"State: {state} | Callsign: {callsign}")
                if stats:
                    print(f"Reboots: {stats.reboot_counter}")
                    print(f"Time in State: {stats.time_in_state_ms} ms")
                    print(f"Battery: {stats.battery_voltage} mV | {stats.battery_current} mA")
                    print(
                        f"Solar: Bus {stats.solar_voltage} mV"
                        f" | A {stats.panel_A_voltage} mV"
                        f" | B {stats.panel_B_voltage} mV"
                    )
                    print(
                        f"Comms: RX {stats.rx_packets} pkts | TX {stats.tx_packets} pkts"
                    )
                    print(
                        f"Status Flags: 0x{stats.device_status:02x}"
                        f" ({', '.join(stats.device_status_flags) or 'none'})"
                    )
                if adcs:
                    print(f"ADCS: w={adcs.angular_velocity:.6f} rad/s | State: {adcs.state}")
                print(f"Signal: RSSI {rssi} dBm | SNR {snr}")
                print("---------------------------------------")
            else:
                logger.info(
                    "BEACON | State: %s | Reboot: %d | Batt: %d mV | RSSI: %s",
                    state,
                    stats.reboot_counter if stats else 0,
                    stats.battery_voltage if stats else 0,
                    str(rssi),
                )

        # 2. Logic for CSV Persistence
        if self.log_to_csv and not console_only and self.initialized and self.file_handle:
            try:
                row = [
                    ts,
                    callsign,
                    state,
                    str(stats.reboot_counter if stats else 0),
                    str(stats.time_in_state_ms if stats else 0),
                    str(stats.battery_voltage if stats else 0),
                    str(stats.battery_current if stats else 0),
                    str(stats.solar_voltage if stats else 0),
                    str(stats.solar_current if stats else 0),
                    str(stats.panel_A_voltage if stats else 0),
                    str(stats.panel_A_current if stats else 0),
                    str(stats.panel_B_voltage if stats else 0),
                    str(stats.panel_B_current if stats else 0),
                    str(stats.rx_packets if stats else 0),
                    str(stats.rx_bytes if stats else 0),
                    str(stats.tx_packets if stats else 0),
                    str(stats.tx_bytes if stats else 0),
                    str(stats.rx_backpressure_drops if stats else 0),
                    str(stats.rx_bad_packet_drops if stats else 0),
                    "0x{:02x}".format(stats.device_status if stats else 0),
                    str(rssi if rssi is not None else ""),
                    str(snr if snr is not None else ""),
                    str(adcs.state if adcs else ""),
                    "{:.6f}".format(adcs.angular_velocity) if adcs else "",
                    beacon_data.raw_hex or "",
                ]

                self.file_handle.write(",".join(row) + "\n")
                self.file_handle.flush()
            except Exception as e:
                logger.error("Telemetry CSV write error: %s", e)

    def close(self):
        """Safely close the log file handle."""
        if self.file_handle:
            try:
                self.file_handle.close()
                self.file_handle = None
                logger.info("Telemetry CSV Logger closed.")
            except (OSError, AttributeError):
                pass


# Global singleton
telemetry_logger = TelemetryLogger()
