# Main file defining all of the commands
import os
import math
import json
import binascii
import shutil
import subprocess
import RPi.GPIO as gpio

from typing import Any

import helpers.camera_utils as camera_utils
import helpers.ssdv_utils as ssdv_utils
from helpers.serial_file_transfer import SerialFileTransfer
from helpers.serial_packet_handler import SerialPacketHandler
from helpers.log_utils import get_boot_count

CODE_DIR = "/home/pi/code"
IMAGES_DIR = "/home/pi/images"
VIDEOS_DIR = "/home/pi/videos"

TX_2400_EXECUTABLE = "/home/pi/code/radio/build/Lora_tx"
S_BAND_FREQ = 2400

RADIO_2400_ENABLE = 27

# Global objects that will be populated by main.py on startup
packet_handler: SerialPacketHandler = None
file_transfer: SerialFileTransfer = None

# System ----------------------------------------------------------------------------------
def ping():
    '''
    Payload replies with "pong" to confirm it is alive
    '''
    return "pong"


def shutdown():
    '''
    Shutdown immediately
    '''
    # (send a packet first so flight computer thinks we have returned)
    packet_handler.write_packet(b'[true, "shutting down"]')
    os.system("sudo shutdown now")


def reboot():
    '''
    Rebot immediately
    '''
    # (send a packet first so flight computer thinks we have returned)
    packet_handler.write_packet(b'[true, "rebooting"]')
    os.system("sudo reboot now")


def info() -> dict[str, str]:
    '''
    Returns a dict containing temperature, boot count, and amount of free storage
    '''
    temp: str = os.popen("vcgencmd measure_temp").readline().removeprefix('temp=').removesuffix("'C\n")
    id: str = str(get_boot_count())
    storage: str = os.popen('free -h').read()
    return {'temp' : temp, 'id' : id, 'storage' : storage}

# Arbitrary code --------------------------------------------------------------------------
def eval_python(expression: str) -> Any:
    '''
    Payload evaluates the python code contained in `expression` and returns the result
    (This only supports simple expressions - no loops, function or class definitions etc)
    '''
    return eval(expression)


def exec_python(code: str):
    '''
    Payload evaluates the python code contained in `code` 
    (This does support complex expressions like loops, function definitions etc)
    '''
    return exec(code)


def exec_terminal(command: str) -> int:
    '''
    Payload executes `command`` as a terminal command
    Returns the exit code (should be 0 if successful)
    '''
    return os.system(command)


# File management -------------------------------------------------------------------------
def list_dir(path: str) -> list[str]:
    '''
    Returns a list of all the files and subdirectories contained within the specified directory
    '''
    return os.listdir(path)


def delete_file(filepath: str):
    '''
    Deletes the specified file
    '''
    return os.remove(filepath)


def receive_file(filepath: str) -> bool:
    '''
    Receives a file (using `serial_file_transfer.py`) into the specified file path
    '''
    return file_transfer.receive_file(filepath)


def send_file(filepath: str, compressed: bool = False) -> bool:
    '''
    Sends a file (using `serial_file_transfer.py`) from the specified file path
    '''
    if filepath == '/home/pi/logs/0.log': #requesting 0.log means requesting the latest log file
        logs = [logfile for logfile in os.listdir('/home/pi/logs') if not logfile.endswith('xz')]
        latest: str = sorted(logs, key = lambda file : int(file.split('.')[0]))[-1] #most recent non-compressed logfile
        latest: str = os.path.join('/home/pi/logs', latest)
        shutil.copy(latest, filepath) #0.log becomes a copy of the latest log file (we need a copy since we are still writing to the currrent log file
    if compressed:
        os.system(f'xz -9 --compress --keep --force {filepath}')
        filepath = f'{filepath}.xz'

    return file_transfer.send_file(filepath)


def send_file_packet(filepath: str, packet_num: int, packet_size: int = 250) -> str:
    '''
    Sends a specific packet of the specified file, with the bytes encoded in base 64
    (as a string - JSON does not support bytes objects)
    Packet size can be optionally specified (defaults to 250 to match radio)
    '''

    with open(filepath, "rb") as file:
        file.seek(packet_num * packet_size)
        packet = file.read(packet_size)

    return binascii.b2a_base64(packet).decode()


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


def send_file_2400(filepath: str):
    '''
    Send a file using the 2400 Radio - run as a subprocess
    '''
    if not os.path.isfile(filepath): raise FileNotFoundError(f"{filepath} not found!")

    turn_on_2400()
    subprocess.Popen(f"{TX_2400_EXECUTABLE} {S_BAND_FREQ} {filepath}", shell=True)


def send_packets_2400(filepath: str, packets: list[int], packet_size: int = 253):
    '''
    Send packets using 2400
    '''
    #TODO
    pass


def get_num_packets(filepath: str, packet_size: int = 250) -> int:
    '''
    Returns the number of packets in a file (including the partial packet at the end)
    Packet size can be optionally specified (defaults to 250 to match radio)
    '''
    num_bytes = os.path.getsize(filepath)
    num_packets = math.ceil(num_bytes / packet_size)

    return num_packets


def crc_file(filepath: str) -> int:
    '''
    Compute the CRC checksum of a file
    (useful for seeing if files were transferred correctly)
    '''
    with open(filepath, "rb") as file:
        crc = binascii.crc32(file.read())

    return crc


# Taking photos/videos ---------------------------------------------------------------------------
def get_photo_config() -> dict:
    '''
    Returns the contents of `photo_config.json`
    '''
    with open(f"{CODE_DIR}/photo_config.json", "r") as file:
        config = json.loads(file.read())

    return config


def set_photo_config(config: dict):
    '''
    Updates the contents of `photo_config.json`
    '''
    with open(f"{CODE_DIR}/photo_config.json", "w+") as file:
        file.write(json.dumps(config))


def get_video_config() -> dict:
    '''
    Returns the contents of `vid_config.json`
    '''
    with open(f"{CODE_DIR}/vid_config.json", "r") as file:
        config = json.loads(file.read())

    return config


def set_video_config(config: dict):
    '''
    Updates the contents of `vid_config.json`
    '''
    with open(f"{CODE_DIR}/vid_config.json", "w+") as file:
        file.write(json.dumps(config, indent=2))


def take_photo(image_id: str, camera_name='A', config="default", w=800, h=600, quality=100, cells_x=1, cells_y=1) -> tuple[int, int, int, int]:
    '''
    Takes a photo using the specified configuration profile

    The raw photo will be taken and saved to {image_id}_raw.jpg

    The photo will then be downsized, compressed (quality ranges from 1 to 100 
    and determines amount of compression), and saved to {image_id}.jpg

    If cells_x or cells_y is specified, the photo will be split into cells, which will be saved as {filename}_{i}.jpg 
    (i ranges from 0 to cells_x * cells_y - 1)

    Returns:
        size (in bytes) of the raw photo, 
        size (in bytes) of the downsized photo, 
        size (in bytes) of the average cell (rounded to nearest integer) 
        size (in bytes) of the largest cell
    '''

    raw_size = camera_utils.capture_raw_image(image_id, config, camera_name)
    compressed_size = camera_utils.downsize_and_compress_image(image_id, w, h, quality)
    avg_cell_size, max_cell_size = camera_utils.split_compressed_image(image_id, cells_x, cells_y, quality)

    return raw_size, compressed_size, avg_cell_size, max_cell_size


def compress_photo(image_id: str, w=800, h=600, quality=100, cells_x=1, cells_y=1) -> tuple[int, int, int, int]:
    '''
    The photo in {image_id}_raw.jpg will then be downsized, compressed (quality ranges from 1 to 100 
    and determines amount of compression), and saved to {image_id}.jpg

     If cells_x or cells_y is specified, the photo will be split into cells, which will be saved as {image_id}_{i}.jpg 
    (i ranges from 0 to cells_x * cells_y - 1)

    Returns:
        size (in bytes) of the raw photo, 
        size (in bytes) of the downsized photo,
        size (in bytes) of the average cell (rounded to nearest integer) 
        size (in bytes) of the largest cell
    '''
    raw_size = os.path.getsize(f"{IMAGES_DIR}/{image_id}_raw.jpg")
    compressed_size = camera_utils.downsize_and_compress_image(image_id, w, h, quality)
    avg_cell_size, max_cell_size = camera_utils.split_compressed_image(image_id, cells_x, cells_y, quality)

    return raw_size, compressed_size, avg_cell_size, max_cell_size

def take_vid(vid_id: str, camera_name='A', libcamera_config='default', ffmpeg_in_config='default', ffmpeg_out_config='default') -> tuple[int, int]:
    '''
    Takes a video and stores it in {vid_id}_raw.h265
    Compresses it into {vid_id}.mp4 using provided ffmpeg settings
    Length determined by libcamera config (default is 5 seconds, 'longvid' can be 30 seconds)
    
    Returns:
        size (in bytes) of the raw video
        size (in bytes) of the compressed video
    '''

    raw_size = camera_utils.capture_raw_vid(vid_id, libcamera_config, camera_name)
    compressed_size = camera_utils.compress_vid(vid_id, ffmpeg_in_config, ffmpeg_out_config)
    return raw_size, compressed_size



# Image + downlink combination commands ------------------------------------------
def take_process_send_image(image_id: str, camera_name='A', config='default', w=800, h=600, quality=100, cells_x=1, cells_y=1, ssdv=True, radio_type:str='2400'):
    '''
    Takes a photo using `take_photo` and send it either to the pycubed, or over 2400 radio (default)
    Image will automatically be packetized by ssdv unless `ssdv=false` kwarg is suppliued
    '''
    take_photo(image_id, camera_name, config, w, h, quality, cells_x, cells_y)
    image_filepath = f'{IMAGES_DIR}/{image_id}.jpg'

    if radio_type == '433':
        # 433 -> send to pycubed
        send_file(image_filepath)
    elif radio_type == '2400':
        # 2400 -> downlink after ssdv compression
        file_to_send = ssdv_utils.encode_file(image_filepath) if ssdv else image_filepath
        send_file_2400(file_to_send)
    else:
        raise ValueError(f'Radio type should be 433 or 2400 (received {radio_type})')


def process_send_image(image_id: str, w=800, h=600, quality=100, cells_x=1, cells_y=1, ssdv=True, radio_type:str='2400'):
    '''
    Process a photo using `compress_photo` and send it either to the pycubed, or over 2400 radio (default)
    Image will automatically be packetized by ssdv unless `ssdv=false` kwarg is suppliued
    '''
    compress_photo(image_id,w,h,quality, cells_x, cells_y)
    image_filepath = f'{IMAGES_DIR}/{image_id}.jpg'

    if radio_type == '433':
        # 433 -> send to pycubed
        send_file(image_filepath)
    elif radio_type == '2400':
        # 2400 -> downlink after ssdv compression
        file_to_send = ssdv_utils.encode_file(image_filepath) if ssdv else image_filepath
        send_file_2400(file_to_send)
    else:
        raise ValueError(f'Radio type should be 433 or 2400 (received {radio_type})')


# Dictionary mapping command names to actual commands
NAMES_TO_COMMANDS = {
    "ping": ping,
    "shutdown": shutdown,
    "reboot": reboot,
    "info": info,

    "eval_python": eval_python,
    "exec_python": exec_python,
    "exec_terminal": exec_terminal,

    "list_dir": list_dir,
    "delete_file": delete_file,
    "receive_file": receive_file,
    "send_file": send_file,
    "send_file_packet": send_file_packet,
    "turn_on_2400": turn_on_2400,
    "turn_off_2400": turn_off_2400,
    "send_file_2400": send_file_2400,
    "get_num_packets": get_num_packets,
    "crc_file": crc_file,

    "get_photo_config": get_photo_config,
    "set_photo_config": set_photo_config,
    "get_video_config": get_video_config,
    "set_video_config": set_video_config,
    "take_photo": take_photo,
    "compress_photo": compress_photo,
    "take_vid" : take_vid,

    "take_process_send_image" : take_process_send_image,
    "process_send_image" : process_send_image
}
