#include "hardware/i2c.h"
#include "macros.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"

#ifdef BRINGUP

#include "scan_task.h"
#include "slate.h"

/**
 * I2C reserves some addresses for special purposes. We exclude these from the
 * scan. These are any addresses of the form 000 0xxx or 111 1xxx
 */
bool reserved_addr(uint8_t addr)
{
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void scan_task_init(slate_t *slate)
{
    // This example will use I2C0 on the default SDA and SCL pins (GP4, GP5 on a
    // Pico)
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    LOG_INFO("Scan task initialized I2C...");
}

void scan_task_dispatch(slate_t *slate)
{
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
    LOG_INFO("\nI2C Bus Scan\n");
    LOG_INFO("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

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
            ret = i2c_read_blocking(i2c_default, addr, &rxdata, 1, false);

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    printf("Done.\n");
}

sched_task_t scan_task = {.name = "scan",
                          .dispatch_period_ms = 1000,
                          .task_init = &scan_task_init,
                          .task_dispatch = &scan_task_dispatch,

                          /* Set to an actual value on init */
                          .next_dispatch = 0};

#endif