
# This file defines several helper functions for talking and processing images using the camera
from PIL import Image
import json
import os
import logging

import RPi.GPIO as gpio

CODE_DIR = "/home/pi/code"
IMAGES_DIR = "/home/pi/images"
VID_DIR = "/home/pi/videos"
log = logging.getLogger(__name__)

def capture_raw_image(image_id: str, config_profile: str, camera_name: str) -> int:
    # Take a photo using the camera using supplied configuration (flags to pass into libcamera)
    # Saves the image to images/{image_id}_raw.png
    # Returns the size of the file in bytes

    select_camera(camera_name)

    # Read config data (libcamera-still flags) from file
    with open(f"{CODE_DIR}/photo_config.json", "r") as config_file:
        camera_flags_dict = json.loads(config_file.read())

    camera_flags = camera_flags_dict[config_profile]
    log.info(f"Taking photo with camera '{camera_name}' profile '{config_file}', flags '{camera_flags}'")

    # Take image using `libcamera-still``
    image_filepath = f"{IMAGES_DIR}/{image_id}_raw.jpg"
    os.system(f"libcamera-still -o {image_filepath} {camera_flags}")

    return os.path.getsize(image_filepath)


def downsize_and_compress_image(image_id: str, width: int, height: int, quality: int) -> int:
    # Downsize a raw image (images/{image_id}_raw.png) and save as a compressed JPEG image (images/{image_id}.jpg)
    # Quality parameter ranges from 1 (worst) to 100 (best) and determines how much the image is compressed
    # Returns the size of the compressed file in bytes

    input_filepath = f"{IMAGES_DIR}/{image_id}_raw.jpg"
    output_filepath = f"{IMAGES_DIR}/{image_id}.jpg"

    raw_image = Image.open(input_filepath)

    resized_image = raw_image.resize((width, height))
    resized_image.save(output_filepath, quality=quality)

    return os.path.getsize(output_filepath)


def split_compressed_image(image_id: str, cells_x: int, cells_y: int, quality: int) -> tuple[int, int]:
    # Split an image into cells_x * cells_y cells
    # !!! May crop a few pixels at the right/bottom edges due to rounding !!!

    # Returns the sizes of the average cell (rounded to nearest integer) and largest cell, in bytes

    input_filepath = f"{IMAGES_DIR}/{image_id}.jpg"

    # If cell size of 1x1 is provided, don't split the image, just return file size\
    if cells_x == 1 and cells_y == 1:
        file_size = os.path.getsize(input_filepath)
        return file_size, file_size

    input_image = Image.open(input_filepath)
    width, height = input_image.size

    cell_width = width // cells_x
    cell_height = height // cells_y

    cell_index = 0

    cell_sizes = []

    # Loop through all cells and crop to the box in question
    for y in range(cells_y):
        for x in range(cells_x):
            cell_top = y * cell_height
            cell_left = x * cell_width

            cell_image = input_image.crop((cell_left, cell_top, cell_left + cell_width, cell_top + cell_height))
            cell_image.save(f"{IMAGES_DIR}/{image_id}_{cell_index}.jpg", quality=quality)

            cell_sizes.append(os.path.getsize(f"{IMAGES_DIR}/{image_id}_{cell_index}.jpg"))

            cell_index += 1

    # Return size of the average cell (in bytes) and largest cell (in bytes)
    return sum(cell_sizes) // len(cell_sizes), max(cell_sizes)

def capture_raw_vid(vid_id: str, libcamera_config_profile: str, camera_name: str) -> int:
    select_camera(camera_name)
    with open(f"{CODE_DIR}/vid_config.json", "r") as config_file:
        camera_flags_dict = json.loads(config_file.read())
    camera_flags = camera_flags_dict['libcamera'][libcamera_config_profile]
    log.info(f"Taking video '{vid_id}' with camera '{camera_name}'")
    vid_filepath = f"{VID_DIR}/{vid_id}_raw.h265"
    os.system(f"libcamera-vid -o {vid_filepath} {camera_flags}")
    return os.path.getsize(vid_filepath)


def compress_vid(vid_id: str, ffmpeg_in_config_profile: str, ffmpeg_out_config_profile) -> int:
    # Downsize a raw image (images/{image_id}_raw.png) and save as a compressed JPEG image (images/{image_id}.jpg)
    # Quality parameter ranges from 1 (worst) to 100 (best) and determines how much the image is compressed
    # Returns the size of the compressed file in bytes

    input_filepath = f"{VID_DIR}/{vid_id}_raw.h265"
    output_filepath = f"{VID_DIR}/{vid_id}.mp4"
    with open(f"{CODE_DIR}/vid_config.json", "r") as config_file:
        camera_flags_dict = json.loads(config_file.read())
    ffmpeg_in_flags = camera_flags_dict['ffmpeg_in'][ffmpeg_in_config_profile]
    ffmpeg_out_flags = camera_flags_dict['ffmpeg_out'][ffmpeg_out_config_profile]
    log.info('Compressing video {input_filepath}')
    os.system(f'ffmpeg -y -nostdin {ffmpeg_in_flags} -i {input_filepath} {ffmpeg_out_flags} {output_filepath}')
    return os.path.getsize(output_filepath)

def select_camera(camera_name : str) -> None:
    '''Tells the Arducam multicamera adapted which camera to select'''

    if camera_name == "NULL": return #to be used whenever the Multiplexer is not attached
    PIN1, PIN2, PIN3 = 4, 17, 18 #The three pins used by the Camera Splitter to communicate with the Pi.
    #Maps each camera to the corresponding i2cset command and pins to be supplied to the camera splitter
    CONFIG_MAP = {
        'A' : ("i2cset -y 1 0x70 0x00 0x04",(0,0,1)),
        'B' : ("i2cset -y 1 0x70 0x00 0x05",(1,0,1)),
        'C' : ("i2cset -y 1 0x70 0x00 0x06",(0,1,0)),
        'D' : ("i2cset -y 1 0x70 0x00 0x07",(1,1,0))
    }

    if camera_name not in CONFIG_MAP:
        log.warning('Invalid cameraname given to select_camera, selecting Camera A as default')
    command, gpiovals = CONFIG_MAP.get(camera_name, CONFIG_MAP['A'])

    gpio.output(PIN1, gpiovals[0])
    gpio.output(PIN2, gpiovals[1])
    gpio.output(PIN3, gpiovals[2])

    os.system(command)
