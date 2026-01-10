# SSDV

This folder contains the source for ssdv, a tool for packetizing images into 

## To build the executable

```sudo make clean && sudo make -j4```

## To run the executable

Encode:
```./ssdv -e -l 233 FILE.jpg FILE.jpg.ssdv```

Decode:
```./ssdv -d -l 253 FILE.jpg.ssdv FILE.jpg```


### Full Usage
Usage: ssdv [-e|-d] [-n] [-t <percentage>] [-c <callsign>] [-i <id>] [-q <level>] [-l <length>] [<in file>] [<out file>]

  -e Encode JPEG to SSDV packets.
  -d Decode SSDV packets to JPEG.

  -n Encode packets with no FEC.
  -t For testing, drops the specified percentage of packets while decoding.
  -c Set the callign. Accepts A-Z 0-9 and space, up to 6 characters.
  -i Set the image ID (0-255).
  -q Set the JPEG quality level (0 to 7, defaults to 4).
  -l Set packet length in bytes (max: 256, default 256).
  -v Print data for each packet decoded.




# From the SSDV Docs

SSDV - simple command line app for encoding / decoding SSDV image data

Created by Philip Heron <phil@sanslogic.co.uk>
http://www.sanslogic.co.uk/ssdv/

A robust packetised version of the JPEG image format.

Uses the Reed-Solomon codec written by Phil Karn, KA9Q.

ENCODING

$ ssdv -e -c TEST01 -i ID input.jpeg output.bin

This encodes the 'input.jpeg' image file into SSDV packets stored in the 'output.bin' file. TEST01 (the callsign, an alphanumeric string up to 6 characters) and ID (a number from 0-255) are encoded into the header of each packet. The ID should be changed for each new image transmitted to allow the decoder to identify when a new image begins.

The output file contains a series of fixed-length SSDV packets (default 256 bytes). Additional data may be transmitted between each packet, the decoder will ignore this.

DECODING

$ ssdv -d input.bin output.jpeg

This decodes a file 'input.bin' containing a series of SSDV packets into the JPEG file 'output.jpeg'.

LIMITATIONS

Only JPEG files are supported, with the following limitations:

 - Greyscale or YUV/YCbCr colour formats
 - Width and height must be a multiple of 16 (up to a resolution of 4080 x 4080)
 - Baseline DCT only
 - The total number of MCU blocks must not exceed 65535

INSTALLING

make

TODO

* Allow the decoder to handle multiple images in the input stream.
* Experiment with adaptive or multiple huffman tables.

