import adafruit_rfm9x
import board
import busio
import digitalio

import config
from logger import get_logger

logger = get_logger("GS.RadioInit")

# Global hardware objects
rfm9x = None
led = None
spi = None
cs = None
reset = None


def detect_board():
    """Detect which board we're running on"""
    # Check for Pico 2 specific pins
    if hasattr(board, "GP0") and hasattr(board, "GP28"):
        return "PICO2"
    # Check for Feather M4 specific pins
    elif hasattr(board, "D0") and hasattr(board, "SCK") and hasattr(board, "A5"):
        return "FEATHER_M4"
    else:
        # Default fallback
        return "RPI"


def get_board_pins(board_type):
    """Get pin configuration for specific board"""
    if board_type == "PICO2":
        return {
            "MOSI": board.GP19,
            "MISO": board.GP16,
            "SCK": board.GP18,
            "CS": board.GP17,
            "RESET": board.GP21,
            "LED": board.LED,
        }
    elif board_type == "FEATHER_M4":
        return {
            "MOSI": board.MOSI,  # Usually D23
            "MISO": board.MISO,  # Usually D22
            "SCK": board.SCK,  # Usually D24
            "CS": board.D10,  # Digital pin 10
            "RESET": board.D11,  # Digital pin 11
            "LED": board.D13,  # Built-in LED
        }
    elif board_type == "RPI":
        return {
            "MOSI": board.MOSI,  # Usually D23
            "MISO": board.MISO,  # Usually D22
            "SCK": board.SCK,  # Usually D24
            "CS": board.CE1,  # Lifted from run_sequence.py
            "RESET": board.D25,  # Lifted from run_sequence.py
            # 'LED': board.D13        # Built-in LED
        }
    else:
        # Default to Pico pins
        logger.warning("Unknown board, defaulting to Pico 2 pin configuration")
        return {
            "MOSI": board.GP15,
            "MISO": board.GP12,
            "SCK": board.GP14,
            "CS": board.GP13,
            "RESET": board.GP11,
            "LED": board.LED,
        }


def initialize():
    """Initialize hardware and radio"""
    global rfm9x, led, spi, cs, reset

    # Guard against double initialization (pins can only be claimed once)
    if rfm9x is not None:
        return rfm9x

    # Detect board and configure pins
    board_type = detect_board()
    logger.info("Detected board: %s", board_type)

    pins = get_board_pins(board_type)

    logger.info(
        "Pin configuration: MOSI=%s, MISO=%s, SCK=%s, CS=%s, RESET=%s",
        pins["MOSI"],
        pins["MISO"],
        pins["SCK"],
        pins["CS"],
        pins["RESET"],
    )

    # Setup LED (may already be in use by the system)
    led = digitalio.DigitalInOut(pins["LED"])
    led.direction = digitalio.Direction.OUTPUT

    # Setup Radio
    spi = busio.SPI(pins["SCK"], MOSI=pins["MOSI"], MISO=pins["MISO"])
    cs = digitalio.DigitalInOut(pins["CS"])
    reset = digitalio.DigitalInOut(pins["RESET"])

    logger.info(
        "Initializing Radio with Freq=%.1f MHz, BW=%d Hz, SF=%d, CR=%d, CRC=%s",
        config.config["frequency"],
        config.config["bandwidth"],
        config.config["spreading_factor"],
        config.config["coding_rate"],
        config.config["crc"],
    )

    rfm9x = adafruit_rfm9x.RFM9x(
        spi, cs, reset, config.config["frequency"], crc=config.config["crc"]
    )
    rfm9x.signal_bandwidth = config.config["bandwidth"]
    rfm9x.spreading_factor = config.config["spreading_factor"]
    rfm9x.coding_rate = config.config["coding_rate"]

    logger.info("Radio initialized successfully!")
    return rfm9x
