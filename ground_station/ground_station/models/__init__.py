"""
Data models for ground station packets and telemetry.

This module provides CircuitPython-compatible data models without requiring Pydantic.
On CPython (Raspberry Pi), Pydantic validation is used if available.
On CircuitPython (Pico/Feather), simple classes are used instead.
"""

from ground_station.config import IS_CIRCUITPYTHON

if not IS_CIRCUITPYTHON:
    try:
        from pydantic import BaseModel

        _BaseModel = BaseModel

        USE_PYDANTIC = True
    except ImportError:
        USE_PYDANTIC = False
else:
    USE_PYDANTIC = False

    class _BaseModel:
        """Simple base model for CircuitPython compatibility"""

        def __init__(self, **kwargs):
            for key, value in kwargs.items():
                setattr(self, key, value)

        def dict(self):
            """Convert to dictionary (Pydantic-compatible method)"""
            return {k: v for k, v in self.__dict__.items() if not k.startswith("_")}


from .adcs_model import *
from .beacon_model import *
from .packet_model import *
