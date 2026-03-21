"""CLI entry point for pip-installed ground station.

This wrapper exists because code.py shadows the stdlib 'code' module
when installed as a package. CircuitPython devices use code.py directly.
"""

import os
import sys

# Ensure the ground station directory is on the path so that
# code.py (and its sibling modules) are importable.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from code import main  # noqa: E402


def entry():
    main()
