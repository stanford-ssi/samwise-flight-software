from typing import Optional

from . import USE_PYDANTIC, _BaseModel


class PacketModel(_BaseModel):
    """Base class for all satellite communication packets.

    Handles the radio-level protocol stuff like authentication and parsing
    out the data field. Specific packet types inherit from this class.
    """

    if USE_PYDANTIC:
        dst: int = 0
        src: int = 0
        flags: int = 0
        seq: int = 0
        data: bytes = b""
        boot_count: Optional[int] = None
        msg_id: Optional[int] = None
        hmac_digest: Optional[bytes] = None
    else:

        def __init__(
            self,
            dst=0,
            src=0,
            flags=0,
            seq=0,
            data=b"",
            boot_count=None,
            msg_id=None,
            hmac_digest=None,
            **kwargs,
        ):
            self.dst = dst
            self.src = src
            self.flags = flags
            self.seq = seq
            self.data = data
            self.boot_count = boot_count
            self.msg_id = msg_id
            self.hmac_digest = hmac_digest

    @property
    def header_bytes(self) -> bytes:
        import struct

        return struct.pack("BBBBB", self.dst, self.src, self.flags, self.seq, len(self.data))

    @property
    def footer_bytes(self) -> bytes:
        import struct

        if self.boot_count is not None and self.msg_id is not None:
            return struct.pack("<II", self.boot_count, self.msg_id)
        return b""

    @classmethod
    def from_raw_data(cls, raw_data: bytes):
        """Create packet instance from raw radio data"""
        # This would be implemented by subclasses
        raise NotImplementedError("Subclasses must implement from_raw_data")
