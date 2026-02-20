# SAMWISE Flight Computer Code

Flight computer firmware for the SAMWISE 2U cubesat. Designed for the Raspberry
Pi RP2040 and RP2350 microcontrollers.

![PXL_20251105_063120461](https://github.com/user-attachments/assets/f1ea9d1d-33db-42ee-8892-54c7ed03953b)

## Getting Started

See [the onboarding doc](docs/ONBOARDING.md) for development environment setup. To set some useful configurations for this repo, run `configure.sh`.

## Design Objectives

Beyond functional parity with the prior CircuitPython-based Sapling firmware,
this project has a few additional goals:
* Reliable OTAs
* Use the [blackboard pattern](https://en.wikipedia.org/wiki/Blackboard_(design_pattern))
* Eliminate heap allocations
* Simple flight/debug configuration
* Clean code

## Building
To build the code in this repo, run `cmake -B build -DPROFILE=PICUBED-DEBUG` then `cmake --build build --parallel`.

You can use the pre-provided scripts, `source build_tests.sh` and `source build_debug.sh`, to run automatically.
**NOTE**: These automatically delete the `build_tests` and `build` directories respectively before running, so run with caution!

The following targets will be built:
* `samwise_pico_debug`: pico exectuable
* `samwise_picubed_debug`: picubed executable, for debugging
* `samwise_picubed_flight`: picubed executable, for flight
* `samwise_picubed_bringup`: picubed executable, for bringing up the board
(these can be configured in `CMakeLists.txt`)

### Build Archives
The **C Build** github action automatically builds RP2040 and RP2350 archives on pushes to pull requests into main.

## Switching build targets
To switch between builds for the RP2040 (Pico 1) and RP2350 (Pico 2 and Pycubed), set the `RP2350` flag to 1 when calling `cmake`. For example, to build for the RP2350, run:
```
cmake .. -DRP2350=1
```

**Note that when switching between the RP2040 and RP2350, it is usually necessary to clear the cmake cache (by running `rm -rf build/*`)**

## Using Pico on WSL (windows)
Very specific instructions for WSL (some steps are shared with normal use). Replace Ubuntu with your favorite Linux/WSL flavor :)

0. Compile with profile PICO (`source build_pico.sh`)
1. Press and hold BOOT, plug in Pico, and then release BOOT
2. File explorer should pop up with the directory contents
3. Drag and drop `.uf2` file in `build_pico/src` into the pico file explorer
4. The pico should disconnect and reconnect -- success! (hopefully)

Now, to read from pico:

5. Install tio if not already on Ubuntu (`sudo snap install tio --classic`)
    * You may have to add `/snap/bin` to PATH using `.zshrc` or `.bashrc`
6. On a Windows Terminal with Administrator Access, run `usbipd list`. You should get:
```
BUSID  VID:PID    DEVICE                                                        STATE
2-5    2e8a:0009  USB Serial Device (COM8), Reset                               Shared
````
7. If "Not shared", run `usbipd bind --busid 2-5` (replacing busid with the actual busid from your PICO)
8. Now, run `usbipd attach --wsl --busid 2-5`.
9. In Ubuntu now, you should be able to interact with the pico using tio! For example, `sudo tio /dev/ttyACM0`.
