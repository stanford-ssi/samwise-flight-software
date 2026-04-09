# Sat Software Onboarding

Welcome to software! We put code **in space**!!

# Part -1: History

First, some context.

Long ago, SSI designed a CubeSat "framework" called the PyCubed. It was great,
because it was a "commercial off-the-shelf" satellite, meaning we could build it
quickly and reliably.

While much of the original design had changed up until this year, one design
decision stuck with us: CircuitPython for our flightcode.

Over time, the benefits of Python as a language began to pale in comparison to
its costs: implicit heap allocations caused random (catastrophic) errors, lack
of proper interrupt support meant lots of polling, lack of explicit static
memory allocation made the aforementioned heap allocations worse, etc.

This year, we decided to finally pivot and **_rewrite all flight
software in C_**.

We _also_ switched microcontrollers from the SAMD51 (which was mostly hidden
from us by CircuitPython) to the lovely RP2350 (A.K.A. the Raspberry Pi Pico 2).

What does that mean for **_you_**? Lots of opportunity to build on what you
learned in CS 106A, 106B, 107, 107E, etc. etc. and build some **real satellite
flight code**. Not only is this a _great_ learning experience, but it's
**great** on a resume! SSI software alumni have gone on to NASA, SpaceX, and
grad school ;)

We're going to use our actual flight software on a special build (`PICO`) that runs some minimal code!

# Part 0: Install

### MacOS

You will need to use the terminal to install some programs. If you are not
familiar with a terminal, I recommend first installing [Visual Studio
Code](https://code.visualstudio.com/). Then, you should use the VS Code
integrated terminal (tutorial
[here](https://code.visualstudio.com/docs/terminal/basics)). Ubuntu has a neat
[tutorial](https://ubuntu.com/tutorials/command-line-for-beginners#4-creating-folders-and-files) on terminal usage - ignore the Ubuntu specific stuff.

Install Homebrew if you don't already have it:

```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

After installing Homebrew, close and re-open your terminal.

Install compiler toolchain:

```
brew install --cask gcc-arm-embedded
```

Again, after installing both, close and re-open your terminal.

### Linux

For [ubuntu/linux](https://askubuntu.com/questions/1243252/how-to-install-arm-none-eabi-gdb-on-ubuntu-20-04-lts-focal-fossa), ARM recently
changed their method for distributing the tool change. Now you
must manually install. As of this lab, the following works:

```
wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2

sudo tar xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /usr/local/
```

We want to get these binaries on our `$PATH` so we don't have to type the
full path to them every time. There's a fast and messy option, or a slower
and cleaner option.

The fast and messy option is to add symlinks to these in your system `bin`
folder:

```
sudo ln -s /usr/local/gcc-arm-none-eabi-10.3-2021.10/bin/* /usr/bin/
```

The cleaner option is to add `/usr/local/gcc-arm-none-eabi-10.3-2021.10/bin` to
your `$PATH` variable in your shell configuration file (e.g., `.zshrc` or
`.bashrc`), save it, and `source` the configuration. When you run:

```
arm-none-eabi-gcc
arm-none-eabi-ar
arm-none-eabi-objdump
```

You should not get a "Command not found" error.

If gcc can't find header files, try:

```
sudo apt-get install libnewlib-arm-none-eabi
```

### Windows

Unfortunately, I have not gotten to fully supporting Windows. The best option is
to install the [compiler toolchain](https://developer.arm.com/downloads/-/gnu-rm) according to its respective websites.

~~[WIP] we'll working on getting support via VSCode RPi extension ([PR](https://github.com/stanford-ssi/samwise-flight-software/pull/153))~~

[WIP] We can try `MSYS2 + MinGW + `https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads to get windows support, but this has never been tried before.

Tentative steps:

1. Build the project
2. Use `picotool` to flash onto the PICO
   - If on WSL, use `usbipd`
3. Finally, use `tio` to listen

# Part 1: Bazel, Building & Project Structure

Look around the `samwise-flight-software` repository, and look for `BUILD.bazel` or any `*.bazel` files. Try to understand them - reading through can be really helpful.

In general:

- `bazel` is a build system that allows you to automatically link and compile files easily, similar to `cmake` (but MUCH better!)
- Features of `bazel`: tests, caching (so we don't have to compile everything every time), `genrule`s (to get into later), and much more
- `.bazelrc` is where we configure `bazel`, including the different build profiles.
  - We will focus on `pico`, which is what we are playing with, but `picubed-flight` is the final version that will go onto our satellite!
  - We also link `pico-sdk` here, which allows us to use the library with the pico/raspi/etc.
- `BUILD.bazel` specifies _targets_, i.e. a component that needs to be compiled, along with its _dependencies_, which are the subcomponents it requires to function.
  - The root `BUILD.bazel` is our actual binary `samwise`, and depends on essentially all code in the repository (of course!)
  - Then, each path in `dependencies` as shown in `//BUILD.bazel` has its own `BUILD.bazel` file - for example, `//src/tasks/print/BUILD.bazel` has a `print_task`, which simply prints a message every few seconds, and it depends on `logger`, for example.
  - We can use `select` to change dependencies based on the current configuration - this is how we change loggers, for example, if we are on a computer or if we are on `pico`.

Please ask if you have any questions!

# Part 2: First PICO Build

All microcontrollers come with some pins you can turn on and off, communicate
over, etc. These pins are called _general purpose input/output pins_ or GPIO
pins. You can configure them from software.

Generally, these are the steps:

- Initialize the pin
- Set the direction (input or output) of the pin
- Write (if it's output) or read (if it's input) from the pin

Some GPIO pins are connected to physical wires on the board, whereas others are
routed to built-in components. For this example, we are going to use pin 25,
which is routed to an LED on the board. This magic number is provided with a
convenient name in the SDK: `PICO_DEFAULT_LED_PIN`.

Microcontrollers also come with some timing facilities. For now, all we need to
use is the sleeping methods, which are exactly like the ones from Python.

## Git

You should create a clone of [this repository](https://github.com/megargayu/ssi-onboarding-26/) using VSCode
(or a git client of your choice). If you are using VSCode, they provide a
[tutorial](https://code.visualstudio.com/docs/sourcecontrol/intro-to-git) on how
to do this.

The rest of this guide takes place from within this folder/repository.

Note that the way that this is setup is almost identical to `samwise-flight-software`. There is a lot of stuff in there, so read its `README` to understand it better!

## Code

Put this in `src/main.c`:

```c
#include "pico/stdlib.h"

int main() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    while (1) {
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(250);
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(1000);
    }
}
```

## Building

Run:

```bash
bazel build :ssi-onboarding-26 --config=pico
```

Which should generate a bunch of folders, including most importantly `bazel-bin/ssi-onboarding-26.uf2`, which is the actual binary! This is essentially the same command for `SAMWISE`, except we use `:samwise` (a different target name) and various configurations, as explained briefly above (`.bazelrc`).

## Uploading

1. Unplug your PICO
2. Hold the BOOT button on your PICO
3. While holding, plug it in. A window should pop up with a directory that corresponds to the PICO, or it should generally be available in Finder/Windows Explorer/etc.
4. Copy `bazel-bin/ssi-onboarding-26.uf2` into the drive that shows up.
5. The folder should close almost immediately, and after the pico reboots, the light should start to blink!

If all of this works, success!

## More

Please keep reading the README.md in the onboarding docs for more in depth and detailed steps! We will not reproduce all of it here.

# Part 3: Samwise Software!

NOTE: If you made it so far, GREAT!

We can build using:

```bash
bazel build :samwise --config=picubed-debug
```
Which builds a debug build for the actual satellite.

Try to compile and build our main [repository](https://github.com/stanford-ssi/samwise-flight-software/tree/main/src#building-from-source) you should get a `bazel-bin/samwise.uf2` generated (SUCCESS!).

## Code Structure

This [overview](https://github.com/stanford-ssi/samwise-flight-software/tree/main/src#code-structure) provides an idea of how our repository is organized.

## Adding new states/tasks

The flight software is organized into a state machine. The satellite can be in
any of several states e.g. "running," "low power," etc.

Each state is associated with tasks that are run regularily (e.g. every half a
second) while the satellite is in that state e.g. the running state might be
associated with a "beacon" task.

If a task is associated with the active state, its `task_dispatch` function is
called at most every `dispatch_period_ms` milliseconds. Additionally, its
`task_init` function is called as part of satellite boot.

You will also see references to a "slate." The slate is a huge block of
statically allocated memory.

Moving forward, you are going to write your own tasks and test it!
