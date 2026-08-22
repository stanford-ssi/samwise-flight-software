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
static int s_fail_explicit_buy;

int bootrom_mock_reboot_count;
uint32_t bootrom_mock_last_reboot_flags;
uint32_t bootrom_mock_last_reboot_delay_ms;
uint32_t bootrom_mock_last_reboot_p0;
uint32_t bootrom_mock_last_reboot_p1;
bool bootrom_mock_buy_pending;
int bootrom_mock_explicit_buy_count;

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

    // Mirrors ota_mvp/pt.json on the real board: 8 KB unpartitioned area,
    // then two linked 152 KB program partitions, then a 196 KB data partition.
    //   partition 0 "A"    sectors 2..39    -> 0x02000, 152 KB
    //   partition 1 "B"    sectors 40..77   -> 0x28000, 152 KB
    //   partition 2 "data" sectors 78..126  -> 0x4E000, 196 KB
    bootrom_mock_add_partition(2, 39);
    bootrom_mock_add_partition(40, 77);
    bootrom_mock_add_partition(78, 126);
    bootrom_mock_link_ab(0, 1);

    s_current_partition = 0;

    s_fail_pt_info = 0;
    s_fail_boot_info = false;
    s_fail_reboot = 0;
    s_fail_explicit_buy = 0;

    bootrom_mock_reboot_count = 0;
    bootrom_mock_last_reboot_flags = 0;
    bootrom_mock_last_reboot_delay_ms = 0;
    bootrom_mock_last_reboot_p0 = 0;
    bootrom_mock_last_reboot_p1 = 0;
    bootrom_mock_buy_pending = false;
    bootrom_mock_explicit_buy_count = 0;
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

void bootrom_mock_fail_explicit_buy(int error_code)
{
    s_fail_explicit_buy = error_code;
}

void bootrom_mock_simulate_boot(void)
{
    if (bootrom_mock_last_reboot_flags == (uint32_t)BOOT_TYPE_FLASH_UPDATE)
    {
        // Find the partition whose first sector corresponds to p0 and boot
        // into it in TBYB mode (BUY_PENDING set).
        uint32_t target_sector =
            (bootrom_mock_last_reboot_p0 - XIP_BASE) / 4096u;
        for (int i = 0; i < s_num_partitions; i++)
        {
            if (s_partitions[i].first_sector == target_sector)
            {
                s_current_partition = (int8_t)i;
                break;
            }
        }
        bootrom_mock_buy_pending = true;
    }
    else if (bootrom_mock_last_reboot_flags == (uint32_t)BOOT_TYPE_NORMAL)
    {
        // Normal reboot: boot the default A partition (index 0), no TBYB.
        s_current_partition = 0;
        bootrom_mock_buy_pending = false;
    }
}

void bootrom_mock_simulate_rollback(void)
{
    // TBYB timer expired — find the A-side partner of the current partition
    // and roll back to it.
    for (int i = 0; i < s_num_partitions; i++)
    {
        if (s_b_partner[i] == (int)s_current_partition)
        {
            s_current_partition = (int8_t)i;
            bootrom_mock_buy_pending = false;
            return;
        }
    }
    // Current partition has no A-side partner — clear TBYB state anyway.
    bootrom_mock_buy_pending = false;
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
    info->tbyb_and_update_info =
        bootrom_mock_buy_pending ? BOOT_TBYB_AND_UPDATE_FLAG_BUY_PENDING : 0;

    // The parameters passed to rom_reboot survive the reboot and reappear
    // here, which is how partition B hands an OTA request over to A.
    info->reboot_params[0] = bootrom_mock_last_reboot_p0;
    info->reboot_params[1] = bootrom_mock_last_reboot_p1;
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

int rom_explicit_buy(uint8_t *buffer, uint32_t buffer_size)
{
    (void)buffer;
    (void)buffer_size;
    if (s_fail_explicit_buy != 0)
        return s_fail_explicit_buy;
    // In the real bootrom this clears the BUY_PENDING flag from the IMAGE_DEF
    // in flash and, on a version downgrade, erases the first sector of the
    // A-side partner to prevent rollback. SAMWISE never calls explicit_buy
    // (it uses watchdog feeds instead), and the erase is conditional on
    // rollback version metadata we do not model here, so the mock only
    // clears BUY_PENDING.
    bootrom_mock_explicit_buy_count++;
    bootrom_mock_buy_pending = false;
    return BOOTROM_OK;
}
