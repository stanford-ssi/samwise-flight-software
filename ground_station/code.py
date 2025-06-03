import board
import busio
import digitalio
import time

led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT

MOSI_PIN = board.GP15
MISO_PIN = board.GP12
SCK_PIN = board.GP14
CS_PIN = board.GP13
RESET_PIN = board.GP11

spi = busio.SPI(SCK_PIN, MOSI=MOSI_PIN, MISO=MISO_PIN)
cs = digitalio.DigitalInOut(CS_PIN)
reset = digitalio.DigitalInOut(RESET_PIN)

import adafruit_rfm9x
rfm9x = adafruit_rfm9x.RFM9x(spi, cs, reset, 438.1)
rfm9x.signal_bandwidth = 125000      
rfm9x.spreading_factor = 7           
rfm9x.coding_rate = 5  

def unpack_beacon(packet):
    bytes_list = list(packet)
    str_len = int(bytes_list[0])
    state_name = bytearray(bytes_list[1:str_len + 1]).decode('utf-8')
    itr = str_len + 1
    boot_count = int(bytes_list[itr])
    return f"State: {state_name} | Boot count: {boot_count}"

def try_get_packet():
    packet = rfm9x.receive(timeout = 5.0) # Wait for a packet to be received (up to 5 seconds)
    if packet is not None:
        print(unpack_beacon(packet))
        print('Received: {0}'.format(packet))
        return True
    else:
        print("No packet received.")
        return False

def send_packet_ping():
    packet = bytes([4]) # 0x04 codes the command for NO_OP
    rfm9x.send(packet)
    print(f"Sent packet: {packet}")

def main():
    while True:
        led.value = True
        time.sleep(0.5)
        led.value = False
        time.sleep(0.5)
        print(f"Sending packet")
        send_packet_ping()
        try_get_packet()

main()

