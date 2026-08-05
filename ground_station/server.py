"""
FastAPI ground station server.

Wraps a single background polling loop (RX + TX) around the existing LoraRadio interface.
Received beacon packets are broadcast to all connected WebSocket clients in real-time.
Commands are submitted via HTTP and queued for the polling thread to send.

Run with:
    python server.py
    # or: uvicorn server:app --host 127.0.0.1 --port 8000

Dashboard available at http://localhost:8000
"""

import asyncio
import collections
import os
import queue
import threading
import time
from contextlib import asynccontextmanager
from typing import Optional

from fastapi import FastAPI, HTTPException, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse
from pydantic import BaseModel

import radio_initialization as hardware
from logger import get_logger
from radio_commands import LoraRadio, get_radio
from state import state_manager

logger = get_logger("GS.Server")

# ---------------------------------------------------------------------------
# Beacon serialization
# ---------------------------------------------------------------------------

_STATIC_DIR = os.path.join(os.path.dirname(__file__), "static")


def _serialize_beacon(beacon_data, rssi=None, snr=None) -> dict:
    """Convert a BeaconData object to a JSON-serializable dict."""
    result = {
        "timestamp": time.time(),
        "state_name": beacon_data.state_name,
        "callsign": beacon_data.callsign,
        "raw_hex": beacon_data.raw_hex,
        "rssi": rssi,
        "snr": snr,
        "stats": None,
        "adcs": None,
    }

    if beacon_data.stats is not None:
        s = beacon_data.stats
        result["stats"] = {
            "reboot_counter": s.reboot_counter,
            "time_in_state_ms": s.time_in_state_ms,
            "rx_bytes": s.rx_bytes,
            "rx_packets": s.rx_packets,
            "rx_backpressure_drops": s.rx_backpressure_drops,
            "rx_bad_packet_drops": s.rx_bad_packet_drops,
            "tx_bytes": s.tx_bytes,
            "tx_packets": s.tx_packets,
            "battery_voltage": s.battery_voltage,
            "battery_current": s.battery_current,
            "solar_voltage": s.solar_voltage,
            "solar_current": s.solar_current,
            "panel_A_voltage": s.panel_A_voltage,
            "panel_A_current": s.panel_A_current,
            "panel_B_voltage": s.panel_B_voltage,
            "panel_B_current": s.panel_B_current,
            "device_status": s.device_status,
            "device_status_flags": s.device_status_flags,
        }

    if beacon_data.adcs is not None:
        a = beacon_data.adcs
        result["adcs"] = {
            "angular_velocity": a.angular_velocity,
            "state": a.state,
            "boot_count": a.boot_count,
            "quaternion": {
                "q0": a.quaternion.q0,
                "q1": a.quaternion.q1,
                "q2": a.quaternion.q2,
                "q3": a.quaternion.q3,
            },
        }

    return result


# ---------------------------------------------------------------------------
# Polling thread
# ---------------------------------------------------------------------------


def _poll_loop(radio: LoraRadio, tx_queue: queue.Queue, rx_queue: asyncio.Queue, loop, state: dict):
    """Background thread: poll radio for RX, drain tx_queue for TX.

    Bridges the synchronous hardware interface to the asyncio event loop via
    loop.call_soon_threadsafe() for safe cross-thread queue access.

    Both RX and TX steps are individually wrapped in try/except so that a
    single bad packet or failing command never kills the thread.
    """
    while state["running"]:
        # --- RX: check for incoming packets ---
        try:
            beacon = radio.try_get_packet(timeout=0.01)
            if beacon is not None:
                state["packet_count"] += 1
                state["last_seen"] = time.time()
                beacon_dict = _serialize_beacon(beacon)
                state["history"].append(beacon_dict)
                loop.call_soon_threadsafe(rx_queue.put_nowait, beacon_dict)
        except Exception as rx_err:
            logger.error("Poll RX error: %s", rx_err)

        # --- TX: execute any queued commands ---
        try:
            cmd_fn = tx_queue.get_nowait()
            try:
                cmd_fn()
            except Exception as cmd_err:
                logger.error("Command execution failed: %s", cmd_err)
        except queue.Empty:
            pass

        time.sleep(0.01)


# ---------------------------------------------------------------------------
# WebSocket broadcast loop (async, runs inside uvicorn event loop)
# ---------------------------------------------------------------------------


async def _broadcast_loop(rx_queue: asyncio.Queue, connections: set):
    """Drain rx_queue and fan out each beacon dict to all active WebSocket clients."""
    while True:
        data = await rx_queue.get()
        dead = set()
        for ws in list(connections):
            try:
                await ws.send_json(data)
            except Exception:
                dead.add(ws)
        connections -= dead


# ---------------------------------------------------------------------------
# App factory (allows test injection of a mock radio)
# ---------------------------------------------------------------------------


def create_app(radio_override: Optional[LoraRadio] = None) -> FastAPI:
    """Create and return the FastAPI application.

    Args:
        radio_override: If provided, use this LoraRadio instance instead of
                        calling get_radio() (used by tests to inject a mock).
    """
    # Shared mutable state (all access from poll thread; reads from HTTP handlers are safe
    # because individual int/float/list assignments in CPython are effectively atomic)
    state = {
        "running": False,
        "packet_count": 0,
        "last_seen": None,
        "history": collections.deque(maxlen=100),
    }

    tx_queue: queue.Queue = queue.Queue()
    connections: set = set()

    # asyncio.Queue is created inside the lifespan (needs a running event loop on Python < 3.10)
    rx_queue_holder: list = []

    @asynccontextmanager
    async def lifespan(app: FastAPI):
        # Initialise radio
        if radio_override is not None:
            radio = radio_override
        else:
            hardware.initialize()
            radio = get_radio()

        # asyncio.Queue must be created inside the running event loop
        rx_queue: asyncio.Queue = asyncio.Queue()
        rx_queue_holder.append(rx_queue)

        loop = asyncio.get_event_loop()

        # Start background asyncio broadcast task
        broadcast_task = asyncio.create_task(_broadcast_loop(rx_queue, connections))

        # Start background polling thread
        state["running"] = True
        poll_thread = threading.Thread(
            target=_poll_loop,
            args=(radio, tx_queue, rx_queue, loop, state),
            daemon=True,
            name="radio-poll",
        )
        poll_thread.start()

        yield  # server is running

        # Shutdown
        state["running"] = False
        poll_thread.join(timeout=2.0)
        broadcast_task.cancel()
        state_manager.shutdown()

    app = FastAPI(title="Samwise Ground Station", lifespan=lifespan)

    # -----------------------------------------------------------------------
    # REST endpoints
    # -----------------------------------------------------------------------

    @app.get("/")
    async def dashboard():
        """Serve the HTML dashboard."""
        index = os.path.join(_STATIC_DIR, "index.html")
        return FileResponse(index, media_type="text/html")

    @app.get("/status")
    async def status():
        """Return current ground station status."""
        return {
            "boot_count": state_manager.boot_count,
            "msg_id": state_manager.msg_id,
            "packet_count": state["packet_count"],
            "last_seen": state["last_seen"],
        }

    @app.get("/telemetry")
    async def telemetry():
        """Return the last 100 received beacon packets."""
        return list(state["history"])

    @app.post("/command/no-op")
    async def cmd_no_op():
        """Queue a NO_OP (ping) command — dedicated endpoint for quick diagnostics."""
        radio = radio_override if radio_override is not None else get_radio()
        tx_queue.put_nowait(radio.send_no_op)
        return {"status": "queued", "type": "no-op"}

    class CommandRequest(BaseModel):
        type: str
        arg: Optional[str] = None

    @app.post("/command")
    async def cmd_generic(req: CommandRequest):
        """Queue a command by type.

        Supported types: payload-exec, payload-on, payload-off,
                         state-override, payload-shutdown

        Types that require an argument (arg field):
            payload-exec   — the command string to execute
            state-override — the target state name
        """
        radio = radio_override if radio_override is not None else get_radio()
        cmd_type = req.type.lower().strip()

        if cmd_type == "payload-exec":
            if not req.arg:
                raise HTTPException(status_code=422, detail="'arg' required for payload-exec")
            arg = req.arg
            tx_queue.put_nowait(lambda: radio.send_payload_exec(arg))

        elif cmd_type == "payload-on":
            tx_queue.put_nowait(radio.send_payload_turn_on)

        elif cmd_type == "payload-off":
            tx_queue.put_nowait(radio.send_payload_turn_off)

        elif cmd_type == "state-override":
            if not req.arg:
                raise HTTPException(status_code=422, detail="'arg' required for state-override")
            arg = req.arg
            tx_queue.put_nowait(lambda: radio.send_manual_state_override(arg))

        elif cmd_type == "payload-shutdown":
            tx_queue.put_nowait(radio.send_payload_shutdown)

        else:
            raise HTTPException(status_code=400, detail=f"Unknown command type: '{req.type}'")

        return {"status": "queued", "type": cmd_type}

    # -----------------------------------------------------------------------
    # WebSocket endpoint
    # -----------------------------------------------------------------------

    @app.websocket("/ws")
    async def websocket_endpoint(ws: WebSocket):
        """Real-time beacon stream. Each message is a JSON-serialized BeaconData dict."""
        await ws.accept()
        connections.add(ws)
        # Send current telemetry history on connect so the client can populate its log immediately
        for beacon_dict in list(state["history"]):
            try:
                await ws.send_json(beacon_dict)
            except Exception:
                break
        try:
            while True:
                # Keep the connection alive; broadcast_loop handles outbound messages
                await ws.receive_text()
        except WebSocketDisconnect:
            pass
        finally:
            connections.discard(ws)

    return app


# ---------------------------------------------------------------------------
# Production singleton
# ---------------------------------------------------------------------------

app = create_app()

if __name__ == "__main__":
    import uvicorn

    uvicorn.run(app, host="127.0.0.1", port=8000)
