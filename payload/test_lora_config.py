"""
Tests for LoRa config management (get_lora_config / set_lora_config).
Run from the payload/ directory:
    python -m pytest test_lora_config.py  -or-  python test_lora_config.py
"""
import json
import os
import sys
import tempfile
import unittest
from unittest.mock import MagicMock

# Mock all Pi-specific / hardware modules before importing commands
for mod in [
    "RPi", "RPi.GPIO",
    "helpers.camera_utils", "helpers.ssdv_utils",
    "helpers.serial_file_transfer", "helpers.serial_packet_handler",
    "helpers.serial_port_pi", "helpers.log_utils",
]:
    sys.modules[mod] = MagicMock()

sys.path.insert(0, os.path.dirname(__file__))
import commands


class TestLoraConfig(unittest.TestCase):
    def setUp(self):
        fd, self.tmp_path = tempfile.mkstemp(suffix=".json")
        os.close(fd)
        os.unlink(self.tmp_path)  # start without a file
        commands.LORA_CONFIG_PATH = self.tmp_path

    def tearDown(self):
        if os.path.exists(self.tmp_path):
            os.unlink(self.tmp_path)

    # --- get_lora_config ---

    def test_get_returns_defaults_when_file_missing(self):
        cfg = commands.get_lora_config()
        self.assertEqual(cfg["coding_rate"], "4_8")
        self.assertEqual(cfg["spreading_factor"], 7)
        self.assertEqual(cfg["bandwidth"], "1600")
        self.assertEqual(cfg["packet_size"], 253)
        self.assertEqual(cfg["crc"], True)

    def test_get_reads_existing_file(self):
        data = {"coding_rate": "4_5", "spreading_factor": 10, "frequency": 2450}
        with open(self.tmp_path, "w") as f:
            json.dump(data, f)
        cfg = commands.get_lora_config()
        self.assertEqual(cfg["coding_rate"], "4_5")
        self.assertEqual(cfg["spreading_factor"], 10)
        self.assertEqual(cfg["frequency"], 2450)

    # --- set_lora_config ---

    def test_set_writes_frequency(self):
        commands.set_lora_config({"frequency": 2450})
        cfg = commands.get_lora_config()
        self.assertEqual(cfg["frequency"], 2450)

    def test_set_partial_update_preserves_other_keys(self):
        commands.set_lora_config({"spreading_factor": 10, "frequency": 2400})
        commands.set_lora_config({"coding_rate": "4_5"})
        cfg = commands.get_lora_config()
        self.assertEqual(cfg["spreading_factor"], 10)
        self.assertEqual(cfg["frequency"], 2400)
        self.assertEqual(cfg["coding_rate"], "4_5")

    def test_set_all_params(self):
        commands.set_lora_config({
            "coding_rate": "4_5",
            "bandwidth": "800",
            "spreading_factor": 9,
            "packet_size": 128,
            "preamble_length": 8,
            "crc": False,
            "header_type": "variable",
            "frequency": 2450,
        })
        cfg = commands.get_lora_config()
        self.assertEqual(cfg["coding_rate"], "4_5")
        self.assertEqual(cfg["bandwidth"], "800")
        self.assertEqual(cfg["spreading_factor"], 9)
        self.assertEqual(cfg["packet_size"], 128)
        self.assertEqual(cfg["preamble_length"], 8)
        self.assertEqual(cfg["crc"], False)
        self.assertEqual(cfg["header_type"], "variable")
        self.assertEqual(cfg["frequency"], 2450)

    def test_set_overwrites_frequency(self):
        commands.set_lora_config({"frequency": 2400})
        commands.set_lora_config({"frequency": 2450})
        self.assertEqual(commands.get_lora_config()["frequency"], 2450)


if __name__ == "__main__":
    unittest.main()
