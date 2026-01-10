# This file defines utility functions for encoding/decoding files using SSDV
# For more info, refer to `/home/pi/code/ssdv/README.md`
#
# Written by Niklas Vainio
# 1/17/2024

import os

SSDV_DIRECTORY = "/home/pi/code/ssdv/"

def encode_file(input_file: str, output_file: str = '', packet_size: int = 253) -> str:
    """
    Encode a file using ssdv

    Parameters:
        input_file          the name of the file to encode
        packet_size         the size of ssdv packets (defaults to 253 packets, which is the size for the 2400)
        output_file         the name of the file (will be `input_file`.ssdv if unspecified)

    Returns:
        The name of the file where the encoded image is
    """
    if not os.path.isfile(input_file): raise FileNotFoundError(f"{input_file} not found!")

    output = output_file if output_file else (input_file + ".ssdv")
    os.system(f"{SSDV_DIRECTORY}/ssdv -e -l {packet_size} {input_file} {output}")

    return output


def decode_file(input_file: str, output_file: str, packet_size: int = 253):
    """
    Decode a file using ssdv

    Parameters:
        input_file          the name of the file to decode
        output_file         the name of the file to place the decoded image
        packet_size         the size of ssdv packets (defaults to 253 packets, which is the size for the 2400)
    """
    if not os.path.isfile(input_file): raise FileNotFoundError(f"{input_file} not found!")
    if not os.path.isfile(output_file): raise FileNotFoundError(f"{output_file} not found!")

    os.system(f"{SSDV_DIRECTORY}/ssdv -d -l {packet_size} {input_file} {output_file}")