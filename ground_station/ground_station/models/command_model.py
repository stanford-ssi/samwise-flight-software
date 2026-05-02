from . import USE_PYDANTIC
from ground_station.models.packet_model import PacketModel

# TODO: Remove this? Is this useful?


class CommandPacket(PacketModel):
    """Packet type for sending commands to the satellite.

    This packet type handles command encoding, authentication,
    and proper message ID sequencing.
    """

    if USE_PYDANTIC:
        command_id: int = 0
        command_payload: str = ""
    else:

        def __init__(self, command_id=0, command_payload="", **kwargs):
            super().__init__(**kwargs)
            self.command_id = command_id
            self.command_payload = command_payload

    @classmethod
    def create_command(cls, cmd_id: int, cmd_payload: str = "", **kwargs):
        """Create a properly formatted command packet"""
        # Implementation would use existing protocol.create_cmd_payload logic
        pass
