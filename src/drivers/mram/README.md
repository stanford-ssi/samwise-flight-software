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
