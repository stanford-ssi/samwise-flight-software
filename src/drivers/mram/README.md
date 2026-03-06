# MRAM Driver — Proposed Changes

This document lists known discrepancies between `mram_mock.c` and `mram.c` that need
to be fixed, along with the downstream test impact of each.

---

## Issues in `mram_mock.c`

### 1. `mram_write` — missing `length > 256` guard

**Real driver:** returns `false` immediately if `length > 256`.
**Mock:** only checks `address + length > MOCK_MRAM_SIZE`, so a 257-byte write
succeeds and returns `true`.

**Impact:** `test_mram_write_exceeds_max_length` fails — it asserts the return
value is `false` for a 257-byte write.

**Fix:** add the guard before the bounds check:
```c
if (length > 256)
    return false;
```

---

### 2. `mram_read` — missing `length > 256` guard

**Real driver:** returns early (no-op) if `length > 256`.
**Mock:** only checks `address + length > MOCK_MRAM_SIZE`, so reads larger than
256 bytes succeed silently.

**Impact:** no current test fails, but the mock does not faithfully represent the
real driver's constraint.

**Fix:** add the guard before the bounds check:
```c
if (length > 256)
    return;
```

---

### 3. `mram_clear` — missing `length > 256` guard

**Real driver:** returns early (no-op) if `length > 256`.
**Mock:** only checks `address + length > MOCK_MRAM_SIZE`, so an oversized clear
still zeroes memory.

**Impact:** `test_mram_clear_exceeds_max_length` would fail — it writes a known
pattern, calls `mram_clear(addr, 257)`, and asserts the data is unchanged.

**Fix:** add the guard before the bounds check:
```c
if (length > 256)
    return;
```

---

### 4. `mram_write` — mock incorrectly gates writes on `write_enabled`

**Real driver:** both `mram_write` and `mram_clear` call `mram_write_enable()`
*internally* before sending the write command. This means calling
`mram_write_disable()` followed by `mram_write()` on real hardware still
**succeeds** — the internal `mram_write_enable()` overrides the prior disable.

**Mock:** `mram_write` checks `!write_enabled` at the top and returns `false` if
writes are disabled. This is the opposite behavior.

**Impact:**
- `mram_write_disable()` + `mram_write()` returns `false` on mock, `true` on
  hardware.
- `test_mram_write_disable_enable` passes on the mock but tests behavior that
  does not occur on real hardware. The test is effectively validating an
  incorrect mock contract.

**Fix:** remove the `!write_enabled` guard from `mram_write`. If gating on
write-enable state is desired for testing purposes, it must model the real
hardware sequence: `mram_write` should set `write_enabled = true` internally
(mirroring the real driver's internal `mram_write_enable()` call) before
performing the write, making `mram_write_disable` irrelevant to subsequent
writes.

---

### 5. `mram_clear` — does not model the write-enable step

**Real driver:** `mram_clear` calls `mram_write_enable()` internally before
clearing, meaning a clear always succeeds regardless of prior
`mram_write_disable()` calls.
**Mock:** `mram_clear` does not check `write_enabled` at all, and does not call
`mram_write_enable()`. The outcome (clear always succeeds) is incidentally
correct, but the mechanism does not match the real driver.

**Fix:** add an internal `write_enabled = true` assignment inside `mram_clear`
to mirror the real driver's internal `mram_write_enable()` call.

---

### 6. `mram_init` — incorrectly sets `write_enabled = true`

**Real driver:** `mram_init` sends a wake command and waits 400 µs. It does not
call `mram_write_enable()`. On real hardware, WEL (Write Enable Latch) defaults
to 0 after power-on or wake.
**Mock:** `mram_init` sets `write_enabled = true`, which does not match the
default WEL = 0 state of the real chip after initialization.

**Impact:** tests that rely on the write-enabled state immediately after
`mram_init` would behave differently on hardware vs. mock.

**Fix:** set `write_enabled = false` in `mram_init`.

---

### 7. `mram_wake` — does not reset `write_enabled` to the post-wake state

**Real driver:** after waking from sleep, the chip's WEL bit is 0 (hardware
default).
**Mock:** `mram_wake` only prints a message, leaving `write_enabled` in whatever
state it was before `mram_sleep()` was called.

**Impact:** if tests call `mram_write_enable()`, then `mram_sleep()`, then
`mram_wake()`, the mock leaves `write_enabled = true` — but real hardware would
have WEL = 0 after wake, requiring an explicit `mram_write_enable()` call before
the next write.

**Fix:** set `write_enabled = false` in `mram_wake`.

---

### 8. Stale functions no longer in the public API

The following functions were removed from `mram.h` but remain defined in
`mram_mock.c`:

- `mram_allocation_init`
- `mram_ranges_overlap`
- `mram_register_allocation`
- `mram_check_collision`
- `mram_free_allocation`

**Impact:** no compile failure (they are orphaned definitions), but they leave
the mock out of sync with the driver API and add dead code.

**Fix:** remove all five functions from `mram_mock.c`.

## Potential issues
Hardware-Specific Reasons the File Would Be Shorter Than Declared

  1. Silent mram_write success on a failed SPI transaction
  mram_write unconditionally returns true even after flash_do_cmd. If the SPI transaction fails (wrong pin config, wrong bus, timing violation), LFS sees success from lfs_mram_wrap_prog and believes the file data was committed. On the subsequent open for reading, LFS reads back old/uninitialized MRAM content.
  Metadata on-disk could show file_size = 0 (the pre-write state), so lfs_file_read returns 0 immediately.

  2. lfs_mram_wrap_erase is a no-op, leaving stale LFS state
  LFS assumes erased blocks read as 0xFF. For MRAM, the erase is a no-op. If the MRAM had LFS metadata from a prior run (e.g., from a previous partial write that left a corrupt superblock), lfs_format writes the new superblock on top but adjacent blocks still carry old, potentially contradictory metadata. LFS
  might read the old metadata path and find a file with 0 or partial content.

  3. WEL bit cleared between the lfs_file_write prog calls
  MRAM hardware auto-clears the WEL bit after each WRITE command completes. mram_write correctly calls mram_write_enable() before each 16-byte chunk. But if there is a setup-time violation (WEL must be asserted for some tWREN before the WRITE opcode arrives, and the back-to-back SPI commands are too fast), one
  prog call might land without WEL set. The MRAM silently ignores it. The file ends up short, hitting EOF mid-read.

  4. flash_do_cmd being the wrong API for the MRAM bus
  mram.c uses hardware/flash.h's flash_do_cmd, which drives the RP2350's QSPI/XIP interface. If the MRAM is connected to a separate GPIO-bitbanged or alternate SPI bus, flash_do_cmd talks to the wrong device entirely. Writes appear to succeed (no return value check); reads return whatever the wrong device (or
  nothing) replies — likely zeros or 0xFF — and LFS sees a filesystem that was never actually written.

  5. MRAM not woken before use (or woken too briefly)
  mram_init sends WAKE (0xAB) and waits 400 µs. If the MRAM was in deep sleep and 400 µs is marginal for the specific part, the chip might still be initializing when the first lfs_format write arrives. Writes during the wake period are silently discarded by the MRAM. The result is an empty filesystem, and any
  subsequent lfs_file_read immediately returns 0.

  6. SPI bus contention corrupting the static command buffer
  mram_write and mram_read both use function-local static uint8_t cmd_buf[]. If an ISR fires between memcpy(&cmd_buf[4], data, length) and save_and_disable_interrupts() and that ISR also calls mram_write (e.g., a radio ISR flushing a log), the static cmd_buf gets overwritten before flash_do_cmd runs, corrupting
  the address or data bytes mid-transaction. The write goes to the wrong MRAM address, leaving the intended location unwritten and the file shorter than expected.

  7. Partial write due to LFS block boundary and block_cycles compaction
  With block_size = 256 and block_cycles = 500, LFS will attempt to relocate a metadata block if it has been written 500 times. If a compaction is triggered mid-write (possible if MRAM was not freshly formatted) and the compaction's prog calls fail silently (reason 1), LFS ends up with a partially-committed file.
   The declared file_size in the slate is the full 64 bytes, but the actual committed content is less.

  ---
  Summary Table

  ┌─────┬────────────────────────────────────┬─────────────────────────────────────────┐
  │  #  │              Trigger               │           Where it manifests            │
  ├─────┼────────────────────────────────────┼─────────────────────────────────────────┤
  │ 1   │ mram_write always returns true     │ Silent write failure → file has 0 bytes │
  ├─────┼────────────────────────────────────┼─────────────────────────────────────────┤
  │ 2   │ No-op erase → stale LFS metadata   │ Old metadata shows 0-byte file          │
  ├─────┼────────────────────────────────────┼─────────────────────────────────────────┤
  │ 3   │ WEL setup-time violation           │ One prog chunk missed → short file      │
  ├─────┼────────────────────────────────────┼─────────────────────────────────────────┤
  │ 4   │ flash_do_cmd on wrong bus          │ All writes lost → file has 0 bytes      │
  ├─────┼────────────────────────────────────┼─────────────────────────────────────────┤
  │ 5   │ MRAM not fully awake during format │ Superblock not written → empty FS       │
  ├─────┼────────────────────────────────────┼─────────────────────────────────────────┤
  │ 6   │ Static SPI buffer corrupted by ISR │ Wrong address written → short file      │
  ├─────┼────────────────────────────────────┼─────────────────────────────────────────┤
  │ 7   │ LFS compaction mid-write failure   │ Partial content committed → short file  │
  └─────┴────────────────────────────────────┴─────────────────────────────────────────┘

  The fix is straightforward: add if (bytes_read == 0) { break; } (or return an error) after the bytes_read < 0 check in the loop.
