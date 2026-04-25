import os
import subprocess
from setup import initialize

# Resolve the Lora_{rx,tx} executables relative to this file so the listener
# works regardless of where the repo is checked out. Build them with:
#   cd ground_station/2400/radio && mkdir -p build && cd build && cmake .. && make -j4
_HERE = os.path.dirname(os.path.abspath(__file__))
RX_2400_EXECUTABLE = os.path.join(_HERE, "radio", "build", "Lora_rx")
TX_2400_EXECUTABLE = os.path.join(_HERE, "radio", "build", "Lora_tx")

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
    while True:
        # Recompute fresh filename each iteration so successive transfers
        # don't clobber the previous file.
        curr_image_id = dir_file_count(IMAGES_DIR)
        curr_image_name = f"{IMAGES_DIR}/{DEFAULT_IMAGE_NAME}{curr_image_id}.jpg"

        print("Listening...")

        # subprocess.run blocks until Lora_rx exits (which it does once a
        # complete file transfer has been received). Using Popen here would
        # spawn a new RX process every loop iteration and they would all
        # contend for /dev/spidev0.0 and the radio GPIOs.
        subprocess.run(
            [RX_2400_EXECUTABLE, str(S_BAND_FREQ), curr_image_name],
            check=False,
        )

        print(f"Finished receiving file. File logged at {curr_image_name}\n")


def turn_on_2400():
    '''
    Turn on the 2400 radio - will be run automatically
    '''
    subprocess.run(["pinctrl", "set", str(RADIO_2400_ENABLE), "op", "dh"], check=True)


def turn_off_2400():
    '''
    Turn off the 2400 radio - must be run manually!
    '''
    subprocess.run(["pinctrl", "set", str(RADIO_2400_ENABLE), "op", "dl"], check=True)


def main():
    print("Initializing pins...")
    initialize()

    print("Turning on radio...")
    turn_on_2400()

    print("Initialization finished...")

    print("Listening for incoming 2400MHz packets...")
    listen_debug()


if __name__ == "__main__":
    main()
