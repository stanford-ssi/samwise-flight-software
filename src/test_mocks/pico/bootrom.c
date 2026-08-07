/**
 * Host-test mock for pico/bootrom.h -- see bootrom.h for rationale.
 */

#include "pico/bootrom.h"

#include <string.h>

typedef struct
{
    uint32_t first_sector;
    uint32_t last_sector; // inclusive
} mock_partition_t;

static mock_partition_t s_partitions[BOOTROM_MOCK_MAX_PARTITIONS];
static int s_b_partner[BOOTROM_MOCK_MAX_PARTITIONS];
static int s_num_partitions;
static int8_t s_current_partition;

static int s_fail_pt_info;
static bool s_fail_boot_info;
static int s_fail_reboot;

int bootrom_mock_reboot_count;
uint32_t bootrom_mock_last_reboot_flags;
uint32_t bootrom_mock_last_reboot_delay_ms;
uint32_t bootrom_mock_last_reboot_p0;
uint32_t bootrom_mock_last_reboot_p1;

void bootrom_mock_clear_partitions(void)
{
    s_num_partitions = 0;
    for (int i = 0; i < BOOTROM_MOCK_MAX_PARTITIONS; i++)
    {
        s_b_partner[i] = BOOTROM_ERROR_NOT_FOUND;
    }
}

void bootrom_mock_reset(void)
{
    bootrom_mock_clear_partitions();

    // Mirrors ota_mvp/pt.json on the real board: an 8 KB partition table,
    // then two linked 256 KB program partitions, then data.
    //   partition 0 "A"    sectors 2..65     -> 0x02000, 256 KB
    //   partition 1 "B"    sectors 66..129   -> 0x42000, 256 KB
    //   partition 2 "data" sectors 130..4095
    bootrom_mock_add_partition(2, 65);
    bootrom_mock_add_partition(66, 129);
    bootrom_mock_add_partition(130, 4095);
    bootrom_mock_link_ab(0, 1);

    s_current_partition = 0;

    s_fail_pt_info = 0;
    s_fail_boot_info = false;
    s_fail_reboot = 0;

    bootrom_mock_reboot_count = 0;
    bootrom_mock_last_reboot_flags = 0;
    bootrom_mock_last_reboot_delay_ms = 0;
    bootrom_mock_last_reboot_p0 = 0;
    bootrom_mock_last_reboot_p1 = 0;
}

int bootrom_mock_add_partition(uint32_t first_sector, uint32_t last_sector)
{
    if (s_num_partitions >= BOOTROM_MOCK_MAX_PARTITIONS)
    {
        return -1;
    }
    int idx = s_num_partitions++;
    s_partitions[idx].first_sector = first_sector;
    s_partitions[idx].last_sector = last_sector;
    return idx;
}

void bootrom_mock_link_ab(int a_index, int b_index)
{
    if (a_index >= 0 && a_index < BOOTROM_MOCK_MAX_PARTITIONS)
    {
        s_b_partner[a_index] = b_index;
    }
}

void bootrom_mock_set_current_partition(int8_t partition)
{
    s_current_partition = partition;
}

void bootrom_mock_fail_partition_table_info(int error_code)
{
    s_fail_pt_info = error_code;
}

void bootrom_mock_fail_boot_info(bool fail)
{
    s_fail_boot_info = fail;
}

void bootrom_mock_fail_reboot(int error_code)
{
    s_fail_reboot = error_code;
}

int rom_get_partition_table_info(uint32_t *out_buffer,
                                 uint32_t out_buffer_word_size,
                                 uint32_t partition_and_flags)
{
    if (s_fail_pt_info != 0)
    {
        return s_fail_pt_info;
    }
    if (out_buffer == NULL)
    {
        return BOOTROM_ERROR_INVALID_ARG;
    }

    // 4 header words + 2 per partition.
    uint32_t needed = 4 + (uint32_t)s_num_partitions * 2;
    if (out_buffer_word_size < needed)
    {
        return BOOTROM_ERROR_BUFFER_TOO_SMALL;
    }

    memset(out_buffer, 0, needed * sizeof(uint32_t));

    out_buffer[0] = partition_and_flags;
    // bit 0x100 = a partition table is present; low nibble = partition count
    out_buffer[1] = 0x100u | ((uint32_t)s_num_partitions & 0xFu);
    out_buffer[2] = 0; // unpartitioned space permissions_and_location
    out_buffer[3] = 0; // unpartitioned space permissions_and_flags

    for (int i = 0; i < s_num_partitions; i++)
    {
        // permissions_and_location: first_sector [12:0], last_sector [25:13]
        out_buffer[4 + i * 2] = (s_partitions[i].first_sector & 0x1FFFu) |
                                ((s_partitions[i].last_sector & 0x1FFFu) << 13);
        out_buffer[5 + i * 2] = 0; // permissions_and_flags
    }

    return (int)needed;
}

bool rom_get_boot_info(boot_info_t *info)
{
    if (s_fail_boot_info || info == NULL)
    {
        return false;
    }

    memset(info, 0, sizeof(*info));
    info->diagnostic_partition_index = 0;
    info->boot_type = BOOT_TYPE_NORMAL;
    info->partition = s_current_partition;
    info->tbyb_and_update_info = 0;
    return true;
}

int rom_get_b_partition(unsigned int pi_a)
{
    if (pi_a >= (unsigned int)BOOTROM_MOCK_MAX_PARTITIONS)
    {
        return BOOTROM_ERROR_INVALID_ARG;
    }
    return s_b_partner[pi_a];
}

int rom_reboot(uint32_t flags, uint32_t delay_ms, uint32_t p0, uint32_t p1)
{
    bootrom_mock_reboot_count++;
    bootrom_mock_last_reboot_flags = flags;
    bootrom_mock_last_reboot_delay_ms = delay_ms;
    bootrom_mock_last_reboot_p0 = p0;
    bootrom_mock_last_reboot_p1 = p1;
    return s_fail_reboot;
}
