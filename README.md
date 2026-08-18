# SAMWISE Flight Computer Code

Flight computer firmware for the SAMWISE 2U cubesat. Designed for the Raspberry
Pi RP2040 and RP2350 microcontrollers.

![PXL_20251105_063120461](https://github.com/user-attachments/assets/f1ea9d1d-33db-42ee-8892-54c7ed03953b)

## Design Objectives

Beyond functional parity with the prior CircuitPython-based Sapling firmware,
this project has a few additional goals:
* Reliable OTAs
* Use the [blackboard pattern](https://en.wikipedia.org/wiki/Blackboard_(design_pattern))
* Eliminate heap allocations
* Simple flight/debug configuration
* Clean code

## Getting Started

### Pre-requisites

1. Install bazelisk by following instructions for your platform here: https://github.com/bazelbuild/bazelisk.
   * You can also use bazel, but this is not recommended.

2. Pull necessary submodules using:

```
git submodule update --init --recursive
```

> Historically, see [the onboarding doc](docs/ONBOARDING.md) for development environment setup (pre-bazel).


## Building
To build the code in this repo, run `bazel build :samwise --config=picubed-debug`.

You can use the pre-provided scripts, `source build_tests.sh` and `source build_debug.sh`, to run automatically.
**NOTE**: These automatically delete the `build_tests` and `build` directories respectively before running, so run with caution!

The following configuration options are available:
* `pico`: pico exectuable
* `pico-hat`: pico-hat executable
* `picubed-debug`: picubed executable, for debugging
* `picubed-flight`: picubed executable, for flight
* `picubed-bringup`: picubed executable, for bringing up the board
(these can be configured in `.bazelrc`)

### Warnings

Warnings are on by default for every profile, and **a warning in our own code
fails the build**. However, warnings only in our own code will fail, and warnings outside (like in libraries) won't.

We however exempt `unused-parameter`, `unused-variable`, `unused-const-variable`, and
`unused-function`. They still print, they just don't fail.

If trying to inspect warnings, it's recommended to pass `-k` into bazel/bazelisk so you can see the full stack without immediately crashing.

#### Seeing the whole backlog

`--config=audit` demotes `-Werror` back to plain warnings, so the build
succeeds and prints everything at once:

```
bazelisk build :samwise --config=picubed-debug --config=audit
```

**NEVER use an audit build as the real one to get around fixing warnings!!**

**Warnings only print for compiles that actually run.** A fully cached build
reports nothing at all, which looks deceptively clean. Force a rebuild of our
sources with `bazel clean`

```
bazelisk clean --expunge
bazelisk build :samwise --config=picubed-debug --config=audit
```

Note that counts differ per profile — `tests` is much noisier than the firmware
profiles because most `unused-parameter` hits are in mocks, and
`picubed-flight` compiles out the bringup paths. Neither is a superset of the
other, so check both.

#### Burning down a category

`--config=strict` re-promotes the four exempted categories, so you can check
whether one has reached zero before deleting its line from `.bazelrc`:

```
bazelisk build //src/... --config=tests --config=strict
```

To work through a single category, promote just that one:

```
bazelisk build //src/... --config=tests --per_file_copt="^src/@-Werror=unused-variable"
```

### Build Archives
The **C Build** github action automatically builds RP2350 archives on pushes to pull requests into main.

# Debugging with OpenOCD on a Pico
We can use the debug probe with a Pico to run `gdb` on real hardware.


```
sudo apt update
sudo apt install libusb-1.0.0-dev libhidapi-dev libjim-dev libftdi-dev
```

Go to some folder (e.g. `~`) and run:
```
git clone https://github.com/raspberrypi/openocd.git
cd openocd
git submodule update --init --recursive
./bootstrap
./configure --disable-werror --enable-cmsis-dap
make -j4
```
