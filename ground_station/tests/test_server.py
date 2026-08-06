"""
Tests for the FastAPI ground station server.

Uses starlette's TestClient (ships with FastAPI) for sync HTTP and WebSocket tests.
The radio hardware is fully mocked via MagicMock — no physical hardware required.
"""

import os
import sys
from unittest.mock import MagicMock

import pytest

# Hardware mocking is handled in conftest.py, but we also need to guard server imports
# from triggering radio_initialization at module load time.
parent_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if parent_dir not in sys.path:
    sys.path.append(parent_dir)

from starlette.testclient import TestClient  # noqa: E402

from models import ADCSData, ADCSQuaternion, BeaconData, BeaconStats  # noqa: E402
from radio_commands import LoraRadio  # noqa: E402
from server import create_app  # noqa: E402

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _make_beacon(state_name="test_state") -> BeaconData:
    """Build a minimal BeaconData object for testing."""
    stats = BeaconStats(
        reboot_counter=3,
        time_in_state_ms=12000,
        rx_bytes=100,
        rx_packets=5,
        rx_backpressure_drops=0,
        rx_bad_packet_drops=0,
        tx_bytes=200,
        tx_packets=10,
        battery_voltage=3700,
        battery_current=50,
        solar_voltage=5100,
        solar_current=120,
        panel_A_voltage=3300,
        panel_A_current=30,
        panel_B_voltage=3300,
        panel_B_current=30,
        device_status=0x23,
    )
    adcs = ADCSData(
        angular_velocity=0.25,
        quaternion=ADCSQuaternion(q0=1.0, q1=0.0, q2=0.0, q3=0.0),
        state=1,
        boot_count=3,
    )
    return BeaconData(state_name=state_name, stats=stats, adcs=adcs, callsign="KC3WNY")


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------


@pytest.fixture()
def mock_radio():
    """A LoraRadio-shaped mock with try_get_packet returning None by default."""
    radio = MagicMock(spec=LoraRadio)
    radio.try_get_packet.return_value = None
    return radio


@pytest.fixture()
def client(mock_radio):
    """TestClient wrapping a create_app(mock_radio) instance."""
    app = create_app(radio_override=mock_radio)
    # TestClient manages lifespan automatically
    with TestClient(app, raise_server_exceptions=True) as c:
        yield c


# ---------------------------------------------------------------------------
# /status
# ---------------------------------------------------------------------------


@pytest.mark.unit
@pytest.mark.server
def test_status_returns_expected_fields(client):
    r = client.get("/status")
    assert r.status_code == 200
    data = r.json()
    assert "boot_count" in data
    assert "msg_id" in data
    assert "packet_count" in data
    assert "last_seen" in data
    assert data["packet_count"] == 0


# ---------------------------------------------------------------------------
# /telemetry
# ---------------------------------------------------------------------------


@pytest.mark.unit
@pytest.mark.server
def test_telemetry_starts_empty(client):
    r = client.get("/telemetry")
    assert r.status_code == 200
    assert r.json() == []


# ---------------------------------------------------------------------------
# GET / (dashboard)
# ---------------------------------------------------------------------------


@pytest.mark.unit
@pytest.mark.server
def test_dashboard_html_served(client):
    r = client.get("/")
    assert r.status_code == 200
    assert "text/html" in r.headers["content-type"]
    assert "Samwise" in r.text


# ---------------------------------------------------------------------------
# POST /command/no-op
# ---------------------------------------------------------------------------


@pytest.mark.unit
@pytest.mark.server
def test_no_op_returns_queued(client):
    r = client.post("/command/no-op")
    assert r.status_code == 200
    body = r.json()
    assert body["status"] == "queued"
    assert body["type"] == "no-op"


@pytest.mark.integration
@pytest.mark.server
def test_no_op_calls_radio_method(mock_radio):
    """After POSTing no-op, the poll loop should call radio.send_no_op()."""
    import time

    app = create_app(radio_override=mock_radio)

    with TestClient(app) as client:
        r = client.post("/command/no-op")
        assert r.status_code == 200
        # Give the poll thread time to drain the tx_queue before lifespan teardown
        time.sleep(0.1)

    mock_radio.send_no_op.assert_called()


# ---------------------------------------------------------------------------
# POST /command (generic)
# ---------------------------------------------------------------------------


@pytest.mark.unit
@pytest.mark.server
def test_command_payload_on_queued(client):
    r = client.post("/command", json={"type": "payload-on"})
    assert r.status_code == 200
    assert r.json()["status"] == "queued"


@pytest.mark.unit
@pytest.mark.server
def test_command_payload_off_queued(client):
    r = client.post("/command", json={"type": "payload-off"})
    assert r.status_code == 200


@pytest.mark.unit
@pytest.mark.server
def test_command_payload_shutdown_queued(client):
    r = client.post("/command", json={"type": "payload-shutdown"})
    assert r.status_code == 200


@pytest.mark.unit
@pytest.mark.server
def test_command_payload_exec_requires_arg(client):
    r = client.post("/command", json={"type": "payload-exec"})
    assert r.status_code == 422


@pytest.mark.unit
@pytest.mark.server
def test_command_payload_exec_with_arg(client):
    r = client.post("/command", json={"type": "payload-exec", "arg": '["run_x"]'})
    assert r.status_code == 200
    assert r.json()["status"] == "queued"


@pytest.mark.unit
@pytest.mark.server
def test_command_state_override_requires_arg(client):
    r = client.post("/command", json={"type": "state-override"})
    assert r.status_code == 422


@pytest.mark.unit
@pytest.mark.server
def test_command_state_override_with_arg(client):
    r = client.post("/command", json={"type": "state-override", "arg": "nominal"})
    assert r.status_code == 200


@pytest.mark.unit
@pytest.mark.server
def test_command_unknown_type_returns_400(client):
    r = client.post("/command", json={"type": "self-destruct"})
    assert r.status_code == 400


# ---------------------------------------------------------------------------
# WebSocket
# ---------------------------------------------------------------------------


@pytest.mark.integration
@pytest.mark.server
def test_websocket_connects(client):
    with client.websocket_connect("/ws"):
        # Just connecting should succeed; no data yet since mock radio returns None
        pass  # clean disconnect


@pytest.mark.integration
@pytest.mark.server
def test_websocket_receives_history_on_connect(mock_radio):
    """On WS connect the server replays existing telemetry history."""
    beacon = _make_beacon("history_state")
    # Make try_get_packet return a beacon once, then None forever
    mock_radio.try_get_packet.side_effect = [beacon] + [None] * 10000

    app = create_app(radio_override=mock_radio)
    import time

    with TestClient(app) as client:
        # Give poll thread a moment to pick up the beacon before we connect
        time.sleep(0.15)

        with client.websocket_connect("/ws") as ws:
            data = ws.receive_json()
            assert data["state_name"] == "history_state"
            assert data["stats"]["reboot_counter"] == 3


# ---------------------------------------------------------------------------
# radio_commands.try_get_packet return type
# ---------------------------------------------------------------------------


@pytest.mark.unit
@pytest.mark.server
def test_try_get_packet_returns_none_when_no_packet():
    """try_get_packet() returns None when radio.receive() returns None."""
    mock_rfm = MagicMock()
    mock_rfm.receive.return_value = None
    radio = LoraRadio(mock_rfm)
    result = radio.try_get_packet(timeout=0.0)
    assert result is None


@pytest.mark.unit
@pytest.mark.server
def test_try_get_packet_returns_none_when_radio_is_none():
    radio = LoraRadio(None)
    result = radio.try_get_packet()
    assert result is None


@pytest.mark.unit
@pytest.mark.server
def test_try_get_packet_returns_none_on_rssi_drop():
    """try_get_packet() returns None when RSSI filter drops the packet."""
    import config as cfg

    mock_rfm = MagicMock()
    # Build a valid-looking raw beacon packet
    import struct

    state = b"test_state\x00"
    stats = struct.pack(
        "<LQ6L8HB", 1, 1000, 0, 0, 0, 0, 0, 0, 3700, 50, 5000, 100, 3300, 30, 3300, 30, 0
    )
    adcs = struct.pack("<fffffBL", 0.1, 1.0, 0.0, 0.0, 0.0, 1, 1)
    callsign = b"KC3WNY"
    payload = state + stats + adcs + callsign
    mock_rfm.receive.return_value = bytes([len(payload)]) + payload
    mock_rfm.last_rssi = -130  # below threshold
    mock_rfm.last_snr = 5

    radio = LoraRadio(mock_rfm)

    # Temporarily enable RSSI filter with threshold of -120
    original = dict(cfg.config)
    cfg.config["enable_rssi_filter"] = True
    cfg.config["rssi_threshold"] = -120
    try:
        result = radio.try_get_packet(timeout=0.0)
    finally:
        cfg.config.update(original)

    assert result is None


@pytest.mark.unit
@pytest.mark.server
def test_try_get_packet_returns_beacon_data_on_success():
    """try_get_packet() returns a BeaconData instance on a valid packet."""
    import struct

    import config as cfg

    mock_rfm = MagicMock()
    state = b"nominal\x00"
    stats = struct.pack(
        "<LQ6L8HB", 5, 9000, 0, 0, 0, 0, 0, 0, 3800, 60, 5100, 110, 3300, 25, 3300, 25, 0x43
    )
    adcs = struct.pack("<fffffBL", 0.5, 0.9, 0.1, 0.2, 0.3, 2, 5)
    callsign = b"KC3WNY"
    payload = state + stats + adcs + callsign
    mock_rfm.receive.return_value = bytes([len(payload)]) + payload
    mock_rfm.last_rssi = -80
    mock_rfm.last_snr = 10

    radio = LoraRadio(mock_rfm)

    original = dict(cfg.config)
    cfg.config["enable_rssi_filter"] = False
    cfg.config["enable_callsign_filter"] = False
    try:
        result = radio.try_get_packet(timeout=0.0)
    finally:
        cfg.config.update(original)

    assert result is not None
    assert isinstance(result, BeaconData)
    assert result.state_name == "nominal"
    assert result.stats is not None
    assert result.stats.reboot_counter == 5
