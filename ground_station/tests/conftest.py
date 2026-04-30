"""
Pytest configuration and shared fixtures for ground station tests.
"""

import os
import sys
from unittest.mock import MagicMock

import pytest

sys.modules["board"] = MagicMock()
sys.modules["busio"] = MagicMock()
sys.modules["digitalio"] = MagicMock()
sys.modules["adafruit_rfm9x"] = MagicMock()


@pytest.fixture(scope="session")
def test_data_dir():
    """Create and return the path to test data directory"""
    test_dir = os.path.join(os.path.dirname(__file__), "runtime_artifacts")
    os.makedirs(test_dir, exist_ok=True)
    return test_dir


def pytest_configure(config):
    """Register custom markers"""
    config.addinivalue_line("markers", "unit: Unit tests for individual components")
    config.addinivalue_line("markers", "integration: Integration tests for component interactions")
    config.addinivalue_line("markers", "compatibility: Platform compatibility tests")
    config.addinivalue_line("markers", "protocol: Protocol encoding/decoding tests")
    config.addinivalue_line("markers", "filtering: Packet filtering tests")
