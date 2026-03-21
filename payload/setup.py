# Created by Niklas Vainio
# 2/1/2024
#
# This file defines a a function for global initialization function
# It is mainly used for global GPIO config (e.g. setting pin modes etc)

import RPi.GPIO as gpio

# Constants for pins
MULTIPLEXER_PIN1 = 4
MULTIPLEXER_PIN2 = 17
MULTIPLEXER_PIN3 = 18

RADIO_2400_ENABLE = 27

def initialize():
    gpio.setmode(gpio.BCM)
    gpio.setwarnings(False)

    # The multiplexer stubbornly takes control of pins 4, 17, and 18 on boot of the payload.
    # This causes conflicts with setting up the pins for output and leads to a "GPIO not allocated" error.
    # As long as this issue remains, the multiplexer pins should not be setup with gpio.setup

    # Radio enable
    gpio.setup(RADIO_2400_ENABLE, gpio.OUT)
