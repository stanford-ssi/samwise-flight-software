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

You will need to use the terminal to install some programs. If you are not
familiar with a terminal, I recommend first installing [Visual Studio
Code](https://code.visualstudio.com/). Then, you should use the VS Code
integrated terminal (tutorial
[here](https://code.visualstudio.com/docs/terminal/basics)). Ubuntu has a neat
[tutorial](https://ubuntu.com/tutorials/command-line-for-beginners#4-creating-folders-and-files) on terminal usage - ignore the Ubuntu specific stuff.

## MacOS - Homebrew

On MacOS, we will use Homebrew to install everything. This is an extremely useful package manager.

Install Homebrew if you don't already have it:

```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

After installing Homebrew, close and re-open your terminal.

## Bazelisk

First, we need to install Bazelisk, our build system manager. You can install it on MacOS with Homebrew:

```bash
brew install bazelisk
```

Or find your binary on the [Github Releases Page](https://github.com/bazelbuild/bazelisk/releases).

## Picotool

This is technically optional, but highly recommended for ease of use. Picotool essentially allows you to work directly with the pico without unplugging/plugging/pressing buttons, which can save a lot of time.

### Linux & WSL2

You can find binaries at the [`pico-sdk-tools Github`](https://github.com/raspberrypi/pico-sdk-tools/releases).

I recommend putting your relevant binary in `/usr/local/bin/picotool` and then adding `/usr/local/bin` to `PATH` if it's not there already.

1. To check PATH, run `echo $PATH | grep /usr/local/bin`. If the output has a highlighted version of that string, then you don't need to do anything.
2. To add to PATH, edit `~/.bashrc` or `~/.zshrc` if you are using `zsh`, and add the line:
   ```bash
   export PATH="/usr/local/bin:$PATH"
   ```
   Then restart your shell.

### Mac

`brew install picotool` should work.

### Windows

Should be similar to Linux, but I've never tried it before. (Good luck!)

## Tio

This software allows you to communicate with a pico and read its logs.

### Linux

Run `sudo snap install tio --classic`.

If you don't have `snap`, install it, for example, on Ubuntu:

```bash
sudo apt update
sudo apt install snapd
```

Note you may have to add snap's bin to path - so in `.bazelrc` or `.zshrc`:

```bash
export PATH="/snap/bin:$PATH"
```

### Mac

`brew install tio`

### Windows

No idea :)

## ARM GCC

We need a specific version of GCC for compilation onto pico.

### MacOS

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

## Moving on

Now we should be done with installation! Here are our
tentative steps:

1. Build the project
2. Drag and drop to flash onto the PICO
3. Use `picotool` to flash onto the PICO
   - If on WSL, use `usbipd`
4. Finally, use `tio` to listen

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
    stdio_usb_init();

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
3. While holding, plug it in. A window should pop up with a directory that corresponds to the PICO, or it should generally be available in Finder/Windows Explorer/etc. On Mac, a drive called "RP2350" or similar should appear on your desktop.
4. Copy `bazel-bin/ssi-onboarding-26.uf2` into the drive that shows up.
5. The folder should close almost immediately, and after the pico reboots, the light should start to blink!

If all of this works, success!

## Uploading (Cool version)

If you are not all about that drag-and-drop life:

1. Unplug your PICO
2. Hold the BOOT button on your PICO
3. While holding, plug it in. A window should pop up with a directory that corresponds to the PICO, or it should generally be available in Finder/Windows Explorer/etc. On Mac, a drive called "RP2350" or similar should appear on your desktop.
4. In the repository, run:

   ```bash
   picotool load bazel-bin/ssi-onboarding-26.uf2 -f
   ```

   You may have to click the "RESET" button after doing this - but the LED should start blinking!

   Note that you may have to put `sudo` at the beginning if it doesn't work.

   Additionally, if you are on `WSL`, you must use `usbipd` (see instructions below)

Note this is cool but also useful. You no longer need to reattach and attach the stick now. If you repeat step 5 again and again, it will automatically force the pico to reboot and then flash the new code. Try it out yourself - edit `main.c` and only run step 4 again! (Make sure to build first!).

## WSL2 Integration

If you are on WSL2, you also have to do an additional step:

Your pico is only accessible from Windows by default. In order for WSL to see it, you have to tell Windows to link it into WSL.

Every time you connect the Pico and either want to `tio` or `picotool`, therefore:

1. Unplug your PICO
2. Hold the BOOT button on your PICO
3. While holding, plug it in. A window should pop up with a directory that corresponds to the PICO, or it should generally be available in Finder/Windows Explorer/etc. On Mac, a drive called "RP2350" or similar should appear on your desktop.
4. From powershell, run `usbipd list`. You should see a bunch of stuff, including:

```
BUSID  VID:PID    DEVICE                                                        STATE
2-3    2e8a:000f  USB Mass Storage Device, RP2350 Boot                          Not shared
```

Each `BUSID` is a port on your computer. Therefore, it usually is the same if you plug it into the same port, but can change around otherwise.

5. If `STATE == Not shared`, then run `usbipd bind --busid {your-bus-id}`. For example, I would use `usbipd bind --busid 2-3` for myself. Usually, you only need to do this once, unless you change ports, update, etc.

6. Now that state is shared, run:

   ```powershell
   usbipd attach --wsl --busid {your-bus-id} --auto-attach
   ```

   It should output like:

   ```
   usbipd: info: Using WSL distribution 'Ubuntu' to attach; the device will be available in all WSL 2 distributions.
   usbipd: info: Loading vhci_hcd module.
   usbipd: info: Detected networking mode 'nat'.
   usbipd: info: Using IP address 172.17.192.1 to reach the host.
   usbipd: info: Starting endless attach loop; press Ctrl+C to quit.
   WSL Monitoring host 172.17.192.1 for BUSID: 2-3
   WSL 2026-04-08 17:39:21 Device 2-3 is available. Attempting to attach...
   WSL 2026-04-08 17:39:21 Attach command for device 2-3 succeeded.
   ```

   And also make the signature Windows detach noise (indicating it is no longer attached to Windows).

   This will automatically repeatedly attach your pico to WSL, until you quit the program (i.e. CTRL+C). So if you reboot or reflash any software, it will keep trying to attach your pico to WSL.

7. Now, `picotool info` (or `sudo picotool info`) should say something like:
   ```
   Program Information
   binary start:  0x10000000
   binary end:    0x10003cd4
   target chip:   RP2350
   image type:    ARM Secure
   ```
   And running step 5 in "Uploading (Cool version)" should now produce a blinking LED! Note that it will make the attach/detach noise a lot of times as the `--auto-attach` in the Powershell window will force it to attach to WSL as soon as possible. This may be annoying; you may have to deal with it :(.

## Reading from PICO

What if we want to read logs in real time?

1. Now, let's edit `main.c` to actually log a counter:

   ```c
   #include "pico/stdlib.h"
   #include "pico/printf.h"

   int main() {
       stdio_usb_init();
       int counter = 0;

       gpio_init(PICO_DEFAULT_LED_PIN);
       gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
       while (1) {
           gpio_put(PICO_DEFAULT_LED_PIN, 0);
           sleep_ms(250);
           gpio_put(PICO_DEFAULT_LED_PIN, 1);
           sleep_ms(1000);

           printf("Hello, world! Our counter is: %d\n", counter);
           counter++;
       }
   }
   ```

2. Now, before we flash, let's setup `tio` by running (note you may have to add `sudo` to the start of any of these commands if you get a "permission denied"):
   - Linux/WSL: It should be `tio /dev/ttyACM0`. If not, try repeating similar steps to Mac below, or just looking for an item under `/dev/` that appears when you plug in the pico.
   - Mac: Try typing `tio /dev/tty.`, then pressing the "TAB" button. You should see a list of items appear - type in the item that starts with `usbmodem` and ends with some number.

     If there are multiple options, try pressing tab before you plug in, and then pressing it after, and seeing which item appeared (which should be the correct one).

     If nothing appears, but your pico is flashing, something is wrong with your port. Try using a different cord or flipping the USB-C Cable (yes, that sometimes works)!

3. Now, either use method (a) or the cool version to build and flash the new code onto the pico.
4. You should see something like this!:

   ```
   [17:56:05.115] Waiting for tty device..
   [17:56:09.122] Connected to /dev/ttyACM0
   Hello, world! Our counter is: 2
   Hello, world! Our counter is: 3
   Hello, world! Our counter is: 4
   Hello, world! Our counter is: 5
   Hello, world! Our counter is: 6
   Hello, world! Our counter is: 7
   Hello, world! Our counter is: 8
   Hello, world! Our counter is: 9
   Hello, world! Our counter is: 10
   ```

   Note that sometimes the first few logs are not available as `tio` connects after the code already started running on the pico.

# Part 3: Samwise Software!

NOTE: If you made it so far, GREAT!

Try to compile and build our main [repository](https://github.com/stanford-ssi/samwise-flight-software/tree/main/src#building-from-source) you should get a `bazel-bin/samwise.uf2` generated (SUCCESS!).

We can build using:

```bash
bazel build :samwise --config=picubed-debug
```

Which builds a debug build for the actual satellite.

Note that if you try to flash this onto your pico, it will cause an error, which is correct!
This happens because the `picubed-debug` build assumes more components are plugged in/available than the pico has, like the radio, and so it crashes when it can't initialize/find them. Therefore, we have to build for pico itself, which disables these features for testing:

```bash
bazel build :samwise --config=pico
```

And then you can flash with the steps provided above! If everything works right, you should get a `print task` that outputs a counter, and periodically a lot of data being spat out (i.e. hardware test task)!

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

# Part 4: What's next?

You've successfully been onboarded! Now, I would suggest trying to get an understanding of our code base. Look through files, understand what's being outputted on the pico, and read the explanation provided of the files in our onboarding code base [here](https://github.com/megargayu/ssi-onboarding-26/).
