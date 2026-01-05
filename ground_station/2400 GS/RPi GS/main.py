import os
import subprocess
import RPi.GPIO as gpio
from setup import initialize

RX_2400_EXECUTABLE = "/home/pi/radio/build/Lora_rx"
TX_2400_EXECUTABLE = "/home/pi/radio/build/Lora_tx"

IMAGES_DIR = "/home/pi/images"
VIDEOS_DIR = "/home/pi/videos"

S_BAND_FREQ = 2400
RADIO_2400_ENABLE = 27

DEFAULT_IMAGE_NAME = "2400_image_"
DEFAULT_VIDEO_NAME = "2400_vid_"

def dir_file_count(dir_path: str):
    count = 0 

    for path in os.listdir(dir_path):
        if os.path.isfile(os.path.join(dir_path, path)):
            count += 1

    return count

def listen_debug():
    curr_image_id = dir_file_count(IMAGES_DIR)
    curr_vid_id = dir_file_count(VIDEOS_DIR)

    curr_image_name = f"{IMAGES_DIR}/{DEFAULT_IMAGE_NAME}{curr_image_id}.jpg"
    curr_vid_name = f"{VIDEOS_DIR}/{DEFAULT_VIDEO_NAME}{curr_vid_id}.mp4"

    while True:
        print("Listening...")

        # Turn on a subprocess
        subprocess.Popen(f"{RX_2400_EXECUTABLE} {S_BAND_FREQ} {curr_image_name}", shell=True)

        print("Finished receiving file...\n")
        print(f"File logged at {curr_image_name}")

        curr_image_id = dir_file_count(IMAGES_DIR)
        curr_image_name = curr_image_name = f"{IMAGES_DIR}{DEFAULT_IMAGE_NAME}{curr_image_id}.jpg"

def turn_on_2400():
    '''
    Turn on the 2400 radio - will be run automatically
    '''
    gpio.output(RADIO_2400_ENABLE, 1)


def turn_off_2400():
    '''
    Turn off the 2400 radio - must be run manually!
    '''
    gpio.output(RADIO_2400_ENABLE, 0)

def main():
    print("Initializing pins...")
    initialize()
    
    print("Turning on radio...")
    turn_on_2400()

    print("Initialization finished...")

    print("Listening for incoming 2400MHz packets...")
    listen_debug()
