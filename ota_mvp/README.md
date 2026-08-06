# Minimal OTA demonstration code

1. Leverage picotool to prepare multiple partitions on the rp2350
2. Write prog\_b (hello world) -> compile into .elf to get the program bytes
3. Write prog\_a that contains program bytes of prog\_b and writes them into the respective regions of flash
4. prog\_a should also handle making the sys calls necessary to reboot from newly written prog\_b bytes

## Quickstart Guide

### Verifying Try-Before-You-Buy (TBYB)

Load the partition table

```bash
picotool load ota_mvp/pt.uf2 -f
picotool reboot
```

First build the 'main' version of the code that will call reboot into the second partition

```bash
bazel build //ota_mvp:ota --config=pico
picotool load bazel-bin/ota_mvp/ota.uf2 -p 0 -f
```

Then build the new version of the code that will have Try-Before-You-Buy (TBYB) flag set

```bash
bazel build //ota_mvp:ota --config=ota-blink
picotool load bazel-bin/ota_mvp/ota.uf2 -p 1 -f
```

Now you can reboot which will start the code executing in partition 0 for ~6s (with led turned on constantly)

```bash
picotool reboot
```

At the end of ~6s it will call rom_reboot into partition 1 (with led blinking) for around 17s (watchdog timer). Since we never call explicit_buy, this software watchdog timer will reboot us back into partition 0!

### Verifying Partition Flashing

After building the ota-blink binary, we can use xxd to extract the binary bytes into a nicely formatted c header file:

```bash
xxd -i bazel-bin/ota_mvp/ota.bin > ota_mvp/partition_b.h
```

Then upon building the 'main' binary using:

```bash
bazel build //ota_mvp:ota --config=pico
```

We can verify the flashing of code bytes in the main binary works by first erasing the initially loaded partition b code:

```bash
picotool erase -p 1
```

And loading the 'main' binary alone:

```bash
picotool load bazel-bin/ota_mvp/ota.uf2 -p 0 -f
```

Which should only leave behind partition 0 with program bytes:

```bash
% picotool info

Partition 0
 Program Information
  features:      USB stdin / stdout
                 UART stdin / stdout
  binary start:  0x10000000
  binary end:    0x10016080
  target chip:   RP2350
  image type:    ARM Secure

Partition 1
 Program Information
  none

Partition 2
 Program Information
  none
```

On the first run it should print:

```md
[18:40:14.061] Partition Table Information:
[18:40:14.063] ----------------------------
[18:40:14.066] Has partition table: Yes
[18:40:14.069] Number of partitions: 3
[18:40:14.072] === End of Partition Table ===
[18:40:14.075] First words of partition 1:
[18:40:14.078]   [0] 0xffffffff
[18:40:14.080]   [1] 0xffffffff
[18:40:14.082]   [2] 0xffffffff
[18:40:14.084]   [3] 0xffffffff
[18:40:14.086]   [4] 0xffffffff
[18:40:14.088]   [5] 0xffffffff
[18:40:14.090]   [6] 0xffffffff
[18:40:14.092]   [7] 0xffffffff

[18:40:14.094] === Starting Flash Update of Partition B ===
[18:40:14.099] Disabling interrupts to erase 32768 bytes at offset 0x00042000...
[18:40:14.468] Erase complete. Programming 29464 bytes...
[18:40:14.579] === Flash Update Complete ===

[18:40:16.583] BOOT Successful: yes
```

## Build & Execute

### Prepare the partition table

First, we need to "compile" the partition table into a binary format suitable for flashing.

> [!NOTE]
> The RP2350 bootloader looks at the *family id* of the UF2 to determine how to treat it. "Normal" UF2 files have a family ID like `rp2350-arm-ns`, and for those the bootloader tries to place the contents into a partition. If we want to load the partition *itself*, we need to use the `absolute` family ID. For these, the bootloader will always load its contents to the start of flash (or whatever address is specified in the UF2?).

The following command generates an `absolute` UF2 containing the binary format of the partition table:

```bash
picotool partition create pt.json pt.uf2
```

### Load the partition table

Place your RP2350 into `BOOTSEL` mode, then use

```bash
picotool load pt.uf2 [-f]
```

to load the partition table. Now, make sure to reboot so the new one is recognized by the bootloader.

To confirm that it worked, run

```bash
picotool partition info
```

which should print out your loaded partition table

### Compile the OTA demo binaries

The 'main' version of the code which stands in as our stable fallback code is compiled using:

```bash
bazel build //ota_mvp:ota --config=pico
```

This will execute the following:
- print out some information on the partitions currently loaded on the device
- pull the default LED pin HIGH constantly (the light remains on)
- call reboot into Partition 1 after ~5s

The new version of the code which stands in as what we will be transmitting up to the satellite is compiled using:

```bash
bazel build //ota_mvp:ota --config=ota-blink
```

Crucially, this enables the Try-Before-You-Buy behavior in Pico's A/B partitioning scheme using a compile flag `--copt=-DPICO_CRT0_IMAGE_TYPE_TBYB=1` (part of the config ota-blink in .bazelrc).

The demo new version of the code does the following:
- blinks the LED (instead of turning it constantly high)
- 'pets' the software watchdog timer every second or so to prevent the default 16.7s reboot

> [!ERROR]
> To commit to a new partition, we would then proceed to call `rom_explicit_buy`. But we're never going to do that in our current design...

### Select partition to install program into

Once the partition table has been set up, one can select which partition to flash a new program into using the following command:

```bash
picotool load ota.uf2 -p 0 [-f]
```

Which will select the first partition (0). To select the second partition, use `-p 1` instead.

Once again, verify after installation using:

```bash
picotool info [-a] [-f]
```

Which will print out the partitions as well as program loaded into each partition.

```bash
Partition 0
 Program Information
  features:      USB stdin / stdout
                 UART stdin / stdout
  binary start:  0x10000000
  binary end:    0x1000e6a8
  target chip:   RP2350
  image type:    ARM Secure

Partition 1
 Program Information
  features:      USB stdin / stdout
                 UART stdin / stdout
  binary start:  0x10000000
  binary end:    0x10007318
  target chip:   RP2350
  image type:    ARM Secure

Partition 2
 Program Information
  none

# Additional boot information from running `picotool info -a -f`

Device Information
 type:                   RP2350
 revision:               A2
 package:                QFN80
 chipid:                 0x028e0efe4c85f348
 flash devinfo:          0x0c00
 current cpu:            ARM
 available cpus:         ARM, RISC-V
 default cpu:            ARM
 secure boot:            0
 debug enable:           1
 secure debug enable:    1
 boot_random:            288348e1:7ccc7acc:6ca5f430:eaf14f25
 boot type:              bootsel
 last booted partition:  partition 1
 explicit buy pending:   true # <-- set by compiling with PICO_CRT0_IMAGE_TYPE_TBYB=1
 diagnostic source:      partition 0
 last boot diagnostics:  0x00000000
 reboot param 0:         0x00000001
 reboot param 1:         0xffffffff
 rom gitrev:             0x312e22fa
 flash size:             16384K
```
