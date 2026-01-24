import board
import busio
import digitalio
import adafruit_rfm9x
import time
from . import config

# Global hardware objects
rfm9x = None
led = None
spi = None
cs = None
reset = None

def detect_board():
    """Detect which board we're running on"""
    # Check for Pico 2 specific pins
    if hasattr(board, 'GP0') and hasattr(board, 'GP28'):
        return "PICO2"
    # Check for Feather M4 specific pins
    elif hasattr(board, 'D0') and hasattr(board, 'SCK') and hasattr(board, 'A5'):
        return "FEATHER_M4"
    else:
        # Default fallback
        return "RPI"

def get_board_pins(board_type):
    """Get pin configuration for specific board"""
    if board_type == "PICO2":
        return {
            'MOSI': board.GP15,
            'MISO': board.GP12,
            'SCK': board.GP14,
            'CS': board.GP13,
            'RESET': board.GP11,
            'LED': board.LED
        }
    elif board_type == "FEATHER_M4":
        return {
            'MOSI': board.MOSI,     # Usually D23
            'MISO': board.MISO,     # Usually D22
            'SCK': board.SCK,       # Usually D24
            'CS': board.A5,         # Analog pin 5
            'RESET': board.D5,      # Digital pin 5
            'LED': board.D13        # Built-in LED
        }
    elif board_type == "RPI":
        return {
            'MOSI': board.MOSI,     # Usually D23
            'MISO': board.MISO,     # Usually D22
            'SCK': board.SCK,       # Usually D24
            'CS': board.CE1,        # Lifted from run_sequence.py
            'RESET': board.D25,     # Lifted from run_sequence.py
            # 'LED': board.D13        # Built-in LED
        }
    else:
        # Default to Pico pins
        print("Unknown board, defaulting to Pico 2 pin configuration")
        return {
            'MOSI': board.GP15,
            'MISO': board.GP12,
            'SCK': board.GP14,
            'CS': board.GP13,
            'RESET': board.GP11,
            'LED': board.LED
        }

def initialize():
    """Initialize hardware and radio"""
    global rfm9x, led, spi, cs, reset
    
    # Detect board and configure pins
    board_type = detect_board()
    print("Detected board: {}".format(board_type))
    
    pins = get_board_pins(board_type)
    
    print("Pin configuration:")
    print("  MOSI: {}".format(pins['MOSI']))
    print("  MISO: {}".format(pins['MISO']))
    print("  SCK: {}".format(pins['SCK']))
    print("  CS: {}".format(pins['CS']))
    print("  RESET: {}".format(pins['RESET']))
    
    # Setup LED
    if pins.get('LED') is not None:
        led = digitalio.DigitalInOut(pins['LED'])
        led.direction = digitalio.Direction.OUTPUT
        
    # Setup Radio
    spi = busio.SPI(pins['SCK'], MOSI=pins['MOSI'], MISO=pins['MISO'])
    cs = digitalio.DigitalInOut(pins['CS'])
    reset = digitalio.DigitalInOut(pins['RESET'])
    
    print("\n=== Initializing Radio ===")
    print(f"Frequency: {config.config['frequency']} MHz")
    print(f"Bandwidth: {config.config['bandwidth']} Hz")
    print(f"Spreading Factor: {config.config['spreading_factor']}")
    print(f"Coding Rate: {config.config['coding_rate']}")
    print(f"CRC: {config.config['crc']}")
    
    rfm9x = adafruit_rfm9x.RFM9x(spi, cs, reset, config.config['frequency'], crc=config.config['crc'])
    rfm9x.signal_bandwidth = config.config['bandwidth']
    rfm9x.spreading_factor = config.config['spreading_factor']
    rfm9x.coding_rate = config.config['coding_rate']
    
    print("Radio initialized successfully!")
    return rfm9x
