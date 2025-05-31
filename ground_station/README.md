# Ground Station (mock)

Mock ground station for testing purposes.

`lib/adafruit_rfm9x.mpy` should be copied into the pico running circuit python. Driver library code taken from: https://github.com/adafruit/Adafruit_CircuitPython_RFM9x.

Current code just sends a NO_OP command then attempts to read back a beacon message from the satellite.
