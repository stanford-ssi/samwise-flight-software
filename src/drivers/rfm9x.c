#include "rfm9x.h"
#include "bit-support.h"
#include "hardware/resets.h"
#include "hardware/spi.h"
#include "pico/time.h"
#include "src/macros.h"
#include <string.h>

rfm9x_t rfm9x_mk(spi_inst_t *spi, uint reset_pin, uint cs_pin, uint spi_tx_pin,
                 uint spi_rx_pin, uint spi_clk_pin, uint d0_pin,
                 rfm9x_interrupt_func interrupt_func)
{
    rfm9x_t r = {.reset_pin = reset_pin,
                 .spi_cs_pin = cs_pin,
                 .spi_tx_pin = spi_tx_pin,
                 .spi_rx_pin = spi_rx_pin,
                 .spi_clk_pin = spi_clk_pin,
                 .d0_pin = d0_pin,
                 .interrupt_func = interrupt_func,
                 .spi = spi,
                 /*
                  * Default values
                  */
                 .seq = 0,
                 .high_power = 1,
                 .max_power = 0,
                 .debug = 0};
    return r;
}

/*
 * RFM9X SPI transaction code.
 *
 * See RFM9X.pdf 4.3 p75
 *
 * One thing that isn't entirely clear from the docs is that the device expects
 * a 0x00 written for every data byte read.
 */

static inline void cs_select(rfm9x_t *r)
{
    busy_wait_us(5);
    gpio_put(r->spi_cs_pin, 0);
    busy_wait_us(5);
}

static inline void cs_deselect(rfm9x_t *r)
{
    busy_wait_us(5);
    gpio_put(r->spi_cs_pin, 1);
    busy_wait_us(5);
}

/*
 * Read a buffer from a register address.
 */
static inline void rfm9x_get_buf(rfm9x_t *r, rfm9x_reg_t reg, uint8_t *buf,
                                 uint32_t n)
{
    cs_select(r);

    // First, configure that we will be GETTING from the Radio Module.
    uint8_t value = reg & 0x7F;

    // WRITES to the radio module the value, of length 1 byte, that says that we
    // are GETTING
    spi_write_blocking(r->spi, &value, 1);

    // GETS from the radio module the buffer.
    // The 0 represents the arbitrary byte that should be passed IN as part of
    // the master/slave interaction.
    spi_read_blocking(r->spi, 0, buf, n);

    cs_deselect(r);
}

/*
 * Write a buffer to a register address.
 */
static inline void rfm9x_put_buf(rfm9x_t *r, rfm9x_reg_t reg, uint8_t *buf,
                                 uint32_t n)
{
    cs_select(r);

    // this value will be passed in to tell the radio that we will be writing
    // data
    uint8_t value = reg | 0x80;

    spi_write_blocking(r->spi, &value, 1);

    // Write to the radio that
    spi_write_blocking(r->spi, buf, n);

    cs_deselect(r);
}

/*
 * Write a single byte to an RFM9X register
 */
static inline void rfm9x_put8(rfm9x_t *r, rfm9x_reg_t reg, uint8_t v)
{
    rfm9x_put_buf(r, reg, &v, 1);
}

/*
 * Get a single byte from an RFM9X register
 */
static inline uint8_t rfm9x_get8(rfm9x_t *r, rfm9x_reg_t reg)
{
    uint8_t v = 0;
    rfm9x_get_buf(r, reg, &v, 1);
    return v;
}

void rfm9x_reset(rfm9x_t *r)
{
    // Reset the chip as per RFM9X.pdf 7.2.2 p109

    // set reset pin to output
    gpio_set_dir(r->reset_pin, GPIO_OUT);
    gpio_put(r->reset_pin, 0);

    sleep_us(100);

    // set reset pin to input
    gpio_set_dir(r->reset_pin, GPIO_IN);

    sleep_ms(5);
}

/*
 * Register access helpers
 */

// (RFM9X 6.2 p87)

/*
 * Set mode (RFM9X 6.2 p87)
 */
static inline void rfm9x_set_mode(rfm9x_t *r, rfm9x_mode_t mode)
{
    uint8_t reg = rfm9x_get8(r, _RH_RF95_REG_01_OP_MODE);
    reg = bits_set(reg, 0, 2, mode);
    rfm9x_put8(r, _RH_RF95_REG_01_OP_MODE, reg);
}

/*
 * Get mode (RFM9X 6.2 p87)
 */
static inline uint8_t rfm9x_get_mode(rfm9x_t *r)
{
    uint8_t reg = rfm9x_get8(r, _RH_RF95_REG_01_OP_MODE);
    return bits_get(reg, 0, 2);
}

/*
 * Set low frequency mode (RFM9X 6.2 p87)
 */
static inline void rfm9x_set_low_freq_mode(rfm9x_t *r, uint8_t low_freq)
{
    uint8_t reg = rfm9x_get8(r, _RH_RF95_REG_01_OP_MODE);
    if (low_freq)
        reg = bit_set(reg, 3);
    else
        reg = bit_clr(reg, 3);
    rfm9x_put8(r, _RH_RF95_REG_01_OP_MODE, reg);
}

/*
 * Set low frequency mode (RFM9X 6.2 p87)
 */
static inline uint8_t rfm9x_get_low_freq_mode(rfm9x_t *r)
{
    uint8_t reg = rfm9x_get8(r, _RH_RF95_REG_01_OP_MODE);
    return bit_is_on(reg, 3);
}

/*
 * Set long range mode (enable/disable LoRa)
 * (RFM9X.pdf 6.2 p87)
 */
static inline void rfm9x_set_lora(rfm9x_t *r, uint8_t lora)
{
    uint8_t reg = rfm9x_get8(r, _RH_RF95_REG_01_OP_MODE);
    if (lora)
        reg = bit_set(reg, 7);
    else
        reg = bit_clr(reg, 7);
    rfm9x_put8(r, _RH_RF95_REG_01_OP_MODE, reg);
}

/*
 * Get long range mode (LoRa status)
 * (RFM9X.pdf 6.2 p87)
 */
static inline uint8_t rfm9x_get_lora(rfm9x_t *r)
{
    uint8_t reg = rfm9x_get8(r, _RH_RF95_REG_01_OP_MODE);
    return bit_is_on(reg, 7);
}

/*
 * Triggers oscillator calibration (RFM9X.pdf 6.2 p93)
 *
 * Must be done outside of LoRa mode, since register 0x24 is aliased.
 */
static inline void rfm9x_trigger_osc_calibration(rfm9x_t *r)
{
    uint8_t reg = rfm9x_get8(r, _RH_RF95_REG_24_HOP_PERIOD);
    reg = bit_set(reg, 3);
    rfm9x_put8(r, _RH_RF95_REG_24_HOP_PERIOD, reg);
}

/*
 * Set frequency in hz (RFM9X.pdf 6.4 p102)
 */
static inline void rfm9x_set_frequency(rfm9x_t *r, uint32_t f)
{
    uint32_t frf = (f / _RH_RF95_FSTEP) & 0xFFFFFF;
    uint8_t msb = (frf >> 16) & 0xFF;
    uint8_t mid = (frf >> 8) & 0xFF;
    uint8_t lsb = frf & 0xFF;
    rfm9x_put8(r, _RH_RF95_REG_06_FRF_MSB, msb);
    rfm9x_put8(r, _RH_RF95_REG_07_FRF_MID, mid);
    rfm9x_put8(r, _RH_RF95_REG_08_FRF_LSB, lsb);
}

/*
 * Get frequency in hz (RFM9X.pdf 6.4 p102)
 */
static inline uint32_t rfm9x_get_frequency(rfm9x_t *r)
{
    uint32_t msb = rfm9x_get8(r, _RH_RF95_REG_06_FRF_MSB);
    uint32_t mid = rfm9x_get8(r, _RH_RF95_REG_07_FRF_MID);
    uint32_t lsb = rfm9x_get8(r, _RH_RF95_REG_08_FRF_LSB);
    uint32_t frf = ((msb << 16) | (mid << 8) | lsb) & 0xFFFFFF;
    return (frf * _RH_RF95_FSTEP);
}

/*
 * Set preamble length (RFM9X.pdf 6.4 p107)
 */
void rfm9x_set_preamble_length(rfm9x_t *r, uint16_t l)
{
    rfm9x_put8(r, _RH_RF95_REG_20_PREAMBLE_MSB, l >> 8);
    rfm9x_put8(r, _RH_RF95_REG_21_PREAMBLE_LSB, l & 0xFF);
}

/*
 * Get preamble length (RFM9X.pdf 6.4 p107)
 */
uint16_t rfm9x_get_preamble_length(rfm9x_t *r)
{
    uint16_t msb = rfm9x_get8(r, _RH_RF95_REG_20_PREAMBLE_MSB);
    uint16_t lsb = rfm9x_get8(r, _RH_RF95_REG_21_PREAMBLE_LSB);

    return (msb << 8) | lsb;
}

/*
 * Set the coding rate. Takes the denominator under 4. Valid values are [5,8].
 *
 * See RFM9X.pdf 6.4 p106
 */
void rfm9x_set_coding_rate(rfm9x_t *r, uint8_t v)
{
    uint8_t denominator = 5;
    if (v > 5)
        denominator = v;
    if (v > 8)
        denominator = 8;

    uint8_t cr_id = denominator - 4;
    uint8_t config = rfm9x_get8(r, _RH_RF95_REG_1D_MODEM_CONFIG1);
    config = bits_set(config, 1, 3, cr_id);
    rfm9x_put8(r, _RH_RF95_REG_1D_MODEM_CONFIG1, config);
}

/*
 * Get the coding rate. Returns the denominator under 4.
 *
 * See RFM9X.pdf 6.4 p106
 */
uint8_t rfm9x_get_coding_rate(rfm9x_t *r)
{
    uint8_t config = rfm9x_get8(r, _RH_RF95_REG_1D_MODEM_CONFIG1);
    return bits_get(config, 1, 3) + 4;
}

/*
 * Set the spreading factor as base-2 logarithm. Valid values are [6,12].
 *
 * See RFM9X.pdf 6.4 p107
 */
void rfm9x_set_spreading_factor(rfm9x_t *r, uint8_t v)
{
    uint8_t factor = 6;
    if (v > 6)
        factor = v;
    if (v > 12)
        factor = 12;

    // We skip all the DETECTION_OPTIMIZE and DETECTION_THRESHOLD stuff because
    // it isn't relevant to the RFM9X family.

    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_1E_MODEM_CONFIG2);
    c = bits_set(c, 4, 7, factor);
    rfm9x_put8(r, _RH_RF95_REG_1E_MODEM_CONFIG2, c);
}

/*
 * Get the spreading factor as base-2 logarithm. Returns values [6,12].
 *
 * See RFM9X.pdf 6.4 p107
 */
uint8_t rfm9x_get_spreading_factor(rfm9x_t *r)
{
    return bits_get(rfm9x_get8(r, _RH_RF95_REG_1E_MODEM_CONFIG2), 4, 7);
}

/*
 * Enable or disable CRC checking
 *
 * See RFM9X.pdf 6.4 p107
 */
void rfm9x_set_crc(rfm9x_t *r, uint8_t crc)
{
    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_1E_MODEM_CONFIG2);
    if (crc)
        c = bit_set(c, 2);
    else
        c = bit_clr(c, 2);
    rfm9x_put8(r, _RH_RF95_REG_1E_MODEM_CONFIG2, c);
}

/*
 * Get CRC checking status
 *
 * See RFM9X.pdf 6.4 p107
 */
uint8_t rfm9x_is_crc_enabled(rfm9x_t *r)
{
    return bit_is_on(rfm9x_get8(r, _RH_RF95_REG_1E_MODEM_CONFIG2), 2);
}

/*
 * check if we had a CRC error
 */
uint8_t rfm9x_crc_error(rfm9x_t *r)
{
    return (rfm9x_get8(r, _RH_RF95_REG_12_IRQ_FLAGS) & 0x20) >> 5;
}

/*
 * Set raw output power. (RFM9X.pdf 6.4 p103)
 */
void rfm9x_set_output_power(rfm9x_t *r, uint8_t power)
{
    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_09_PA_CONFIG);
    c = bits_set(c, 0, 3, power);
    rfm9x_put8(r, _RH_RF95_REG_09_PA_CONFIG, c);
}

/*
 * Get raw output power. (RFM9X.pdf 6.4 p103)
 */
uint8_t rfm9x_get_output_power(rfm9x_t *r)
{
    return bits_get(rfm9x_get8(r, _RH_RF95_REG_09_PA_CONFIG), 0, 3);
}

/*
 * Set max power. (RFM9X.pdf 6.4 p103)
 */
void rfm9x_set_max_power(rfm9x_t *r, uint8_t power)
{
    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_09_PA_CONFIG);
    c = bits_set(c, 4, 6, power);
    rfm9x_put8(r, _RH_RF95_REG_09_PA_CONFIG, c);
}

/*
 * Get max power. (RFM9X.pdf 6.4 p103)
 */
uint8_t rfm9x_get_max_power(rfm9x_t *r)
{
    return bits_get(rfm9x_get8(r, _RH_RF95_REG_09_PA_CONFIG), 4, 6);
}

/*
 * Enable or disable PA output pin
 *
 * See RFM9X.pdf 6.4 p103
 */
void rfm9x_set_pa_output_pin(rfm9x_t *r, uint8_t select)
{
    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_09_PA_CONFIG);
    if (select)
        c = bit_set(c, 7);
    else
        c = bit_clr(c, 7);
    rfm9x_put8(r, _RH_RF95_REG_09_PA_CONFIG, c);
}

/*
 * Get PA output pin setting
 *
 * See RFM9X.pdf 6.4 p103
 */
uint8_t rfm9x_get_pa_output_pin(rfm9x_t *r)
{
    return bit_is_on(rfm9x_get8(r, _RH_RF95_REG_09_PA_CONFIG), 7);
}

/*
 * Set PA ramp. (RFM9X.pdf 6.4 p103)
 */
void rfm9x_set_pa_ramp(rfm9x_t *r, uint8_t ramp)
{
    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_0A_PA_RAMP);
    c = bits_set(c, 0, 3, ramp);
    rfm9x_put8(r, _RH_RF95_REG_0A_PA_RAMP, c);
}

/*
 * Get PA ramp. (RFM9X.pdf 6.4 p103)
 */
uint8_t rfm9x_get_pa_ramp(rfm9x_t *r)
{
    return bits_get(rfm9x_get8(r, _RH_RF95_REG_0A_PA_RAMP), 0, 3);
}

/*
 * Set PA DAC (RFM9X.pdf 6.1 p84)
 */
void rfm9x_set_pa_dac(rfm9x_t *r, uint8_t dac)
{
    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_4D_PA_DAC);
    c = bits_set(c, 0, 4, dac);
    rfm9x_put8(r, _RH_RF95_REG_4D_PA_DAC, c);
}

/*
 * Get PA ramp. (RFM9X.pdf 6.4 p103)
 *
 * Note: not entirely accurate, value should be returned in its entirety. It's
 * like this to be symmetrical with rfm9x_set_pa_dac
 */
uint8_t rfm9x_get_pa_dac(rfm9x_t *r)
{
    return bits_get(rfm9x_get8(r, _RH_RF95_REG_4D_PA_DAC), 0, 4);
}

#define BW_BIN_COUNT 9
static uint32_t bw_bins[BW_BIN_COUNT + 1] = {
    7800, 10400, 15600, 20800, 31250, 41700, 62500, 125000, 250000, 0};

void rfm9x_set_bandwidth(rfm9x_t *r, uint32_t bandwidth)
{
    uint8_t bin = 9;
    for (uint8_t i = 0; bw_bins[i] != 0; i++)
    {
        if (bandwidth <= bw_bins[i])
        {
            bin = i;
            break;
        }
    }

    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_1D_MODEM_CONFIG1);
    c = bits_set(c, 4, 7, bin);
    rfm9x_put8(r, _RH_RF95_REG_1D_MODEM_CONFIG1, c);

    if (bandwidth >= 500000)
    {
        /* see Semtech SX1276 errata note 2.1 */
        rfm9x_put8(r, 0x36, 0x02);
        rfm9x_put8(r, 0x3a, 0x64);
    }
    else
    {
        if (bandwidth == 7800)
        {
            rfm9x_put8(r, 0x2F, 0x48);
        }
        else if (bandwidth >= 62500)
        {
            /* see Semtech SX1276 errata note 2.3 */
            rfm9x_put8(r, 0x2F, 0x40);
        }
        else
        {
            rfm9x_put8(r, 0x2F, 0x44);
        }

        rfm9x_put8(r, 0x30, 0);
    }
}

uint32_t rfm9x_get_bandwidth(rfm9x_t *r)
{
    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_1D_MODEM_CONFIG1);
    c = bits_get(c, 4, 7);

    if (c >= BW_BIN_COUNT)
        return 500000;
    else
        return bw_bins[c];
}

/*
 * Set the TX power. If chip is high power, valid values are [5, 23], otherwise
 * [-1, 14]
 */
void rfm9x_set_tx_power(rfm9x_t *r, int8_t power)
{
    if (r->max_power)
    {
        rfm9x_put8(r, _RH_RF95_REG_0B_OCP, 0x3F); /* set 0cp to 240mA */
        rfm9x_set_pa_dac(r, _RH_RF95_PA_DAC_ENABLE);
        rfm9x_set_pa_output_pin(r, 1);
        rfm9x_set_max_power(r, 0b111);
        rfm9x_set_output_power(r, 0x0F);
        return;
    }

    if (r->high_power)
    {
        if (power > 23)
            power = 23;
        if (power < 5)
            power = 5;

        if (power > 20)
        {
            rfm9x_set_pa_dac(r, _RH_RF95_PA_DAC_ENABLE);
            power -= 3;
        }
        else
        {
            rfm9x_set_pa_dac(r, _RH_RF95_PA_DAC_DISABLE);
        }
        rfm9x_set_pa_output_pin(r, 1);
        rfm9x_set_output_power(r, (power - 5) & 0xF);
    }
    else
    {
        if (power > 14)
            power = 14;
        if (power < -1)
            power = -1;

        rfm9x_set_pa_output_pin(r, 1);
        rfm9x_set_max_power(r, 0b111);
        rfm9x_set_output_power(r, (power + 1) & 0x0F);
    }
}

/*
 * Get the TX power. If chip is high power, valid values are [5, 23], otherwise
 * [-1, 14]
 */
int8_t rfm9x_get_tx_power(rfm9x_t *r)
{
    if (r->high_power)
    {
        return rfm9x_get_output_power(r) + 5;
    }
    else
    {
        return (int8_t)rfm9x_get_output_power(r) - 1;
    }
}

void rfm9x_set_lna_boost(rfm9x_t *r, uint8_t boost)
{
    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_0C_LNA);
    c = bits_set(c, 0, 1, boost);
    rfm9x_put8(r, _RH_RF95_REG_0C_LNA, c);
}

uint8_t rfm9x_get_lna_boost(rfm9x_t *r)
{
    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_0C_LNA);
    c = bits_get(c, 0, 1);
    return c;
}

void rfm9x_init(rfm9x_t *r)
{
    // Setup reset line
    gpio_init(r->reset_pin);
    gpio_set_dir(r->reset_pin, GPIO_IN);
    gpio_disable_pulls(r->reset_pin);
    gpio_put(r->reset_pin, 1);

    // Setup cs line
    gpio_init(r->spi_cs_pin);
    gpio_set_dir(r->spi_cs_pin, GPIO_OUT);
    gpio_disable_pulls(r->spi_cs_pin);
    gpio_put(r->spi_cs_pin, 1);

    gpio_set_function(r->spi_clk_pin, GPIO_FUNC_SPI);
    gpio_set_function(r->spi_tx_pin, GPIO_FUNC_SPI);
    gpio_set_function(r->spi_rx_pin, GPIO_FUNC_SPI);
    gpio_set_function(17, GPIO_FUNC_SPI); // ???

    // Setup interrupt line
    gpio_init(r->d0_pin);
    gpio_set_dir(r->d0_pin, GPIO_IN);
    gpio_pull_down(r->d0_pin);

    // Initialize SPI for the RFM9X

    busy_wait_ms(10);

    // RFM9X.pdf 4.3 p75:
    // CPOL = 0, CPHA = 0 (mode 0)
    // MSB first
    spi_init(r->spi, RFM9X_SPI_BAUDRATE);
    spi_set_format(r->spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // Reset the chip
    // rfm9x_reset(r);

    /*
     * Calibrate the oscillator
     */
    rfm9x_set_mode(r, STANDBY_MODE);
    sleep_ms(10);
    rfm9x_trigger_osc_calibration(r);
    sleep_ms(1000); // 1 second

    /*
     * Configure LoRa
     */
    rfm9x_set_mode(r, SLEEP_MODE);
    sleep_ms(10);
    rfm9x_set_lora(r, 1);

    /*
     * Use entire FIFO for RX & TX
     * TODO: This seems bad for simultaneous RX & TX...
     */
    rfm9x_put8(r, _RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0);
    rfm9x_put8(r, _RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0);

    /*
     * Disable frequency hopping
     */
    rfm9x_put8(r, _RH_RF95_REG_24_HOP_PERIOD, 0);

    rfm9x_set_mode(r, STANDBY_MODE);

    /*
     * Configure tranceiver properties
     */
    rfm9x_set_frequency(r, RFM9X_FREQUENCY); /* Always */

    rfm9x_set_preamble_length(r, 8); /* 8 bytes matches Radiohead library */
    ASSERT(rfm9x_get_preamble_length(r) == 8);

    rfm9x_set_bandwidth(r, RFM9X_BANDWIDTH); /* Configure 125000 to match
                                       Radiohead, see SX1276 errata note 2.3 */
    ASSERT(rfm9x_get_bandwidth(r) == RFM9X_BANDWIDTH);

    rfm9x_set_coding_rate(r, 5); /* Configure 4/5 to match Radiohead library */
    ASSERT(rfm9x_get_coding_rate(r) == 5);

    rfm9x_set_spreading_factor(
        r, 7); /* Configure to 7 to match Radiohead library */
    ASSERT(rfm9x_get_spreading_factor(r) == 7);

    rfm9x_set_crc(r, 0); /* Disable CRC checking */
    ASSERT(rfm9x_is_crc_enabled(r) == 0);

    rfm9x_put8(r, _RH_RF95_REG_26_MODEM_CONFIG3, 0x00); /* No sync word */
    rfm9x_set_tx_power(r, 15);                          /* Known good value */
    ASSERT(rfm9x_get_tx_power(r) == 15);

    rfm9x_set_pa_ramp(r, 0);
    ASSERT(rfm9x_get_pa_ramp(r) == 0);

    rfm9x_set_lna_boost(r, 0b11);
    ASSERT(rfm9x_get_lna_boost(r) == 0b11);

    // Setup interrupt
    if (r->interrupt_func != NULL)
    {
        gpio_set_irq_enabled_with_callback(r->d0_pin, GPIO_IRQ_EDGE_RISE, true,
                                           r->interrupt_func);
    }
}

/*
 * Print a raw packet. L includes all bytes, even header.
 */
void rfm9x_print_packet(char *msg, char *packet, uint8_t l)
{
    printf(msg);
    printf("\r\n");
    printf("  Size: %d\r\n", l);
    if (l >= 1)
        printf("  Destination: %x\r\n", packet[0]);
    if (l >= 2)
        printf("  From :%x\r\n", packet[1]);
    if (l >= 3)
        printf("  ID: %x\r\n", packet[2]);
    if (l >= 4)
        printf("  Flags: %x\r\n", packet[3]);
    if (l >= 5)
    {
        printf("  Payload (ASCII): ");
        for (int i = 4; i < l; i++)
            printf("%c", packet[i]);
        printf("\r\n  Payload (Hex): ");
        for (int i = 4; i < l; i++)
            printf("%x ", packet[i]);
    }
    printf("\r\n");
}

uint32_t rfm9x_version(rfm9x_t *r)
{
    return (uint32_t)rfm9x_get8(r, _RH_RF95_REG_42_VERSION);
}

void rfm9x_transmit(rfm9x_t *r)
{
    // we do not have an LNA
    rfm9x_set_mode(r, TX_MODE);
    uint8_t dioValue = rfm9x_get8(r, _RH_RF95_REG_40_DIO_MAPPING1);
    dioValue = bits_set(dioValue, 6, 7, 0b00);
    rfm9x_put8(r, _RH_RF95_REG_40_DIO_MAPPING1, dioValue);
}

void rfm9x_listen(rfm9x_t *r)
{
    rfm9x_set_mode(r, RX_MODE);
    uint8_t dioValue = rfm9x_get8(r, _RH_RF95_REG_40_DIO_MAPPING1);
    dioValue = bits_set(dioValue, 6, 7, 0b00);
    rfm9x_put8(r, _RH_RF95_REG_40_DIO_MAPPING1, dioValue);
}

uint8_t rfm9x_tx_done(rfm9x_t *r)
{
    return (rfm9x_get8(r, _RH_RF95_REG_12_IRQ_FLAGS) & 0x8) >> 3;
}

uint8_t rfm9x_rx_done(rfm9x_t *r)
{
    uint8_t dioValue = rfm9x_get8(r, _RH_RF95_REG_40_DIO_MAPPING1);
    if (dioValue)
    {
        return dioValue;
    }
    else
    {
        return (rfm9x_get8(r, _RH_RF95_REG_12_IRQ_FLAGS) & 0x40) >> 6;
    }
}

int rfm9x_await_rx(rfm9x_t *r)
{
    rfm9x_listen(r);
    while (!rfm9x_rx_done(r))
        ; // spin until RX done
    return 1;
}

uint8_t rfm9x_packet_to_fifo(rfm9x_t *r, uint8_t *buf, uint8_t n)
{
    uint8_t old_mode = rfm9x_get_mode(r);
    rfm9x_set_mode(r, STANDBY_MODE);

    rfm9x_put8(r, _RH_RF95_REG_0D_FIFO_ADDR_PTR, 0x00);

    rfm9x_put_buf(r, _RH_RF95_REG_00_FIFO, buf, n);
    rfm9x_put8(r, _RH_RF95_REG_22_PAYLOAD_LENGTH, n);

    rfm9x_set_mode(r, old_mode);
    return 0;
}

uint8_t rfm9x_packet_from_fifo(rfm9x_t *r, uint8_t *buf)
{
    uint8_t n_read = 0;
    uint8_t old_mode = rfm9x_get_mode(r);
    rfm9x_set_mode(r, STANDBY_MODE);

    // Check for CRC error
    if (rfm9x_is_crc_enabled(r) && rfm9x_crc_error(r))
    {
        // TODO report somehow
    }
    else
    {
        uint8_t fifo_length = rfm9x_get8(r, _RH_RF95_REG_13_RX_NB_BYTES);
        if (fifo_length > 0)
        {
            uint8_t current_addr =
                rfm9x_get8(r, _RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR);
            rfm9x_put8(r, _RH_RF95_REG_0D_FIFO_ADDR_PTR, current_addr);

            // read the packet
            rfm9x_get_buf(r, _RH_RF95_REG_00_FIFO, buf, fifo_length);
        }
        n_read = fifo_length;
    }
    rfm9x_set_mode(r, old_mode);
    return n_read;
}

void rfm9x_clear_interrupts(rfm9x_t *r)
{
    rfm9x_put8(r, _RH_RF95_REG_12_IRQ_FLAGS, 0xFF);
}

uint8_t rfm9x_receive_packet(rfm9x_t *r, uint8_t node, char *buf)
{
    while (1)
    {
        while (!rfm9x_await_rx(r))
            ; // spin until RX done
        uint8_t fifo_length = rfm9x_get8(r, _RH_RF95_REG_13_RX_NB_BYTES);
        if (r->debug)
            printf("FIFO length: %d\r\n", fifo_length);

        // keep listening, no ack, yes debug, no view
        rfm9x_receive(r, buf, node, 1, 0, 1);

        if (fifo_length > 0)
        {
            if (r->debug)
                printf("Received packet: %s\r\n", buf);
            return fifo_length;
        }
        else
        {
            if (r->debug)
                printf("No packet received\r\n");
        }
    }
}

// my pointer stuff with packet is almost definitely wrong!
uint8_t rfm9x_receive(rfm9x_t *r, char *packet, uint8_t node,
                      uint8_t keep_listening, uint8_t with_ack,
                      bool blocking_wait_for_packet)
{
    uint8_t n_bytes = 0;

    // Spin until we have a packet
    // while(!rfm9x_rx_done(r));
    if (blocking_wait_for_packet)
    {
        rfm9x_await_rx(r);
    }

    // enter idle mode so that we don't receive other packets
    rfm9x_set_mode(r, STANDBY_MODE);

    if (rfm9x_is_crc_enabled(r) && rfm9x_crc_error(r))
    {
        if (r->debug)
            printf("[rfm9x] CRC error\r\n");
    }
    else
    {
        if (r->debug)
            printf("[rfm9x] No CRC error\r\n");

        /* FIFO length */
        uint8_t fifo_length = rfm9x_get8(r, _RH_RF95_REG_13_RX_NB_BYTES);

        if (fifo_length > 0)
        {
            // read and clear the FIFO if there is a packet in it
            if (r->debug)
                printf("[rfm9x] FIFO length: %d\r\n", fifo_length);

            uint8_t current_addr =
                rfm9x_get8(r, _RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR);
            rfm9x_put8(r, _RH_RF95_REG_0D_FIFO_ADDR_PTR, current_addr);

            // read the packet
            rfm9x_get_buf(r, _RH_RF95_REG_00_FIFO, (uint8_t *)packet,
                          fifo_length);
        }
        else
        {
            if (r->debug)
                printf("[rfm9x] FIFO is empty\r\n");
            return 0;
        }

        // Clear interrupt
        rfm9x_put8(r, _RH_RF95_REG_12_IRQ_FLAGS, 0xFF);

        if (fifo_length < 5)
        {
            if (r->debug)
                printf("[rfm9x] Missing packet header\r\n");
            return 0;
        }

        if (r->debug)
            rfm9x_print_packet("[rfm9x] Received packet", packet, fifo_length);

        /* Check if addressed to us */
        if (node != _RH_BROADCAST_ADDRESS &&
            packet[0] != _RH_BROADCAST_ADDRESS && packet[0] != node)
        {
            if (r->debug)
                printf("[rfm9x] Packet not for us\r\n");
            return 0;
        }

        /* Check if requesting ACK */
        if ((with_ack || (packet[3] & _SAP_FLAGS_ACK_REQUEST)) &&
            (packet[3] & _RH_FLAGS_ACK) == 0 &&
            (packet[0] != _RH_BROADCAST_ADDRESS))
        {
            // delay before sending Ack to give receiver a chance to get ready
            sleep_ms(100);

            if (r->debug)
                printf("[rfm9x] Sender requested ACK\r\n");

            rfm9x_send(r, "!", 1, keep_listening, packet[1], packet[0],
                       packet[2], (packet[3] | _RH_FLAGS_ACK));

            if (r->debug)
                printf("[rfm9x] Sent ACK to %d\r\n", packet[1]);
        }

        n_bytes = fifo_length;
    }

    if (keep_listening)
    {
        rfm9x_listen(r);
    }

    // Clear interrupt
    // rfm9x_put8(r, _RH_RF95_REG_12_IRQ_FLAGS, 0xFF);

    return n_bytes;
}

uint8_t rfm9x_send(rfm9x_t *r, char *data, uint32_t l, uint8_t keep_listening,
                   uint8_t destination, uint8_t node, uint8_t identifier,
                   uint8_t flags)
{
    // check that length of data is greater than 0 and less than or equal to
    // PAYLOAD_SIZE using an assert
    assert(l > 0 && l <= PAYLOAD_SIZE);

    // set mode to standby
    rfm9x_set_mode(
        r, STANDBY_MODE); // Stop receiving to clear FIFO and keep it clear
    l += 4;               // add 4 bytes for the header

    // Fill the FIFO with a packet to send
    rfm9x_put8(r, _RH_RF95_REG_0D_FIFO_ADDR_PTR, 0x00);

    uint32_t payloadLength = 0;

    // combine header and data to form payload
    if (l == 1 && data[0] == '!')
    {
        payloadLength = 5;
    }
    else
    {
        payloadLength = l;
    }

    printf("l = %d, payloadLength = %d\r\n", l, payloadLength);

    char payload[payloadLength];

    payload[0] = destination;
    payload[1] = node;
    payload[2] = identifier;
    payload[3] = flags;

    // copy the data into the payload
    for (int i = 4; i < payloadLength; i++)
    {
        payload[i] = data[i - 4];
    }

    // write payload
    rfm9x_put_buf(r, _RH_RF95_REG_00_FIFO, (uint8_t *)payload, payloadLength);

    // write payload and header length
    rfm9x_put8(r, _RH_RF95_REG_22_PAYLOAD_LENGTH, payloadLength);

    if (r->debug)
        rfm9x_print_packet("[rfm9x] Transmitting packet", payload,
                           payloadLength);

    // set mode to transmit to send the packet
    rfm9x_transmit(r);

    // wait for tx done interrupt with explicit polling
    while (!rfm9x_tx_done(r))
        ;

    if (keep_listening)
    {
        rfm9x_listen(r);
    }
    else
    {
        rfm9x_set_mode(r, STANDBY_MODE);
    }

    // clear the interrupt
    rfm9x_put8(r, _RH_RF95_REG_12_IRQ_FLAGS, 0xFF);

    return 0;
}

uint8_t rfm9x_send_ack(rfm9x_t *r, char *data, uint32_t l, uint8_t destination,
                       uint8_t node, uint8_t max_retries)
{
    uint8_t acked = 0;
    uint8_t retries = 0;
    uint8_t flags = _SAP_FLAGS_ACK_REQUEST;
    char ack_buffer[256];
    while (!acked && retries < max_retries)
    {
        rfm9x_send(r, data, l, 0, /* Don't keep listening, we do that */
                   destination, node, r->seq, flags);

        // If set we must've sent a retry
        if (flags & _RH_FLAGS_RETRY)
            retries++;

        if (destination == _RH_BROADCAST_ADDRESS)
        {
            if (r->debug)
                printf("[rfm9x] Skipping ACK for broadcast...\r\n");
            // No ack for broadcast
            acked = 1;
        }
        else
        {
            // Wait for ack
            uint16_t l = rfm9x_receive(r, ack_buffer, node, 0, 0, 1);
            if (l > 0)
            { /* Received something */
                if (ack_buffer[3] & _RH_FLAGS_ACK)
                { /* Was an ACK */
                    if (ack_buffer[2] == r->seq)
                    { /* Was an ACK for this message */
                        acked = 1;
                    }
                    else
                    {
                        if (r->debug)
                            printf("[rfm9x] Not for this message\r\n");
                    }
                }
                else
                {
                    if (r->debug)
                        printf("[rfm9x] Not an ACK\r\n");
                }
            }
        }

        // If we didn't receive an ACK, wait to retransmit
        if (!acked)
        {
            sleep_ms(1000); // 1 second
        }

        flags |= _RH_FLAGS_RETRY;
    }

    return acked;
}
