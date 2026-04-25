/**
 * @author Lundeen Cahilly
 * @date 2025-08-18
 *
 * This file contains functions for interfacing with MR25H40MDF
 * MRAM via QMI direct mode on a RP2350B chip.
 *
 * The MRAM shares the QSPI bus (SCLK, SD0, SD1) with the system
 * flash but uses a dedicated chip select on QMI CS1 (GPIO47).
 * All SPI transactions use QMI direct mode so that only CS1 is
 * asserted — the system flash on CS0 is never touched.
 */

#include "mram.h"

#include <string.h>

#include "hardware/gpio.h"
#include "hardware/regs/qmi.h"
#include "hardware/structs/qmi.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

#include "logger.h"
#include "macros.h"

// MRAM COMMANDS
#define WREN_CMD 0x06 // Write Enable
#define WRDI_CMD 0x04 // Write Disable
#define RDSR_CMD 0x05 // Read Status Register
#define WRSR_CMD 0x01 // Write Status Register
#define READ_CMD 0x03
#define WRITE_CMD 0x02
#define SLEEP_CMD 0xB9
#define WAKE_CMD 0xAB

// MRAM Timing Constants
#define WAKE_TIME_US 400 // Wake-up time in microseconds

// Status Register Bit Masks
#define STATUS_WEL_BIT 0x02  // Write Enable Latch (bit 1)
#define STATUS_BP0_BIT 0x04  // Block Protect 0 (bit 2)
#define STATUS_BP1_BIT 0x08  // Block Protect 1 (bit 3)
#define STATUS_SRWD_BIT 0x80 // Status Register Write Disable (bit 7)

// QMI CS1 pin for the MRAM (GPIO47 on RP2350B)
#define MRAM_CS_PIN 47

// SPI clock divider for QMI direct mode.
// Even N → N/2 system-clock cycles per SCK half-period → SPI_CLK = sys_clk / N.
// At 150 MHz sys_clk, CLKDIV=6 gives 25 MHz, well within the MRAM's 40 MHz max.
#define MRAM_QMI_CLKDIV 6

/**
 * Send a raw SPI transaction to the MRAM via QMI direct mode on CS1.
 *
 * Semantics match flash_do_cmd: txbuf[0..count-1] are clocked out on
 * MOSI while rxbuf[0..count-1] are captured from MISO simultaneously.
 * Pass rxbuf = NULL to ignore received data.
 *
 * Placed in SRAM via __no_inline_not_in_flash_func because QMI direct
 * mode stalls XIP. Callers MUST disable interrupts before calling.
 */
static void __no_inline_not_in_flash_func(mram_qmi_cmd)(
    const uint8_t *txbuf, uint8_t *rxbuf, size_t count)
{
    // Enable direct mode with an explicit clock divider.
    // A direct write is used intentionally to get a clean register state;
    // CLKDIV must be included because the reset default (6) would be
    // zeroed by a bare EN write.
    qmi_hw->direct_csr =
        QMI_DIRECT_CSR_EN_BITS |
        ((uint32_t)MRAM_QMI_CLKDIV << QMI_DIRECT_CSR_CLKDIV_LSB);

    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS)
        tight_loop_contents();

    // Drain any stale bytes left in the RX FIFO from a prior transaction.
    while (!(qmi_hw->direct_csr & QMI_DIRECT_CSR_RXEMPTY_BITS))
        (void)qmi_hw->direct_rx;

    // Manually assert CS1 so it stays held for the entire transaction.
    // AUTO_CS1N would deassert between bytes if the TX FIFO momentarily
    // empties, which can break multi-byte SPI commands.
    hw_set_bits(&qmi_hw->direct_csr, QMI_DIRECT_CSR_ASSERT_CS1N_BITS);

    for (size_t i = 0; i < count; i++)
    {
        uint32_t flags = QMI_DIRECT_TX_OE_BITS;
        if (!rxbuf)
            flags |= QMI_DIRECT_TX_NOPUSH_BITS;

        while (qmi_hw->direct_csr & QMI_DIRECT_CSR_TXFULL_BITS)
            tight_loop_contents();

        qmi_hw->direct_tx = flags | txbuf[i];

        while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS)
            tight_loop_contents();

        if (rxbuf)
        {
            while (qmi_hw->direct_csr & QMI_DIRECT_CSR_RXEMPTY_BITS)
                tight_loop_contents();
            rxbuf[i] = (uint8_t)qmi_hw->direct_rx;
        }
    }

    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS)
        tight_loop_contents();

    // Deassert CS1, then disable direct mode to re-enable XIP.
    hw_clear_bits(&qmi_hw->direct_csr, QMI_DIRECT_CSR_ASSERT_CS1N_BITS);
    hw_clear_bits(&qmi_hw->direct_csr, QMI_DIRECT_CSR_EN_BITS);
}

/**
 * Initialize MRAM and wake from sleep mode
 */
void mram_init(void)
{
    LOG_INFO("[mram] Init start");

    gpio_set_function(MRAM_CS_PIN, GPIO_FUNC_XIP_CS1);

    uint8_t wake_cmd = WAKE_CMD;

    uint32_t interrupts = save_and_disable_interrupts();
    mram_qmi_cmd(&wake_cmd, NULL, 1);
    restore_interrupts(interrupts);

    sleep_us(WAKE_TIME_US);

    LOG_INFO("[mram] Init complete");
}

/**
 * Read status register from MRAM
 * @return Status register value
 */
uint8_t mram_read_status(void)
{
    uint8_t tx_buf[2] = {RDSR_CMD, 0x00};
    uint8_t rx_buf[2] = {0x00, 0x00};

    uint32_t interrupts = save_and_disable_interrupts();
    mram_qmi_cmd(tx_buf, rx_buf, 2);
    restore_interrupts(interrupts);

    return rx_buf[1];
}

/**
 * Enable write operations on MRAM
 */
void mram_write_enable(void)
{
    uint8_t cmd = WREN_CMD;

    uint32_t interrupts = save_and_disable_interrupts();
    mram_qmi_cmd(&cmd, NULL, 1);
    restore_interrupts(interrupts);
}

/**
 * Read data from MRAM at specified address
 * @param address 24-bit address to read from
 * @param data Buffer to store read data
 * @param length Number of bytes to read (max 256)
 */
void mram_read(uint32_t address, uint8_t *data, size_t length)
{
    if (length > 256)
    {
        return;
    }

    static uint8_t cmd_buf[256 + 4];
    static uint8_t rx_buf[256 + 4];

    cmd_buf[0] = READ_CMD;
    cmd_buf[1] = (address >> 16) & 0xFF;
    cmd_buf[2] = (address >> 8) & 0xFF;
    cmd_buf[3] = address & 0xFF;

    for (size_t i = 0; i < length; i++)
    {
        cmd_buf[4 + i] = 0x00;
    }

    uint32_t interrupts = save_and_disable_interrupts();
    mram_qmi_cmd(cmd_buf, rx_buf, 4 + length);
    restore_interrupts(interrupts);

    memcpy(data, &rx_buf[4], length);
}

/**
 * Clear/erase a region of MRAM by writing zeros
 * @param address 24-bit address to clear
 * @param length Number of bytes to clear (max 256)
 */
void mram_clear(uint32_t address, size_t length)
{
    if (length > 256)
    {
        LOG_INFO("[mram] Clear failed: length %zu exceeds maximum", length);
        return;
    }

    mram_write_enable();

    static uint8_t clear_buf[256 + 4];

    clear_buf[0] = WRITE_CMD;
    clear_buf[1] = (address >> 16) & 0xFF;
    clear_buf[2] = (address >> 8) & 0xFF;
    clear_buf[3] = address & 0xFF;

    memset(&clear_buf[4], 0x00, length);

    uint32_t interrupts = save_and_disable_interrupts();
    mram_qmi_cmd(clear_buf, NULL, 4 + length);
    restore_interrupts(interrupts);
}

/**
 * Write data to MRAM at specified address
 * @param address 24-bit address to write to
 * @param data Buffer containing data to write
 * @param length Number of bytes to write (max 256)
 * @return true if write succeeded, false if length exceeds maximum
 */
bool mram_write(uint32_t address, const uint8_t *data, size_t length)
{
    if (length > 256)
    {
        LOG_DEBUG("[mram] Write failed: length %zu exceeds maximum", length);
        return false;
    }

    mram_write_enable();

    static uint8_t cmd_buf[256 + 4];

    cmd_buf[0] = WRITE_CMD;
    cmd_buf[1] = (address >> 16) & 0xFF;
    cmd_buf[2] = (address >> 8) & 0xFF;
    cmd_buf[3] = address & 0xFF;

    memcpy(&cmd_buf[4], data, length);

    uint32_t interrupts = save_and_disable_interrupts();
    mram_qmi_cmd(cmd_buf, NULL, 4 + length);
    restore_interrupts(interrupts);

    return true;
}

/**
 * Disable write operations on MRAM
 */
void mram_write_disable(void)
{
    uint8_t cmd = WRDI_CMD;

    uint32_t interrupts = save_and_disable_interrupts();
    mram_qmi_cmd(&cmd, NULL, 1);
    restore_interrupts(interrupts);
}

/**
 * Put MRAM into low power sleep mode
 */
void mram_sleep(void)
{
    uint8_t cmd = SLEEP_CMD;

    uint32_t interrupts = save_and_disable_interrupts();
    mram_qmi_cmd(&cmd, NULL, 1);
    restore_interrupts(interrupts);
}

/**
 * Wake MRAM from sleep mode
 */
void mram_wake(void)
{
    uint8_t cmd = WAKE_CMD;

    uint32_t interrupts = save_and_disable_interrupts();
    mram_qmi_cmd(&cmd, NULL, 1);
    restore_interrupts(interrupts);

    sleep_us(WAKE_TIME_US);
}
