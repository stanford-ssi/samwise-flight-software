import RPi.GPIO as gpio

RADIO_2400_ENABLE = 27

def initialize():
    gpio.setmode(gpio.BCM)
    gpio.setwarnings(False)

    # Radio enable
    # NOTE: initial= must be passed explicitly. On Pi OS Bookworm/Trixie the
    # RPi.GPIO package is the rpi-lgpio shim, which (without initial=) tries
    # to gpio_read() the pin before claiming it and raises
    # `lgpio.error: 'GPIO not allocated'`.
    gpio.setup(RADIO_2400_ENABLE, gpio.OUT, initial=gpio.LOW)
