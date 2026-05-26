"""
RPi Payload Beacon
Periodic 2400 MHz telemetry transmitter. Runs as a standalone systemd service,
independent from main.py.

Packet layout (29 bytes, zero-padded to 253 for the LoRa fixed-length config):
  [0:8]   identifier  b"RPIBC\x00\x00\x00"
  [8:12]  boot_count  uint32 LE
  [12:16] uptime_s    uint32 LE
  [16:18] cpu_temp    int16 LE  (degrees C * 10, e.g. 453 = 45.3 C)
  [18]    cpu_pct     uint8  (0-100)
  [19]    mem_pct     uint8  (0-100)
  [20]    disk_pct    uint8  (0-100)
  [21:29] callsign    b"KC3WNY\x00\x00"
"""

import json
import logging
import os
import struct
import subprocess
import sys
import time
from pathlib import Path

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------
BEACON_INTERVAL_S = 30
S_BAND_FREQ       = 2400                               # MHz
RADIO_BEACON_EXE  = "/home/pi/code/radio/build/Lora_beacon"
STATUS_FILE       = "/home/pi/code/status.json"
LOG_FILE          = "/home/pi/logs/beacon.log"
RADIO_ENABLE_PIN  = 27                                 # BCM GPIO

IDENTIFIER = b"RPIBC\x00\x00\x00"   # 8 bytes — identifies RPi beacon packets
CALLSIGN   = b"KC3WNY\x00\x00"      # 8 bytes

# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------
def _setup_logging():
    Path("/home/pi/logs").mkdir(parents=True, exist_ok=True)
    fmt = logging.Formatter("%(asctime)s %(levelname)s %(message)s")
    fh  = logging.FileHandler(LOG_FILE)
    fh.setFormatter(fmt)
    sh  = logging.StreamHandler(sys.stdout)
    sh.setFormatter(fmt)
    logging.basicConfig(level=logging.DEBUG, handlers=[fh, sh], force=True)

log = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# GPIO helpers
# ---------------------------------------------------------------------------
def _gpio_setup():
    import RPi.GPIO as gpio
    gpio.setmode(gpio.BCM)
    gpio.setwarnings(False)
    gpio.setup(RADIO_ENABLE_PIN, gpio.OUT)
    gpio.output(RADIO_ENABLE_PIN, 0)

def _radio_power(on: bool):
    import RPi.GPIO as gpio
    gpio.output(RADIO_ENABLE_PIN, 1 if on else 0)

# ---------------------------------------------------------------------------
# Telemetry collection
# ---------------------------------------------------------------------------
def _boot_count() -> int:
    try:
        with open(STATUS_FILE) as f:
            return int(json.load(f).get("boot_count", 0))
    except Exception:
        return 0

def _uptime_s() -> int:
    try:
        with open("/proc/uptime") as f:
            return int(float(f.readline().split()[0]))
    except Exception:
        return 0

def _cpu_temp_c10() -> int:
    try:
        raw = os.popen("vcgencmd measure_temp").readline()
        return int(float(raw.removeprefix("temp=").removesuffix("'C\n")) * 10)
    except Exception:
        return 0

def _cpu_pct() -> int:
    try:
        with open("/proc/stat") as f:
            parts = f.readline().split()
        user, nice, sys_, idle = int(parts[1]), int(parts[2]), int(parts[3]), int(parts[4])
        total = user + nice + sys_ + idle
        return int((user + nice + sys_) * 100 // total) if total > 0 else 0
    except Exception:
        return 0

def _mem_pct() -> int:
    try:
        info = {}
        with open("/proc/meminfo") as f:
            for line in f:
                k, v = line.split(":")
                info[k.strip()] = int(v.split()[0])
                if "MemAvailable" in info and "MemTotal" in info:
                    break
        total = info.get("MemTotal", 0)
        avail = info.get("MemAvailable", 0)
        return int((total - avail) * 100 // total) if total > 0 else 0
    except Exception:
        return 0

def _disk_pct() -> int:
    try:
        st = os.statvfs("/home/pi")
        total = st.f_blocks * st.f_frsize
        used  = (st.f_blocks - st.f_bfree) * st.f_frsize
        return int(used * 100 // total) if total > 0 else 0
    except Exception:
        return 0

def collect_telemetry() -> dict:
    return {
        "boot_count": _boot_count(),
        "uptime_s":   _uptime_s(),
        "cpu_temp":   _cpu_temp_c10(),
        "cpu_pct":    _cpu_pct(),
        "mem_pct":    _mem_pct(),
        "disk_pct":   _disk_pct(),
    }

# ---------------------------------------------------------------------------
# Packet builder
# ---------------------------------------------------------------------------
def build_packet(t: dict) -> bytes:
    stats = struct.pack(
        "<IIhBBB",
        t["boot_count"],
        t["uptime_s"],
        t["cpu_temp"],
        t["cpu_pct"],
        t["mem_pct"],
        t["disk_pct"],
    )
    return IDENTIFIER + stats + CALLSIGN   # 29 bytes total

# ---------------------------------------------------------------------------
# Transmit
# ---------------------------------------------------------------------------
def transmit(packet: bytes):
    _radio_power(True)
    try:
        result = subprocess.run(
            ["sudo", RADIO_BEACON_EXE, str(S_BAND_FREQ)],
            input=packet,
            timeout=30,
            capture_output=True,
        )
        if result.returncode != 0:
            log.error("Lora_beacon failed (rc=%d): %s",
                      result.returncode, result.stderr.decode(errors="replace"))
        else:
            log.debug("Lora_beacon stdout: %s", result.stdout.decode(errors="replace").strip())
    except subprocess.TimeoutExpired:
        log.error("Lora_beacon timed out")
    except FileNotFoundError:
        log.error("Lora_beacon executable not found at %s", RADIO_BEACON_EXE)
    except Exception:
        log.exception("Unexpected error during transmit")
    finally:
        _radio_power(False)

# ---------------------------------------------------------------------------
# Main loop
# ---------------------------------------------------------------------------
def main():
    _setup_logging()
    _gpio_setup()
    log.info("RPi beacon started (interval=%ds, freq=%dMHz)",
             BEACON_INTERVAL_S, S_BAND_FREQ)

    while True:
        try:
            t   = collect_telemetry()
            pkt = build_packet(t)
            log.info(
                "Beacon | boot=%d up=%ds temp=%.1f°C cpu=%d%% mem=%d%% disk=%d%%",
                t["boot_count"], t["uptime_s"], t["cpu_temp"] / 10,
                t["cpu_pct"], t["mem_pct"], t["disk_pct"],
            )
            transmit(pkt)
        except Exception:
            log.exception("Beacon cycle error")
        time.sleep(BEACON_INTERVAL_S)


if __name__ == "__main__":
    main()
