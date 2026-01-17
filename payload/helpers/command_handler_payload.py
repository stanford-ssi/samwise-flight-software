import json
from typing import Callable, Any
import logging

from helpers.serial_packet_handler import SerialPacketHandler

log = logging.getLogger(__name__)

class SerialCommandHandlerPayload():
    # Layered class with support for handling commands
    def __init__(self, serial_port, names_to_commands: dict[str, Callable]) -> None:
        # Initialize lower layer
        self.packet_handler = SerialPacketHandler(serial_port)

        # Store dispatch - dictionary from commands to actual functions
        self.names_to_commands = names_to_commands

    def _receive_command(self) -> tuple[str, list, dict]:
        # Receive a command and arguments from the host
        # Returns command, args, kwargs if successful
        # Returns None otherwise
        response = self.packet_handler.read_packet()

        if response is None: return None
        
        # Decode packet
        try:
            packet = response[0]
            command, args, kwargs = json.loads(packet)
        except:
            log.error(f"Error decoding packet {packet}")
            return None
        
        
        return command, args, kwargs

        
    def _dispatch_command(self, command_name: str, args: list, kwargs: dict):
        # Run a command and send back the result
        # The result is a JSON-encoded list with 2 items:
            #Â The first is a boolean representing whether the command was succesful
            # The second is the actual result

        log.debug(f"Dispatching {command_name} with args {args} and kwargs {kwargs}...")

        try:
            if command_name not in self.names_to_commands:
                raise Exception(f"Invalid command {command_name}!")

            command_function = self.names_to_commands[command_name]
            result = command_function(*args, **kwargs)
            successful = True

            log.debug(f"Successfully ran command! Result = {result}")

        except Exception as error:
            # An error occured - send back details of the error
            log.debug(f"An error occured - {type(error).__name__}: {error.with_traceback(None)}")

            result = f"{type(error).__name__}: {error.with_traceback(None)}"
            successful = False

        #Â Send result packet
        self.packet_handler.write_packet(
            json.dumps([successful, result]).encode()
        )


    def receive_and_dispatch_command(self):
        # Receive and run a command
        log.debug("Waiting for command...")
        response = self._receive_command()

        # Error receiving command - do nothing
        if response is None: return

        # Received a command - call with supplied args and kwargs
        command_name, args, kwargs = response
        self._dispatch_command(command_name, args, kwargs)
