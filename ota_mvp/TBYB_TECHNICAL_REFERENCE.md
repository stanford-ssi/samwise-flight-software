# RP2350 Boot System, Partitioning, and TBYB Technical Reference (Revised)

> This is a corrected and expanded version of the original reference document. Corrections
> and additions are called out explicitly where they differ from the original.

---

## 1. Memory Mapping and Physical Flash Layout

The RP2350 coordinates communication with external flash memory via a hardware Query
Memory Interface (QMI) controller. This creates a distinction between the physical storage
layout on the flash chip and the virtual execution space exposed to the processor core.

- **Physical Flash Space (Storage Perspective):** To the external flash memory chip,
  storage begins at absolute byte address `0x00000000`. This is where the primary metadata
  block — the Partition Table — is physically written.

- **CPU Memory Map (Execution Perspective):** The RP2350 processor core addresses
  external flash through a dedicated memory window starting at `0x10000000`. When the
  CPU requests an instruction or data byte from `0x10000000`, the QMI hardware
  transparently maps this request to physical offset `0x00000000` on the flash chip.

---

## 2. picotool Behavior and Target Addressing Constraints

The host-side utility picotool behaves strictly according to the arguments passed to it. It does
not dynamically interpret the execution state of the chip or contextually redirect writes unless
explicitly commanded to do so.

### What `-f` Actually Does [CORRECTION]

> **Original document stated:** "picotool load uses the -f flag to transmit a high-priority USB
> interrupt that cleanly terminates the running application."
>
> This is correct but incomplete. The original framing implied `-f` is just about force-writing.
> The full sequence matters:

The `-f` (force) flag causes picotool to first send a USB command to the running firmware,
which calls `reset_usb_boot()` internally. This drops the CPU's execution context into the
internal ROM bootloader — stopping the application entirely — **before any flash modification
begins**. Only once the chip is running from ROM does picotool begin writing to flash.

Consequences of this:

1. **The running program is fully terminated before any write occurs.** There is no race
   between execution and the flash write.

2. **It is safe to load into the currently running partition.** Because the chip is executing
   from ROM (not flash) during the write, overwriting the active partition's flash region does
   not cause a crash. There is no executing code to corrupt.

3. **The device stays in ROM bootloader mode after the load**, waiting. It does not
   auto-start the newly loaded code. You must issue a separate `picotool reboot` to
   start execution.

### Absolute Writing Without Flags [DANGER]

When a standard application binary (such as a `.uf2` file) is compiled, the linker maps the
executable to run at the default flash window base: `0x10000000`. This target address is
embedded into the metadata header of every individual 512-byte block within the UF2 file.

If you execute a load command without specifying a target partition slot:

```bash
picotool load app.uf2 -f
```

picotool acts as a blind absolute programmer. It reads the destination address embedded
directly in the file blocks (`0x10000000`) and streams them to that exact location. Because the
on-flash partition table also resides at `0x10000000`, **this operation will overwrite and
destroy the partition layout completely**, replacing it with raw application code.

### Partition-Targeted Mode (`-p` Flag)

When a specific partition index is targeted:

```bash
picotool load app.uf2 -p 1 -f
```

picotool reads the existing on-flash partition table to fetch the runtime offsets defined for
Partition Index 1. It then intercepts the incoming UF2 data blocks, ignores their internal
`0x10000000` headers, and dynamically writes them into the boundaries of the chosen partition
slot, leaving Sector 0 (the partition table) untouched.

---

## 3. The Execute-in-Place (XIP) Runtime Flash Constraint

### CORRECTION: Code Does NOT Automatically Run from RAM

> **Common misconception:** "All code is copied into RAM before execution — the program
> counter points into RAM."
>
> **This is incorrect for the RP2350.**

By default, the RP2350 CPU fetches instructions **directly from flash via XIP**. The program
counter points into the `0x10000000` address space, which is the flash window. The QMI
hardware and its XIP cache transparently handle instruction fetches, serving cache hits
instantly and fetching from physical flash on a cache miss. No automatic copy to RAM occurs.

This is different from:
- Desktop systems, where the OS loads executables into RAM before running them.
- Some other embedded MCUs that require an explicit copy-to-SRAM step before execution.

**The exception** is code explicitly marked `__not_in_flash_func()` in the pico-sdk. The
flash write routines (`flash_range_erase`, `flash_range_program`) use this attribute and are
copied into SRAM at startup, so they can execute while flash is bus-locked during write
operations. This is opt-in, not the default.

### The Dual-Use Conflict

A flash sector cannot be read from while **any sector on that same physical chip** is
undergoing an active erase or write operation. Note this is a chip-wide constraint, not just
the sector being written.

If application code attempts to modify flash without properly staging:

1. The flash chip enters an internal write/erase cycle and locks its data lines.
2. The CPU continues executing from its internal XIP cache for a fraction of a microsecond.
3. The moment a cache miss occurs (interrupt, function jump, variable lookup), the QMI
   controller tries to read missing bytes from the locked flash chip.
4. The flash chip returns invalid data (`0xFFFFFFFF`), resulting in an immediate Hard Fault.

### How the pico-sdk Handles This

The pico-sdk's `flash_range_erase` and `flash_range_program` are designed to be called
from within a running application to write to a **different** partition. The pattern used in
`main.c` is:

```c
uint32_t ints = save_and_disable_interrupts();  // prevent any flash-resident ISR from firing
flash_range_erase(target_offset, erase_size);   // runs from SRAM
flash_range_program(target_offset + offset, page_buf, FLASH_PAGE_SIZE);  // runs from SRAM
restore_interrupts(ints);
```

`save_and_disable_interrupts()` ensures no interrupt handler (which may live in flash) fires
mid-write. The write functions themselves execute from SRAM. After `restore_interrupts`,
execution resumes normally from flash.

**This only works safely when writing to a partition different from the one currently executing.**
Writing to your own executing partition would erase the code the CPU returns to after the
write, causing a hard fault on the next cache miss.

---

## 4. Bootrom Execution Flow and Selection Priority

Upon a hardware reset, execution authority lands exclusively within the internal chip Bootrom.
The Bootrom determines which application binary to execute by evaluating structural flags
inside the partition map in memory, independent of the host PC.

| Metric | Evaluation Rule |
|--------|----------------|
| **Boot Priority Parameter** | The Bootrom scans the integer fields assigned to each partition entry. The slot holding the highest numerical priority value that is not flagged as `bad_image` is selected for execution. |
| **Image Generation / Ping-Pong Loops** | Every time an update is deployed, the update utility increments the `boot_priority` integer of the target slot. The Bootrom naturally selects this higher integer at next boot, allowing safe alternation back and forth between slots over generations. **Note: this is a Bootrom capability that SAMWISE deliberately does not use — see "SAMWISE Does Not Ping-Pong" below.** |
| **Application Invalidation** | If a slot fails its probation checks, the Bootrom updates its flag to `bad_image` and lowers its priority. On the secondary pass, it selects the next highest valid partition entry in the pool. |

### SAMWISE Does Not Ping-Pong [CORRECTION]

The ping-pong scheme in the table above describes what the Bootrom *supports*, not what
SAMWISE *does*. Our OTA design is deliberately different:

> **Partition A is a permanent golden image. Partition B is the only slot ever written.**

`ota_task.c` enforces this. If an OTA is requested while the satellite is running from B, it
refuses to flash and reboots back to A instead:

```c
// Running from B. Reboot to A; ground must resend the OTA command.
rom_reboot(BOOT_TYPE_NORMAL, 200, 0, 0);
```

So A is never written by any code path, and every update overwrites B.

**Why we chose this over ping-pong.** With alternation, a sequence of bad updates can
eventually land in both slots and leave no known-good image. With a golden image, an
application-layer bug — however severe — cannot corrupt A, because nothing in the firmware
can write to it. A is always there to fall back on.

**What it costs.**

- **A can never be patched.** Whatever ships at launch is permanent for the mission. A bug in
  A is unfixable in orbit.
- **Updates pass through old code.** Running v2 on B and want v3? The satellite reboots into A
  (v1) first, then writes B from there.
- **Two ground passes when running from B.** The first OTA command only triggers the reboot;
  ground must resend it once A is running. This is worth removing: `rom_reboot`'s two
  parameters survive the reboot and reappear in `boot_info.reboot_params[]`, so B could pass a
  magic value plus the target filename and A could resume the OTA automatically on boot.

Anyone reading the ping-pong row above and assuming both slots get updated will have the wrong
model of how SAMWISE OTA behaves.

---

### TBYB Interaction with Boot Priority [ADDITION]

When the Bootrom selects the highest-priority valid partition and finds a TBYB flag in that
image's header, it **arms a hardware watchdog before jumping to the entry pointer**. The
TBYB partition is still selected first (because it has higher priority from being the newest
load). It does not defer to the stable partition; rather, it runs the new code under watchdog
supervision.

This means: **the bootrom will boot the newly loaded TBYB partition directly on a cold
reboot**, not the stable fallback. The stable partition only runs if the TBYB one is
marked `bad_image` due to a failed probation.

> **Note on the demo flow in `README.md`:** The README describes partition 0 running first
> for ~6s before jumping to partition 1. This is specific to that demo's partition priority
> configuration, not a general TBYB rule. In the production OTA design, the newly received
> TBYB image in partition 1 would have higher boot priority and run first.

---

## 5. Try Before You Buy (TBYB) Architecture

The Try Before You Buy subsystem isolates unstable or unverified firmware updates by
ensuring the device can automatically recover if a newly flashed binary crashes or hangs.

### Location of the TBYB Declaration

The TBYB flag lives inside the **Application Binary's Header** (the picobin-style crt0 entry
block compiled into the application itself), not inside the partition table. It is set at compile
time via:

```
--copt=-DPICO_CRT0_IMAGE_TYPE_TBYB=1
```

which in this repo is part of the `--config=ota-blink` profile in `.bazelrc`.

When the Bootrom parses the binary before execution, it checks the header for
`PICO_CRT0_IMAGE_TYPE_TBYB`. If present, it triggers the watchdog loop.

### The Watchdog and Rollback Process

1. The Bootrom detects the TBYB flag and **arms a hardware watchdog** before jumping to
   the program's entry point.
2. The new program executes. If stable and intended to be permanent, it calls
   `rom_explicit_buy()` to clear probation status and disarm the watchdog.
3. If the program crashes, deadlocks, or otherwise stops petting the watchdog, the timer
   hits zero and issues a hard hardware reset.
4. Post-reset, the Bootrom identifies the probation failure and marks that partition's
   runtime flag as `bad_image`. **The binary itself is never erased** — it is left intact to
   preserve flash write cycles and allow post-mortem debugging.
5. The Bootrom runs its priority selection pass again. The failed slot is excluded, and the
   next highest eligible stable partition runs.

---

## 6. How Samwise Exploits TBYB

By setting the TBYB flag on code received Over-The-Air, the bootrom automatically arms
the watchdog and sets the partition to a "trying" state when it boots.

**We never call `rom_explicit_buy()`** — any new code is always in probation. This is
intentional: bugs may only surface under specific conditions, and we never want to
permanently commit to new code without ground confirmation.

### The Watchdog Petting Strategy [CORRECTION TO README]

> **What `README.md` says:** "Since we never call explicit_buy, this software watchdog
> timer will reboot us back into partition 0 after ~17s."
>
> **This is misleading.** The watchdog firing after 17s would only happen if the code does
> NOT pet the watchdog. The actual code in `main.c` does pet it:

```c
// Inside the while(1) loop in the BUILD_BLINK branch:
watchdog_enable(MAX_WATCHDOG_TIMEOUT_MS, false);  // arms 16.7s timer every iteration
watchdog_update();                                 // resets the countdown every ~1s
```

This is **intentional by design**, not a bug. The strategy is:

- **If new code is running normally**: it pets the watchdog every loop iteration (~1s),
  extending its runtime indefinitely. Ground operators can observe it, validate behavior,
  and decide whether to keep it running or command a rollback.
- **If new code crashes or enters an infinite loop without petting**: the watchdog fires,
  the Bootrom marks it `bad_image`, and the stable partition takes over automatically.

The safety guarantee comes from **the code crashing or hanging**, not from a fixed time
limit. The README's "~17s" description only applies if watchdog petting is deliberately
removed (as in the TBYB verification demo test case).

### Summary: Failure vs Success Paths

| State | Outcome |
|-------|---------|
| New code runs and pets watchdog | Stays in partition 1 indefinitely; ground confirms |
| Ground commands explicit rollback | Call appropriate ROM function to switch back |
| New code crashes / hangs | Watchdog fires → `bad_image` → partition 0 resumes |
| New code calls `rom_explicit_buy()` | Permanently committed — **we never do this** |

---

## 7. Safe Loading Into the Active Partition [NEW SECTION]

A natural concern is: what happens if `picotool load -f -p 0` is issued while partition 0
is currently running?

Because `-f` terminates the running program and drops into the ROM bootloader **before
any flash write begins**, this is safe. The chip is not executing from flash during the write.
After the load completes, the device sits in bootloader mode. A subsequent `picotool reboot`
starts the newly written code.

The dangerous case is **a running application calling `flash_range_erase` on its own
partition's address range** (as the OTA code does in `main.c`, but targeting partition 1's
offset `0x42000`). Writing to your own executing region would erase the code the CPU
must return to after the write, causing a hard fault.

Mitigation in `main.c`: the target offset is hardcoded to `0x42000` (partition 1). A
production-hardened version should validate that the target offset does not overlap with
the currently executing partition before calling any flash write functions.

---

## 8. Build Configuration Reference [NEW SECTION]

| Config | Board | TBYB | Optimization | `BUILD_BLINK` | Purpose |
|--------|-------|------|-------------|----------------|---------|
| `--config=pico` | `samwise_pico` | off | `-Og` debug | not set | Stable partition 0 / OTA initiator |
| `--config=ota-blink` | `samwise_picubed` | on (`TBYB=1`) | `-Os` size | set | New OTA payload for partition 1 |

The `BUILD_BLINK` preprocessor flag is what selects which branch of `main.c` runs — the
same source file compiles into both partition images.