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

    # Pins for multiplexer
    gpio.setup(MULTIPLEXER_PIN1, gpio.OUT)
    gpio.setup(MULTIPLEXER_PIN2, gpio.OUT)
    gpio.setup(MULTIPLEXER_PIN3, gpio.OUT)

    # Radio enable
    gpio.setup(RADIO_2400_ENABLE, gpio.OUT)