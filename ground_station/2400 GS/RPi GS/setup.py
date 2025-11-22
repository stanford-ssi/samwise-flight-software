import RPi.GPIO as gpio

RADIO_2400_ENABLE = 27

def initialize():
    gpio.setmode(gpio.BCM)
    gpio.setwarnings(False)

    # Radio enable
    gpio.setup(RADIO_2400_ENABLE, gpio.OUT)
