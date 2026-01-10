import serial
import logging

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

# Initialise pins -----------
initialize()

# Setting up the logger ------

update_boot_count()
boot_count = get_boot_count()
setup_logger(boot_count)
clean_logs()

log = logging.getLogger(__name__)

# Main code entry point
log.info(f"Pi running, boot number {boot_count}...")

with serial.Serial(PORT_NAME, BAUDRATE, timeout=TIMEOUT) as ser:

    command_handler = SerialCommandHandlerPayload(ser, commands.NAMES_TO_COMMANDS)

    #Â Set file transfer protocol in commands module
    commands.file_transfer = SerialFileTransfer(ser)
    commands.packet_handler = SerialPacketHandler(ser)

    while True:
        command_handler.receive_and_dispatch_command()
