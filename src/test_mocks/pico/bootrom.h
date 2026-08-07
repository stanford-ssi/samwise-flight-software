/**
 * Host-test mock for pico/bootrom.h
 *
 * ota_task.c does not merely call the bootrom, it makes decisions from what
 * the bootrom returns: which partition it is running from, where the partner
 * partition lives, and how big it is. An empty stub would make every OTA test
 * bail out at the size check and prove nothing, so this mock maintains a real
 * (if simplified) partition table that tests can arrange.
 *
 * Constants and the boot_info_t layout are copied from the pico-sdk so that
 * host tests exercise the same bit-packing as flight:
 *   src/rp2_common/boot_bootrom_headers/include/boot/bootrom_constants.h
 *   src/rp2_common/pico_bootrom/include/pico/bootrom.h
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

// --- Return codes (bootrom_constants.h) ---
#define BOOTROM_OK 0
#define BOOTROM_ERROR_INVALID_ARG (-5)
#define BOOTROM_ERROR_BUFFER_TOO_SMALL (-13)
#define BOOTROM_ERROR_NOT_FOUND (-17)

// --- Reboot types (bootrom_constants.h) ---
#define BOOT_TYPE_NORMAL 0
#define BOOT_TYPE_BOOTSEL 2
#define BOOT_TYPE_RAM_IMAGE 3
#define BOOT_TYPE_FLASH_UPDATE 4

// --- Partition table query flags (bootrom_constants.h) ---
#define PT_INFO_PT_INFO 0x0001
#define PT_INFO_PARTITION_LOCATION_AND_FLAGS 0x0010
#define PT_INFO_PARTITION_ID 0x0020
#define PT_INFO_SINGLE_PARTITION 0x8000

/*
 * Normally from hardware/regs/addressmap.h, which has no mock. Guarded so a
 * future addressmap mock can define it without colliding.
 */
#ifndef XIP_BASE
#define XIP_BASE 0x10000000u
#endif

/*
 * Mirrors the pico-sdk definition exactly. The packed struct and boot_word
 * alias the same 4 bytes, so `partition` occupies bits [23:16] -- which is
 * what ota_task.c extracts via (boot_word >> 16) & 0xFF.
 */
typedef struct
{
    union
    {
        struct __attribute__((packed))
        {
            int8_t diagnostic_partition_index;
            uint8_t boot_type;
            int8_t partition;
            uint8_t tbyb_and_update_info;
        };
        uint32_t boot_word;
    };
    uint32_t boot_diagnostic;
    uint32_t reboot_params[2];
} boot_info_t;

// --- Bootrom API (signatures match the pico-sdk) ---

/*
 * Fills out_buffer in the bootrom's layout and returns the number of words
 * written, or a negative BOOTROM_ERROR_* code:
 *   word 0      flags echoed back
 *   word 1      PT_INFO: bit 0x100 = has partition table, bits [3:0] = count
 *   words 2-3   unpartitioned space
 *   words 4+    two words per partition
 * Partition word 0 packs first_sector in bits [12:0] and last_sector
 * (inclusive) in bits [25:13], both in 4 KB sectors.
 */
int rom_get_partition_table_info(uint32_t *out_buffer,
                                 uint32_t out_buffer_word_size,
                                 uint32_t partition_and_flags);

bool rom_get_boot_info(boot_info_t *info);

// Returns the B partner of A partition pi_a, or BOOTROM_ERROR_NOT_FOUND.
int rom_get_b_partition(unsigned int pi_a);

/*
 * Records the call and returns. Note the real bootrom does NOT return on
 * success, so a test asserting "we rebooted" should check
 * bootrom_mock_reboot_count rather than this return value.
 */
int rom_reboot(uint32_t flags, uint32_t delay_ms, uint32_t p0, uint32_t p1);

// --- Test control ---

#define BOOTROM_MOCK_MAX_PARTITIONS 8

/*
 * Resets to a partition table matching the real board (ota_mvp/pt.json):
 *   partition 0 "A"  sectors 2..65    (0x02000, 256 KB)
 *   partition 1 "B"  sectors 66..129  (0x42000, 256 KB)
 *   partition 2 data sectors 130..4095
 * with 0 linked to 1 as its B partner, and currently booted from partition 0.
 * Call this at the start of every test.
 */
void bootrom_mock_reset(void);

// Appends a partition; returns its index, or -1 if the table is full.
int bootrom_mock_add_partition(uint32_t first_sector, uint32_t last_sector);

// Clears all partitions and A/B links (for building a table from scratch).
void bootrom_mock_clear_partitions(void);

// Declares b_index to be the B partner of a_index.
void bootrom_mock_link_ab(int a_index, int b_index);

// Sets which partition rom_get_boot_info() reports as currently running.
void bootrom_mock_set_current_partition(int8_t partition);

// Error injection. Pass 0 / false to clear.
void bootrom_mock_fail_partition_table_info(int error_code);
void bootrom_mock_fail_boot_info(bool fail);
void bootrom_mock_fail_reboot(int error_code);

// --- Observation ---

extern int bootrom_mock_reboot_count;
extern uint32_t bootrom_mock_last_reboot_flags;
extern uint32_t bootrom_mock_last_reboot_delay_ms;
extern uint32_t bootrom_mock_last_reboot_p0;
extern uint32_t bootrom_mock_last_reboot_p1;
