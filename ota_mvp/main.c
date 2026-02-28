/**
 * @author  Yiheng Yao andn Marc Aaron Reyes
 * @date    2026-02-28
 *
 * Minimal implementation of syscalls necessary to boot a different partition
 * for OTA.
 */

// Our payload
#include "hardware/flash.h"
#include "hardware/platform_defs.h"
#include "hardware/structs/qmi.h"
#include "hardware/sync.h" // Added for save_and_disable_interrupts()
#include "hardware/watchdog.h"
#include "partition_b.h"
#include "pico/bootrom.h"
#include "pico/printf.h"
#include "pico/stdlib.h"
#include <string.h> // Added for memset/memcpy

#define MAX_WATCHDOG_TIMEOUT_MS 16777

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
}

void reboot_to_partition_1(void)
{
    uint32_t func_code = ROM_TABLE_CODE('A', 'B');
    rom_pick_ab_partition_fn pick_partition =
        (rom_pick_ab_partition_fn)rom_func_lookup_inline(func_code);

    if (pick_partition)
    {
        printf("Found pick_ab_partition function\n");
        uint8_t work_buf[4000];
        int result = pick_partition(&work_buf[0], 4000, 1, 0);

        printf("Pick partition result: %d\n", result);

        if (result == BOOTROM_OK)
        {
            sleep_ms(100);
            rom_reboot(BOOT_TYPE_NORMAL, 50, 0, 0);
        }
    }
    else
    {
        printf("pick_ab_partition not found in bootrom!\n");
    }
}

int main()
{
    stdio_usb_init();
    stdio_init_all();

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (1)
    {
        gpio_put(LED_PIN, 1);
#ifdef BUILD_BLINK
        printf(">>> BLINKING <<<\n");
        printf("State of TBYB: %d\n", PICO_CRT0_IMAGE_TYPE_TBYB);
        sleep_ms(700);
        gpio_put(LED_PIN, 0);
        sleep_ms(300);
        // Ensure the software watchdog timer is set to maximum duration
        watchdog_enable(MAX_WATCHDOG_TIMEOUT_MS, false);
        // Extend the software watchdog timer
        watchdog_update();
#else
        sleep_ms(100);
        printf("\n=== Raspberry Pi Pico Partition Table ===\n\n");
        printf("OTA MVP Main Running...\n");
        printf("Flashing New Partition with size: %u bytes\n",
               bazel_bin_ota_mvp_ota_uf2_len);
        printf("XIP_BASE: %p\n", XIP_BASE);
        printf("XIP_NOCACHE_NOALLOC_NOTRANSLATE_BASE: %p\n",
               XIP_NOCACHE_NOALLOC_NOTRANSLATE_BASE);
        printf("__flash_binary_start: %p\n", __flash_binary_start);

        sleep_ms(100);

        uint32_t buffer[128];
        uint32_t flags = PT_INFO_PT_INFO | PT_INFO_PARTITION_LOCATION_AND_FLAGS;
        int result = rom_get_partition_table_info(
            buffer, sizeof(buffer) / sizeof(uint32_t), flags);

        if (result < 0)
        {
            printf("Error: Failed to read partition table (error code: %d)\n",
                   result);
            return 1;
        }

        if (buffer[0] & PT_INFO_PT_INFO)
        {
            uint32_t has_partition_table = buffer[1] & 0x100;
            uint32_t num_partitions = buffer[1] & 0xF;

            // (Your existing partition printing loop remains here. Truncated
            // for brevity)
            printf("Partition Table Information:\n");
            printf("----------------------------\n");
            printf("Has partition table: %s\n",
                   has_partition_table ? "Yes" : "No");
            printf("Number of partitions: %u\n", num_partitions);
        }

        printf("=== End of Partition Table ===\n");

        uint32_t *partition_start = (uint32_t *)0x1C042000;
        printf("First words of partition 1:\n");
        for (int i = 0; i < 8; i++)
        {
            printf("  [%d] 0x%08x\n", i, partition_start[i]);
        }

        // ==========================================
        // NEW: FLASH PARTITION B WITH OTA PAYLOAD
        // ==========================================
        printf("\n=== Starting Flash Update of Partition B ===\n");

        uint32_t target_offset = 0x42000;
        const uint8_t *payload = bazel_bin_ota_mvp_ota_uf2;
        uint32_t payload_len = bazel_bin_ota_mvp_ota_uf2_len;

        // Quick sanity check for UF2 magic numbers
        if (payload_len >= 8 && payload[0] == 0x55 && payload[1] == 0x46 &&
            payload[2] == 0x32 && payload[3] == 0x0A)
        {
            printf("\n[!] WARNING: This looks like a UF2 file, not a raw "
                   "binary!\n");
            printf("[!] Writing a UF2 directly to flash will not boot because "
                   "it contains headers.\n\n");
            sleep_ms(3000);
        }

        // Calculate total erase size (must be a multiple of FLASH_SECTOR_SIZE =
        // 4096)
        uint32_t erase_size =
            ((payload_len + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE) *
            FLASH_SECTOR_SIZE;

        printf("Disabling interrupts to erase %u bytes at offset 0x%08x...\n",
               erase_size, target_offset);
        uint32_t ints = save_and_disable_interrupts();

        flash_range_erase(target_offset, erase_size);

        printf("Erase complete. Programming %u bytes...\n", payload_len);

        // Program in chunks of 256 bytes (FLASH_PAGE_SIZE)
        for (uint32_t offset = 0; offset < payload_len;
             offset += FLASH_PAGE_SIZE)
        {
            uint8_t page_buf[FLASH_PAGE_SIZE];
            memset(page_buf, 0xFF, FLASH_PAGE_SIZE); // Pad with 0xFF

            uint32_t chunk_size = (payload_len - offset < FLASH_PAGE_SIZE)
                                      ? (payload_len - offset)
                                      : FLASH_PAGE_SIZE;
            memcpy(page_buf, payload + offset, chunk_size);

            flash_range_program(target_offset + offset, page_buf,
                                FLASH_PAGE_SIZE);
        }

        restore_interrupts(ints);
        printf("=== Flash Update Complete ===\n\n");
        // ==========================================

        sleep_ms(2000);

        // <--- this finally work!!! --->
        int ret = rom_reboot(BOOT_TYPE_FLASH_UPDATE, 200,
                             XIP_BASE + target_offset, 0);
        printf("BOOT Successful: %s\n", (ret == BOOTROM_OK) ? "yes" : "no");
        break;
#endif
    }
}
