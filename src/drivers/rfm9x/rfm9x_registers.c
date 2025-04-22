#include "rfm9x.h"
#include "rfm9x_spi.h"
#include "rfm9x_registers.h"

/*
 * Register access helpers
 */

// (RFM9X 6.2 p87)

/*
 * Set mode (RFM9X 6.2 p87)
 */
void rfm9x_set_mode(rfm9x_t *r, rfm9x_mode_t mode)
{
    uint8_t reg = rfm9x_get8(r, _RH_RF95_REG_01_OP_MODE);
    reg = bits_set(reg, 0, 2, mode);
    rfm9x_put8(r, _RH_RF95_REG_01_OP_MODE, reg);
}

/*
 * Get mode (RFM9X 6.2 p87)
 */
uint8_t rfm9x_get_mode(rfm9x_t *r)
{
    uint8_t reg = rfm9x_get8(r, _RH_RF95_REG_01_OP_MODE);
    return bits_get(reg, 0, 2);
}

/*
 * Set low frequency mode (RFM9X 6.2 p87)
 */
void rfm9x_set_low_freq_mode(rfm9x_t *r, uint8_t low_freq)
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
uint8_t rfm9x_get_low_freq_mode(rfm9x_t *r)
{
    uint8_t reg = rfm9x_get8(r, _RH_RF95_REG_01_OP_MODE);
    return bit_is_on(reg, 3);
}

/*
 * Set long range mode (enable/disable LoRa)
 * (RFM9X.pdf 6.2 p87)
 */
void rfm9x_set_lora(rfm9x_t *r, uint8_t lora)
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
uint8_t rfm9x_get_lora(rfm9x_t *r)
{
    uint8_t reg = rfm9x_get8(r, _RH_RF95_REG_01_OP_MODE);
    return bit_is_on(reg, 7);
}

/*
 * Triggers oscillator calibration (RFM9X.pdf 6.2 p93)
 *
 * Must be done outside of LoRa mode, since register 0x24 is aliased.
 */
void rfm9x_trigger_osc_calibration(rfm9x_t *r)
{
    uint8_t reg = rfm9x_get8(r, _RH_RF95_REG_24_HOP_PERIOD);
    reg = bit_set(reg, 3);
    rfm9x_put8(r, _RH_RF95_REG_24_HOP_PERIOD, reg);
}

/*
 * Set frequency in hz (RFM9X.pdf 6.4 p102)
 */
void rfm9x_set_frequency(rfm9x_t *r, uint32_t f)
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
uint32_t rfm9x_get_frequency(rfm9x_t *r)
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
 * Set LNA gain.
 *
 * See RFM9X.pdf 6.5 p108
 */
void rfm9x_set_lna_gain(rfm9x_t *r, uint8_t gain)
{
    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_0C_LNA);
    c = bits_set(c, 0, 2, gain);
    rfm9x_put8(r, _RH_RF95_REG_0C_LNA, c);
}

/*
 * Get LNA gain.
 *
 * See RFM9X.pdf 6.5 p108
 */
uint8_t rfm9x_get_lna_gain(rfm9x_t *r)
{
    return bits_get(rfm9x_get8(r, _RH_RF95_REG_0C_LNA), 0, 2);
}

/*
 * Set AGC auto-on
 *
 * See RFM9X.pdf 6.5 p108
 */
void rfm9x_set_agc_auto(rfm9x_t *r, uint8_t auto_on)
{
    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_0C_LNA);
    if (auto_on)
        c = bit_set(c, 3);
    else
        c = bit_clr(c, 3);
    rfm9x_put8(r, _RH_RF95_REG_0C_LNA, c);
}

/*
 * Get AGC auto-on
 *
 * See RFM9X.pdf 6.5 p108
 */
uint8_t rfm9x_get_agc_auto(rfm9x_t *r)
{
    return bit_is_on(rfm9x_get8(r, _RH_RF95_REG_0C_LNA), 3);
}

/*
 * Set OCP mode
 *
 * See RFM9X.pdf 6.5 p104
 */
void rfm9x_set_ocp_mode(rfm9x_t *r, uint8_t mode)
{
    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_0B_OCP);
    c = bits_set(c, 0, 4, mode);
    rfm9x_put8(r, _RH_RF95_REG_0B_OCP, c);
}

/*
 * Get OCP mode
 *
 * See RFM9X.pdf 6.5 p104
 */
uint8_t rfm9x_get_ocp_mode(rfm9x_t *r)
{
    return bits_get(rfm9x_get8(r, _RH_RF95_REG_0B_OCP), 0, 4);
}

/*
 * Set OCP enable
 *
 * See RFM9X.pdf 6.5 p104
 */
void rfm9x_set_ocp(rfm9x_t *r, uint8_t enable)
{
    uint8_t c = rfm9x_get8(r, _RH_RF95_REG_0B_OCP);
    if (enable)
        c = bit_set(c, 5);
    else
        c = bit_clr(c, 5);
    rfm9x_put8(r, _RH_RF95_REG_0B_OCP, c);
}

/*
 * Get OCP enable
 *
 * See RFM9X.pdf 6.5 p104
 */
uint8_t rfm9x_get_ocp(rfm9x_t *r)
{
    return bit_is_on(rfm9x_get8(r, _RH_RF95_REG_0B_OCP), 5);
}

/*
 * Set FIFO base address for TX
 *
 * See RFM9X.pdf 6.6 p108
 */
void rfm9x_set_tx_base_address(rfm9x_t *r, uint8_t addr)
{
    rfm9x_put8(r, _RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, addr);
}

/*
 * Get FIFO base address for TX
 *
 * See RFM9X.pdf 6.6 p108
 */
uint8_t rfm9x_get_tx_base_address(rfm9x_t *r)
{
    return rfm9x_get8(r, _RH_RF95_REG_0E_FIFO_TX_BASE_ADDR);
}

/*
 * Set FIFO base address for RX
 *
 * See RFM9X.pdf 6.6 p108
 */
void rfm9x_set_rx_base_address(rfm9x_t *r, uint8_t addr)
{
    rfm9x_put8(r, _RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, addr);
}

/*
 * Get FIFO base address for RX
 *
 * See RFM9X.pdf 6.6 p108
 */
uint8_t rfm9x_get_rx_base_address(rfm9x_t *r)
{
    return rfm9x_get8(r, _RH_RF95_REG_0F_FIFO_RX_BASE_ADDR);
}

/*
  Set the current FIFO address pointer
*/
void rfm9x_set_fifo_address_pointer(rfm9x_t *r, uint8_t addr)
{
    rfm9x_put8(r, _RH_RF95_REG_0D_FIFO_ADDR_PTR, addr);
}

/*
  Get the current FIFO address pointer
*/
uint8_t rfm9x_get_fifo_address_pointer(rfm9x_t *r)
{
    return rfm9x_get8(r, _RH_RF95_REG_0D_FIFO_ADDR_PTR);
}

/*
 * Set the IRQ mask
 *
 * See RFM9X.pdf 6.7 p109
 */
void rfm9x_set_irq_mask(rfm9x_t *r, uint8_t mask)
{
    rfm9x_put8(r, _RH_RF95_REG_11_IRQ_FLAGS_MASK, mask);
}

/*
 * Get the IRQ mask
 *
 * See RFM9X.pdf 6.7 p109
 */
uint8_t rfm9x_get_irq_mask(rfm9x_t *r)
{
    return rfm9x_get8(r, _RH_RF95_REG_11_IRQ_FLAGS_MASK);
}/*
 * Get the IRQ flags
 *
 * See RFM9X.pdf 6.7 p109
 */
uint8_t rfm9x_get_irq_flags(rfm9x_t *r)
{
    return rfm9x_get8(r, _RH_RF95_REG_12_IRQ_FLAGS);
}

/*
 * Clear the IRQ flags
 *
 * See RFM9X.pdf 6.7 p109
 */
void rfm9x_clear_irq_flags(rfm9x_t *r, uint8_t flags)
{
    rfm9x_put8(r, _RH_RF95_REG_12_IRQ_FLAGS, flags);
}

/*
 * Get the number of received bytes
 *
 * See RFM9X.pdf 6.7 p110
 */
uint8_t rfm9x_get_rx_bytes(rfm9x_t *r)
{
    return rfm9x_get8(r, _RH_RF95_REG_13_RX_NB_BYTES);
}

/*
 * Get the current RSSI value
 *
 * See RFM9X.pdf 6.8 p112
 */
int16_t rfm9x_get_rssi(rfm9x_t *r)
{
    int8_t rssi = rfm9x_get8(r, _RH_RF95_REG_1A_PKT_RSSI_VALUE);
    // Corrected formula from documentation
    return -157 + rssi;
}

/*
 * Get the packet SNR value
 *
 * See RFM9X.pdf 6.8 p112
 */
int8_t rfm9x_get_packet_snr(rfm9x_t *r)
{
    return rfm9x_get8(r, _RH_RF95_REG_19_PKT_SNR_VALUE);
}

/*
 * Get the packet RSSI value
 *
 * See RFM9X.pdf 6.8 p112
 */
int16_t rfm9x_get_packet_rssi(rfm9x_t *r)
{
    int8_t rssi = rfm9x_get8(r, _RH_RF95_REG_1A_PKT_RSSI_VALUE);
    return -157 + rssi;
}

/*
 * Get the radio version. Returns 0x12 for semtech.
 *
 * See RFM9X.pdf 6.10 p114
 */
uint8_t rfm9x_version(rfm9x_t *r)
{
    return rfm9x_get8(r, _RH_RF95_REG_42_VERSION);
}
