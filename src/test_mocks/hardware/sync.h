#pragma once

#include <stdint.h>

// Mock hardware sync functions for flash operations
static inline void __compiler_memory_barrier(void)
{
}

/*
 * Interrupt save/restore. Signatures match the pico-sdk
 * (src/rp2_common/hardware_sync/include/hardware/sync.h); on the host there
 * are no interrupts to disable, so the saved state is just an opaque token.
 * Callers must still pair every save with a restore, as on hardware.
 */
static inline uint32_t save_and_disable_interrupts(void)
{
    return 0;
}

static inline void restore_interrupts(uint32_t status)
{
    (void)status;
}
