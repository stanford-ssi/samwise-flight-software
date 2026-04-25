import subprocess

RADIO_2400_ENABLE = 27


def _pinctrl(*args: str) -> None:
    """Run `pinctrl` and surface failures.

    We use the pinctrl CLI instead of RPi.GPIO / lgpio because on this Pi
    something at boot (likely a `gpio=` line in /boot/firmware/config.txt)
    already claims GPIO 27 via the chardev, so `lgpio.gpio_claim_output`
    fails with `'GPIO busy'`. pinctrl just pokes the SoC registers and
    doesn't need an exclusive chardev claim.
    """
    subprocess.run(["pinctrl", "set", *args], check=True)


def initialize():
    # Configure GPIO 27 as a low output. main.turn_on_2400 will then drive it
    # high to enable the 2400 MHz radio rail.
    _pinctrl(str(RADIO_2400_ENABLE), "op", "dl")
