"""Top-level entry point.

Edit MODE (or pass via CLI arg) to swap between the FastAPI server, the
interactive CLI, or the legacy debug loop in code.py.

Usage:
    python -m ground_station           # Defaults to server
    python -m ground_station server
    python -m ground_station code
"""

import sys

DEFAULT_MODE = "server"


def main():
    mode = sys.argv[1] if len(sys.argv) >= 2 else DEFAULT_MODE

    if mode == "server":
        from ground_station.server import run

        run()
    elif mode == "code":
        from ground_station.code import main as code_main

        code_main()
    else:
        raise SystemExit(f"unknown mode: {mode!r} (expected server|cli|code)")
