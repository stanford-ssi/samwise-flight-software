/**
 * RP2350 Three-Partition Bootloader
 * 
 * ARCHITECTURE:
 * - BOOTLOADER (this code): Always boots first, never changes
 * - PARTITION A: Stable baseline + OTA update manager
 * - PARTITION B: New features, experimental
 * 
 * BOOT FLOW:
 * 1. Bootloader always runs first
 * 2. Check if B crashed during init → boot A
 * 3. Check if B was running and rebooted → increment counter
 * 4. Check if B is valid → boot B with crash detection enabled
 * 5. Otherwise → boot A (safe fallback)
 * 
 * CRASH DETECTION:
 * - Layer 1: Init crash flag (scratch[4]) - catches startup crashes
 * - Layer 2: Running flag (scratch[6]) - catches runtime crashes
 * - Layer 3: Hardware watchdog - catches hangs/deadlocks
 */

#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "hardware/sync.h"
#include <string.h>
#include <stdio.h>
#include <pico/bootrom.h>
#include <pico/platform.h>

/* ============================================================================
 * ARM INTRINSICS
 * ============================================================================ */

// ARM Cortex-M0+ intrinsic functions
static inline void __disable_irq(void) {
    __asm volatile("cpsid i" : : : "memory");
}

static inline void __set_MSP(uint32_t topOfMainStack) {
    __asm volatile("MSR msp, %0" : : "r" (topOfMainStack) : "sp");
}

static inline void __DSB(void) {
    __asm volatile("dsb" : : : "memory");
}

static inline void __ISB(void) {
    __asm volatile("isb" : : : "memory");
}

/* ============================================================================
 * MEMORY LAYOUT
 * ============================================================================ */

#define FLASH_BASE              0x10000000

// Bootloader partition (this code)
#define BOOTLOADER_BASE         FLASH_BASE
#define BOOTLOADER_SIZE         (256 * 1024)    // 256KB

// Partition A: Stable baseline application
#define PARTITION_A_BASE        (FLASH_BASE + BOOTLOADER_SIZE)
#define PARTITION_A_SIZE        (896 * 1024)    // 896KB

// Partition B: New features/updates
#define PARTITION_B_BASE        (PARTITION_A_BASE + PARTITION_A_SIZE)
#define PARTITION_B_SIZE        (896 * 1024)    // 896KB

/* ============================================================================
 * CRASH DETECTION REGISTERS
 * ============================================================================ */

// Watchdog scratch register assignments
#define CRASH_FLAG_REGISTER         4  // Init crash detection
#define BOOT_COUNT_REGISTER         5  // Boot attempt counter
#define PARTITION_B_RUNNING_REGISTER 6  // Runtime crash detection

// Magic values
#define CRASH_MAGIC                 0xDEADBEEF  // Init crash marker
#define PARTITION_B_RUNNING_MAGIC   0xB00710AD  // Runtime marker

// Max boot attempts before giving up on partition B
#define MAX_BOOT_ATTEMPTS           3

/* ============================================================================
 * PARTITION VALIDATION
 * ============================================================================ */

// Vector table structure at start of each partition
typedef struct {
    uint32_t stack_ptr;      // Initial stack pointer
    uint32_t reset_handler;  // Reset vector (entry point)
    // ... more interrupt vectors follow
} vector_table_t;
 
static bool is_partition_valid(uint32_t partition_addr) {
    vector_table_t *vectors = (vector_table_t *)partition_addr;
    
    // Check 1: Stack pointer should be in RAM (0x20000000 - 0x20042000)
    uint32_t sp = vectors->stack_ptr;
    if (sp < 0x20000000 || sp > 0x20042000) {
        printf("  Invalid stack pointer: 0x%08lx\n", sp);
        return false;
    }
    
    // Check 2: Reset handler should be in flash (0x10000000 - 0x10200000)
    uint32_t reset = vectors->reset_handler;
    if (reset < FLASH_BASE || reset > (FLASH_BASE + 0x200000)) {
        printf("  Invalid reset handler: 0x%08lx\n", reset);
        return false;
    }
    
    // Check 3: Reset handler should have Thumb bit set (bit 0 = 1)
    // ARM Cortex-M always runs in Thumb mode
    if (!(reset & 0x1)) {
        printf("  Reset handler missing Thumb bit\n");
        return false;
    }
    
    return true;
}

/* ============================================================================
 * CRASH DETECTION FUNCTIONS
 * ============================================================================ */

/**
 * Check if partition B crashed during initialization
 * 
 * If crash flag is still set, B never cleared it = crashed during init
 */
static bool check_partition_b_init_crash(void) {
    uint32_t crash_flag = watchdog_hw->scratch[CRASH_FLAG_REGISTER];
    uint32_t boot_count = watchdog_hw->scratch[BOOT_COUNT_REGISTER];
    
    if (crash_flag == CRASH_MAGIC) {
        printf("  Init crash detected (attempt %lu/%d)\n", 
               boot_count, MAX_BOOT_ATTEMPTS);
        
        if (boot_count >= MAX_BOOT_ATTEMPTS) {
            printf("  Max boot attempts reached\n");
            return true;
        }
    }
    
    return false;
}

/**
 * Check if we were running partition B when device rebooted
 * 
 * This catches runtime crashes (after init completed)
 */
static bool was_running_partition_b(void) {
    return watchdog_hw->scratch[PARTITION_B_RUNNING_REGISTER] == 
           PARTITION_B_RUNNING_MAGIC;
}

/**
 * Set crash detection flags before booting partition B
 * 
 * Sets both init crash flag and running flag
 */
static void set_crash_detection_flags(void) {
    uint32_t boot_count = watchdog_hw->scratch[BOOT_COUNT_REGISTER];
    
    // Increment boot attempt counter
    watchdog_hw->scratch[BOOT_COUNT_REGISTER] = boot_count + 1;
    
    // Set init crash flag
    watchdog_hw->scratch[CRASH_FLAG_REGISTER] = CRASH_MAGIC;
    
    // Set running flag
    watchdog_hw->scratch[PARTITION_B_RUNNING_REGISTER] = 
        PARTITION_B_RUNNING_MAGIC;
}

/**
 * Clear all crash detection flags
 * 
 * Called when successfully booting to partition A
 */
static void clear_crash_detection_flags(void) {
    watchdog_hw->scratch[CRASH_FLAG_REGISTER] = 0;
    watchdog_hw->scratch[BOOT_COUNT_REGISTER] = 0;
    watchdog_hw->scratch[PARTITION_B_RUNNING_REGISTER] = 0;
}

/* ============================================================================
 * PARTITION BOOT FUNCTIONS
 * ============================================================================ */

/**
 * Jump to a partition
 * 
 * This manually sets up the CPU state and jumps to the partition's
 * reset handler. Does NOT use rom_chain_image - we want full control.
 * 
 * This function DOES NOT RETURN.
 */
static void __attribute__((noreturn)) boot_partition(uint32_t partition_addr, 
                                                      const char *name) {
    printf("\n");
    printf("====================================\n");
    printf("Booting to %s\n", name);
    printf("Address: 0x%08lx\n", partition_addr);
    printf("====================================\n");
    
    sleep_ms(500);  // Give UART time to flush
    
    vector_table_t *vectors = (vector_table_t *)partition_addr;
    
    // Disable all interrupts
    __disable_irq();
    
    // Disable SysTick timer
    *(volatile uint32_t *)0xE000E010 = 0;  // SysTick->CTRL = 0
    
    // Disable all NVIC interrupts
    for (int i = 0; i < 8; i++) {
        ((volatile uint32_t *)0xE000E180)[i] = 0xFFFFFFFF;  // NVIC->ICER
        ((volatile uint32_t *)0xE000E280)[i] = 0xFFFFFFFF;  // NVIC->ICPR
    }
    
    // Set vector table offset register (VTOR)
    *(volatile uint32_t *)0xE000ED08 = partition_addr;
    
    // Set stack pointer
    __set_MSP(vectors->stack_ptr);
    
    // Ensure all writes complete
    __DSB();
    __ISB();
    
    // Jump to reset handler (clear Thumb bit for function pointer)
    void (*reset_handler)(void) = (void (*)(void))(vectors->reset_handler & ~0x1);
    reset_handler();
    
    // Should never reach here
    while (1) {
        tight_loop_contents();
    }
}

/* ============================================================================
 * MAIN BOOTLOADER LOGIC
 * ============================================================================ */

int main(void) {
    // Initialize stdio
    stdio_init_all();
    
    // Wait for USB serial connection (helpful for debugging)
    sleep_ms(2000);
    
    // Print banner
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  RP2350 BOOTLOADER v1.0                            ║\n");
    printf("╚════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Memory Layout:\n");
    printf("  Bootloader: 0x%08x - 0x%08x (%3d KB)\n", 
           BOOTLOADER_BASE, 
           BOOTLOADER_BASE + BOOTLOADER_SIZE - 1,
           BOOTLOADER_SIZE / 1024);
    printf("  Partition A: 0x%08x - 0x%08x (%3d KB)\n", 
           PARTITION_A_BASE, 
           PARTITION_A_BASE + PARTITION_A_SIZE - 1,
           PARTITION_A_SIZE / 1024);
    printf("  Partition B: 0x%08x - 0x%08x (%3d KB)\n", 
           PARTITION_B_BASE, 
           PARTITION_B_BASE + PARTITION_B_SIZE - 1,
           PARTITION_B_SIZE / 1024);
    printf("\n");
    
    /* -----------------------------------------------------------------------
     * STEP 1: Check if B was running when device rebooted (runtime crash)
     * ----------------------------------------------------------------------- */
    
    if (was_running_partition_b()) {
        printf("WARNING: Partition B was running but device rebooted\n");
        printf("This indicates a runtime crash (after initialization)\n");
        
        uint32_t boot_count = watchdog_hw->scratch[BOOT_COUNT_REGISTER];
        printf("Boot attempt count: %lu\n", boot_count);
        
        // Increment crash counter
        watchdog_hw->scratch[BOOT_COUNT_REGISTER] = boot_count + 1;
        
        if (boot_count >= MAX_BOOT_ATTEMPTS) {
            printf("Too many runtime crashes, forcing partition A\n");
            printf("\n");
            
            // Clear flags and boot to A
            clear_crash_detection_flags();
            
            if (is_partition_valid(PARTITION_A_BASE)) {
                boot_partition(PARTITION_A_BASE, "Partition A (Safe Mode)");
            } else {
                printf("ERROR: Partition A is invalid!\n");
                printf("System halted - reflash required\n");
                while (1) { tight_loop_contents(); }
            }
        }
        
        printf("Will retry partition B (attempt %lu/%d)\n", 
               boot_count + 1, MAX_BOOT_ATTEMPTS);
        printf("\n");
    }
    
    /* -----------------------------------------------------------------------
     * STEP 2: Check if B crashed during initialization
     * ----------------------------------------------------------------------- */
    
    if (check_partition_b_init_crash()) {
        printf("Partition B crashed during initialization\n");
        printf("Booting to partition A (safe mode)\n");
        printf("\n");
        
        clear_crash_detection_flags();
        
        if (is_partition_valid(PARTITION_A_BASE)) {
            boot_partition(PARTITION_A_BASE, "Partition A (Safe Mode)");
        } else {
            printf("ERROR: Partition A is also invalid!\n");
            printf("System halted - reflash required\n");
            while (1) { tight_loop_contents(); }
        }
    }
    
    /* -----------------------------------------------------------------------
     * STEP 3: Try to boot partition B
     * ----------------------------------------------------------------------- */
    
    printf("Checking partition B...\n");
    
    if (is_partition_valid(PARTITION_B_BASE)) {
        printf("  Partition B is valid\n");
        printf("  Attempting to boot partition B\n");
        printf("\n");
        
        // Set crash detection flags
        set_crash_detection_flags();
        
        // Boot to partition B
        boot_partition(PARTITION_B_BASE, "Partition B");
        // Does not return
    }
    
    printf("  Partition B is not valid or not present\n");
    printf("\n");
    
    /* -----------------------------------------------------------------------
     * STEP 4: Boot to partition A (fallback)
     * ----------------------------------------------------------------------- */
    
    printf("Falling back to partition A\n");
    printf("\n");
    
    if (is_partition_valid(PARTITION_A_BASE)) {
        clear_crash_detection_flags();
        boot_partition(PARTITION_A_BASE, "Partition A (Baseline)");
        // Does not return
    }
    
    /* -----------------------------------------------------------------------
     * STEP 5: No valid partitions - halt
     * ----------------------------------------------------------------------- */
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  FATAL ERROR: No valid partitions found           ║\n");
    printf("╚════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Both partition A and B are invalid or missing.\n");
    printf("Please flash valid firmware to at least one partition.\n");
    printf("\n");
    printf("To recover:\n");
    printf("  1. Hold BOOTSEL button and connect USB\n");
    printf("  2. Flash partition_a.uf2 to device\n");
    printf("  3. Reset device\n");
    printf("\n");
    printf("System halted.\n");
    
    // Halt forever
    while (1) {
        tight_loop_contents();
    }
    
    return 0;  // Never reached
}