# Minimal OTA demonstration code

1. Leverage picotool to prepare multiple partitions on the rp2350
2. Write prog\_b (hello world) -> compile into .elf to get the program bytes
3. Write prog\_a that contains program bytes of prog\_b and writes them into the respective regions of flash
4. prog\_a should also handle making the sys calls necessary to reboot from newly written prog\_b bytes

## Build & Execute

```
cmake -B build-ota # builds the static.uf2 version
cmake -B build-ota -DVERSION=BLINK # builds the blink.uf2 version
```

To compile the toy program:
```
cmake --build build-ota --target ota
```

From the repo root to only build this demo app (skips samwise build).

## Prepare the partition table

First, we need to "compile" the partition table into a binary format suitable for flashing.

> [!NOTE]
> The RP2350 bootloader looks at the *family id* of the UF2 to determine how to treat it. "Normal" UF2 files have a family ID like `rp2350-arm-ns`, and for those the bootloader tries to place the contents into a partition. If we want to load the partition *itself*, we need to use the `absolute` family ID. For these, the bootloader will always load its contents to the start of flash (or whatever address is specified in the UF2?).

The following command generates an `absolute` UF2 containing the binary format of the partition table:

```
picotool partition create pt.json pt.uf2
```

## Load the partition table

Place your RP2350 into `BOOTSEL` mode, then use

```
picotool load pt.uf2 [-f]
```

to load the partition table. Now, make sure to reboot so the new one is recognized by the bootloader.

To confirm that it worked, run

```
picotool partition info
```

which should print out your loaded partition table

## Select partition to install program into

Once the partition table has been set up, one can select which partition to flash a new program into using the following command:

```
picotool load blink.uf2 -p 0 [-f]
```

Which will select the first partition (0). To select the second partition, use `-p 1` instead.

Once again, verify after installation using:

```
picotool info [-f]
```

Which will print out the partitions as well as program loaded into each partition.

## Example setup

Two pre-compiled binaries are available, `static.uf2` keeps the built-in LEDturned on while `blink.uf2` blinks the LED.

The two programs also have different names so one can tell them apart when inspecting `picotool info`, e.g:

```
Partition 0
 Program Information
  name:          ota_demo
  version:       0.0
  features:      UART stdin / stdout
                 USB stdin / stdout
  binary start:  0x10000000
  binary end:    0x10007770
  target chip:   RP2350
  image type:    ARM Secure

Partition 1
 Program Information
  name:          ota_blink
  version:       0.1
  features:      UART stdin / stdout
                 USB stdin / stdout
  binary start:  0x10000000
  binary end:    0x10007788
  target chip:   RP2350
  image type:    ARM Secure
```

