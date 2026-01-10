
# Hardware abstraction layer for serial port
# Implementation details will depend on device

# RPi specific
from serial import Serial

class SerialPort():

    def __init__(self, serial: Serial):
        # Take in a raw serial port object
        self.serial = serial

    def read(self, num_bytes: int) -> bytes:
        # Read a certain number of bytes
        # May stop early due to a timeout
        return self.serial.read(num_bytes)

    def write(self, data: bytes) -> bool:
        # Write the given data to the port - block until done
        # Return whether write was successful
        self.serial.write(data)

        while self.serial.out_waiting > 0: pass
        return True

    @property
    def has_data(self) -> bool:
        # Return whether true if there is data to read
        return self.serial.in_waiting > 0