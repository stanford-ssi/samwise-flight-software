#include "diagnostics_task.h"
#include "macros.h"
#ifdef BRINGUP

/**
 * I2C reserves some addresses for special purposes. We exclude these from the
 * scan. These are any addresses of the form 000 0xxx or 111 1xxx
 */
static bool reserved_addr(uint8_t addr)
{
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void diagnostics_task_init(slate_t *slate)
{
    slate->loop_counter = 0;
    LOG_INFO("Scan task initialized I2C...");
}

void diagnostics_task_dispatch(slate_t *slate)
{

    slate->loop_counter++;
    LOG_INFO("Loop #%d", slate->loop_counter);

    /*
     * Unique ID
     */
    char id_str[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    pico_get_unique_board_id_string(id_str, sizeof(id_str));
    LOG_INFO("Chip ID: %s", id_str);

    /*
     * I2C
     */

    // Sweep through all 7-bit I2C addresses, to see if any slaves are present
    // on the I2C bus. Print out a table that looks like this:
    //
    // I2C Bus Scan
    //    0 1 2 3 4 5 6 7 8 9 A B C D E F
    // 00 . . . . . . . . . . . . . . . .
    // 10 . . @ . . . . . . . . . . . . .
    // 20 . . . . . . . . . . . . . . . .
    // 30 . . . . @ . . . . . . . . . . .
    // 40 . . . . . . . . . . . . . . . .
    // 50 . . . . . . . . . . . . . . . .
    // 60 . . . . . . . . . . . . . . . .
    // 70 . . . . . . . . . . . . . . . .
    // E.g. if addresses 0x12 and 0x34 were acknowledged.
    LOG_INFO("I2C Bus Scan %d", I2C_NUM(SAMWISE_MPPT_I2C));
    LOG_INFO("\n0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (int addr = 0; addr < (1 << 7); ++addr)
    {
        if (addr % 16 == 0)
        {
            printf("%02x ", addr);
        }

        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.

        // Skip over any reserved addresses.
        int ret;
        uint8_t rxdata;
        if (reserved_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
            // ret = i2c_read_blocking(SAMWISE_MPPT_I2C, addr, &rxdata, 1,
            // false);
            ret = i2c_read_timeout_us(SAMWISE_MPPT_I2C, addr, &rxdata, 1, false,
                                      1000);

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    LOG_INFO("I2C Bus Scan %d", I2C_NUM(SAMWISE_POWER_MONITOR_I2C));
    LOG_INFO("\n0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (int addr = 0; addr < (1 << 7); ++addr)
    {
        if (addr % 16 == 0)
        {
            printf("%02x ", addr);
        }

        int ret;
        uint8_t rxdata;
        if (reserved_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
            // ret = i2c_read_blocking(SAMWISE_POWER_MONITOR_I2C, addr, &rxdata,
            // 1, false);
            ret = i2c_read_timeout_us(SAMWISE_POWER_MONITOR_I2C, addr, &rxdata,
                                      1, false, 1000);

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    LOG_INFO("Radio version: v%d", rfm9x_version(&slate->radio));
    LOG_INFO("Done.\n");
}

sched_task_t diagnostics_task = {.name = "diagnostics",
                                 .dispatch_period_ms = 1000,
                                 .task_init = &diagnostics_task_init,
                                 .task_dispatch = &diagnostics_task_dispatch,

                                 /* Set to an actual value on init */
                                 .next_dispatch = 0};

#endif
