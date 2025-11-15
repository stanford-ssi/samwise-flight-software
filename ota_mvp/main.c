/**
 * @author  Yao Yiheng
 * @date    2025-10-28
 *
 * Minimal implementation of syscalls necessary to boot a different partition
 * for OTA.
 */

#include "hardware/flash.h"
#include "hardware/platform_defs.h"
#include "hardware/structs/qmi.h"
#include "pico/bootrom.h"
#include "pico/printf.h"
#include "pico/stdlib.h"

#ifndef PICO
// Ensure that PICO_RP2350A is defined to 0 for PICUBED builds.
// This is to enable full 48pin GPIO support on the RP2350A chip.
// boards/samwise_picubed.h should define it to 0.
// The CMakeLists.txt file points to this file for the board definition.
static_assert(PICO_RP2350A == 0,
              "PICO_RP2350A must be defined to 0 for PICUBED builds.");
#endif

extern char __flash_binary_start;

// Read directly from flash via QMI, bypassing XIP entirely
uint32_t read_flash_word(uint32_t *flash_offset)
{
    // Read via direct mode or by temporarily accessing the flash
    // This is more complex but guaranteed to work
    // printf("Trying to read from: %p\n", (uint32_t *)(XIP_NOCACHE_NOALLOC_BASE
    // + flash_offset)); uint32_t *addr = (uint32_t *)(XIP_NOCACHE_NOALLOC_BASE
    // + flash_offset);
    return *flash_offset;
}

void reboot_to_partition(uint32_t *partition_addr)
{
    uint32_t initial_sp = read_flash_word(partition_addr);
    uint32_t reset_handler = read_flash_word(partition_addr + 4);

    printf("Rebooting to partition 1...\n");
    printf("  Flash offset: 0x%08x\n", partition_addr);
    printf("  Stack pointer: 0x%08x\n", initial_sp);
    printf("  Reset handler: 0x%08x\n", reset_handler);

    sleep_ms(1000); // Let printf complete

    // Reboot with 50ms delay
    // p0 = reset handler address (entry point)
    // p1 = initial stack pointer
    // rom_reboot(BOOT_TYPE_FLASH_UPDATE, 50, reset_handler, initial_sp);
}

void reboot_to_partition_1(void)
{
    // Look up the function in bootrom by its code
    // The code is two ASCII characters: 'P' 'A' for "Pick A/B"
    uint32_t func_code = ROM_TABLE_CODE('A', 'B');

    rom_pick_ab_partition_fn pick_partition =
        (rom_pick_ab_partition_fn)rom_func_lookup_inline(func_code);

    if (pick_partition)
    {
        printf("Found pick_ab_partition function\n");

        uint8_t work_buf[4000];
        // Pick partition 1 for next boot
        int result = pick_partition(&work_buf[0], 4000, 1, 0);

        printf("Pick partition result: %d\n", result);

        if (result == BOOTROM_OK)
        {
            // Reboot normally - bootrom will boot partition 1
            sleep_ms(100);
            rom_reboot(BOOT_TYPE_NORMAL, 50, 0, 0);
        }
    }
    else
    {
        printf("pick_ab_partition not found in bootrom!\n");
    }
}

/**
 * Main code entry point.
 *
 * This should never return (unless something really bad happens!)
 */
int main()
{
    stdio_init_all();

// Initialize LED pin
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Infinite loop
    while (1)
    {
        gpio_put(LED_PIN, 1);
        printf("OTA MVP Main Running...\n");
        printf("XIP_BASE: %p\n", XIP_BASE);
        printf("XIP_NOCACHE_NOALLOC_NOTRANSLATE_BASE: %p\n",
               XIP_NOCACHE_NOALLOC_NOTRANSLATE_BASE);
        printf("__flash_binary_start: %p\n", __flash_binary_start);
#ifdef BUILD_BLINK
        printf(">>> BLINKING <<<\n");
        sleep_ms(700);
        gpio_put(LED_PIN, 0);
        sleep_ms(300);
#else
        sleep_ms(1000);

        printf("\n=== Raspberry Pi Pico Partition Table ===\n\n");

        // Buffer to hold partition table info (36 words should be enough for
        // most cases)
        uint32_t buffer[128];

        // Request partition table info and partition locations
        uint32_t flags = PT_INFO_PT_INFO | PT_INFO_PARTITION_LOCATION_AND_FLAGS;

        int result = rom_get_partition_table_info(
            buffer, sizeof(buffer) / sizeof(uint32_t), flags);

        if (result < 0)
        {
            printf("Error: Failed to read partition table (error code: %d)\n",
                   result);
            return 1;
        }

        printf("Words returned: %d\n", result);
        printf("Supported flags: 0x%08x\n\n", buffer[0]);

        // Parse the buffer
        // Word 0: supported flags
        // Word 1: has partition table flag (0 or 1)
        // Word 2: number of partitions
        // Word 3: unpartitioned space size

        if (buffer[0] & PT_INFO_PT_INFO)
        {
            uint32_t has_partition_table = buffer[1] & 0x100;
            uint32_t num_partitions = buffer[1] & 0xF;

            printf("Partition Table Information:\n");
            printf("----------------------------\n");
            printf("Has partition table: %s\n",
                   has_partition_table ? "Yes" : "No");
            printf("Number of partitions: %u\n", num_partitions);

            // Parse partition space permissions and locations + flags words
            // buffer[2:3] -> unpartitioned space
            // subsequent buffer[i:i+1] -> each partition

            // For each partition: permissions_and_location,
            // permissions_and_flags (2 words)
            int idx = 2;
            for (uint32_t i = 0; i < num_partitions + 1; i++)
            {
                // Word 0: permissions_and_location
                uint32_t perms_and_loc = buffer[idx++];

                // Extract fields from permissions_and_location
                // Bits [11:0]: location (4KB blocks from start of flash)
                // Bits [23:12]: size (in 4KB blocks)
                // Bits [31:24]: permission flags
                uint32_t location_first_sector_block =
                    perms_and_loc & 0x00001fffu;
                uint32_t location_last_sector_block =
                    (perms_and_loc & 0x03ffe000u) >> 13;

                // Word 1: permissions_and_flags (additional partition info)
                uint32_t perms_and_flags = buffer[idx++];
                uint32_t perms_secure_read_flags =
                    perms_and_flags & 0x04000000u;
                uint32_t perms_secure_write_flags =
                    perms_and_flags & 0x08000000u;
                uint32_t perms_nonsecure_read_flags =
                    perms_and_flags & 0x10000000u;
                uint32_t perms_nonsecure_write_flags =
                    perms_and_flags & 0x20000000u;

                if (i == 0)
                {
                    printf("Unpartitioned Space:\n", i);
                    printf("  Location: 0x%08x (%u KB from flash start)\n",
                           location_first_sector_block * 4096,
                           location_first_sector_block * 4);
                    printf("  Size: %u KB\n",
                           (location_last_sector_block -
                            location_first_sector_block + 1) *
                               4);

                    // Decode permission bits (exact bit positions depend on
                    // implementation) This is a simplified interpretation
                    printf("  Access Rights:\n");
                    printf("    Secure: %s%s\n",
                           (perms_secure_read_flags) ? "R" : "-",
                           (perms_secure_write_flags) ? "W" : "-");
                    printf("    Non-secure: %s%s\n",
                           (perms_nonsecure_read_flags) ? "R" : "-",
                           (perms_nonsecure_write_flags) ? "W" : "-");
                    printf("\n");
                }
                else
                {
                    printf("Partition %u:\n", i - 1);
                    printf("  Location: 0x%08x (%u KB from flash start)\n",
                           location_first_sector_block * 4096,
                           location_first_sector_block * 4);
                    printf("  Size: %u KB\n",
                           (location_last_sector_block -
                            location_first_sector_block + 1) *
                               4);

                    // Decode permission bits (exact bit positions depend on
                    // implementation) This is a simplified interpretation
                    printf("  Access Rights:\n");
                    printf("    Secure: %s%s\n",
                           (perms_secure_read_flags) ? "R" : "-",
                           (perms_secure_write_flags) ? "W" : "-");
                    printf("    Non-secure: %s%s\n",
                           (perms_nonsecure_read_flags) ? "R" : "-",
                           (perms_nonsecure_write_flags) ? "W" : "-");
                    printf("\n");
                }
            }
        }

        printf("=== End of Partition Table ===\n");

        // Dump raw buffer for debugging
        printf("\nRaw buffer dump (first %d words):\n",
               (result < 20) ? result : 20);
        for (int i = 0; i < result && i < 20; i++)
        {
            printf("  [%2d] 0x%08x\n", i, buffer[i]);
        }

        // Check if partition 1 has an IMAGE_DEF block
        uint32_t *partition_start = (uint32_t *)0x1C042000;

        // Check for IMAGE_DEF signature
        // Should start with specific magic values
        printf("First words of partition 1:\n");
        for (int i = 0; i < 8; i++)
        {
            printf("  [%d] 0x%08x\n", i, partition_start[i]);
        }

        // <--- this finally work!!! --->
        int ret =
            rom_reboot(BOOT_TYPE_FLASH_UPDATE, 200, XIP_BASE + 0x42000, 0);
        printf("BOOT Successful: %s\n", (ret == BOOTROM_OK) ? "yes" : "no");
        break;

        // <--- the following doesn't work at all --->
        // // Read vector table from partition 1 using NON-CACHED XIP
        // uint32_t *vector_table = (uint32_t *)0x1C042000;

        // uint32_t initial_sp = vector_table[0];      // 0x20082000
        // uint32_t reset_handler = vector_table[1];   // 0x1000015d

        // printf("Rebooting to partition 1:\n");
        // printf("  Stack Pointer: 0x%08x\n", initial_sp);
        // printf("  Reset Handler: 0x%08x\n", reset_handler);

        // sleep_ms(100);  // Let printf complete

        // // Reboot to specific PC/SP
        // // Note: This only works in Secure mode!
        // rom_reboot(BOOT_TYPE_PC_SP, 50, reset_handler, initial_sp);

        // Try rebotting using RAM mode
        // rom_reboot(BOOT_TYPE_RAM_IMAGE, 50, 0x42000 /* word-aligned flash
        // start */, 0x10000 /* blocked size */);

        // // Use the A/B partition switching
        // uint8_t work_buf[4000];
        // rom_pick_ab_partition(&work_buf[0], 4000, 1, 0);
        // rom_reboot(BOOT_TYPE_NORMAL, 50, 0, 0);

        // reboot_to_partition_1();
#endif
    }
}
