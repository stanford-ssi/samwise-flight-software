from . import USE_PYDANTIC, _BaseModel

if USE_PYDANTIC:
    from pydantic import Field


class ADCSQuaternion(_BaseModel):
    """ADCS quaternion representation"""

    if USE_PYDANTIC:
        q0: float = 0.0
        q1: float = 0.0
        q2: float = 0.0
        q3: float = 0.0
    else:

        def __init__(self, q0=0.0, q1=0.0, q2=0.0, q3=0.0, **kwargs):
            self.q0 = q0
            self.q1 = q1
            self.q2 = q2
            self.q3 = q3

    @property
    def magnitude(self) -> float:
        return (self.q0**2 + self.q1**2 + self.q2**2 + self.q3**2) ** 0.5


class ADCSData(_BaseModel):
    """ADCS telemetry data"""

    if USE_PYDANTIC:
        angular_velocity: float = 0.0
        quaternion: ADCSQuaternion = Field(default_factory=ADCSQuaternion)
        state: int = 0
        boot_count: int = 0
    else:

        def __init__(self, angular_velocity=0.0, quaternion=None, state=0, boot_count=0, **kwargs):
            self.angular_velocity = angular_velocity
            self.quaternion = quaternion if quaternion else ADCSQuaternion()
            self.state = state
            self.boot_count = boot_count
