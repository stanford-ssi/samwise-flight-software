import serial
import logging
import sys
import time

import commands
from setup import initialize
from helpers.command_handler_payload import SerialCommandHandlerPayload
from helpers.serial_packet_handler import SerialPacketHandler
from helpers.serial_file_transfer import SerialFileTransfer
from helpers.log_utils import update_boot_count, get_boot_count, setup_logger, clean_logs

# Static Config --------------

PORT_NAME = "/dev/serial0"
BAUDRATE = 115200

TIMEOUT = 10

MAX_SERIAL_RETRIES = 5
SERIAL_RETRY_DELAY = 2

# Initialise pins -----------
try:
    initialize()
except Exception as e:
    print(f"GPIO init failed: {e}")

# Setting up the logger ------
boot_count = -1
try:
    update_boot_count()
    boot_count = get_boot_count()
    setup_logger(boot_count)
    clean_logs()
except Exception as e:
    print(f"Logger setup failed: {e}")
    try:
        setup_logger(boot_count)
    except Exception:
        pass

log = logging.getLogger(__name__)

# Main code entry point
log.info(f"Pi running, boot number {boot_count}...")

# Open serial port with retries
for attempt in range(MAX_SERIAL_RETRIES):
    try:
        ser = serial.Serial(PORT_NAME, BAUDRATE, timeout=TIMEOUT)
        break
    except serial.SerialException as e:
        log.error(f"Serial open failed (attempt {attempt + 1}/{MAX_SERIAL_RETRIES}): {e}")
        time.sleep(SERIAL_RETRY_DELAY)
else:
    log.critical("Could not open serial port after retries. Exiting.")
    sys.exit(1)

with ser:
    command_handler = SerialCommandHandlerPayload(ser, commands.NAMES_TO_COMMANDS)

    # Set file transfer protocol in commands module
    commands.file_transfer = SerialFileTransfer(ser)
    commands.packet_handler = SerialPacketHandler(ser)

    while True:
        try:
            command_handler.receive_and_dispatch_command()
        except Exception as e:
            log.error(f"Unhandled error in command loop: {e}", exc_info=True)
