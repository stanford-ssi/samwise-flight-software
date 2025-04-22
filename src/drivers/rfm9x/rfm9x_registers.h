#ifndef RFM9X_REGISTERS_H_
#define RFM9X_REGISTERS_H_

#include "rfm9x.h"

/*
 * Register access helpers
 */

/*
 * Set mode (RFM9X 6.2 p87)
 */
void rfm9x_set_mode(rfm9x_t *r, rfm9x_mode_t mode);

/*
 * Get mode (RFM9X 6.2 p87)
 */
uint8_t rfm9x_get_mode(rfm9x_t *r);

/*
 * Set low frequency mode (RFM9X 6.2 p87)
 */
void rfm9x_set_low_freq_mode(rfm9x_t *r, uint8_t low_freq);

/*
 * Set low frequency mode (RFM9X 6.2 p87)
 */
uint8_t rfm9x_get_low_freq_mode(rfm9x_t *r);

/*
 * Set long range mode (enable/disable LoRa)
 * (RFM9X.pdf 6.2 p87)
 */
void rfm9x_set_lora(rfm9x_t *r, uint8_t lora);

/*
 * Get long range mode (LoRa status)
 * (RFM9X.pdf 6.2 p87)
 */
uint8_t rfm9x_get_lora(rfm9x_t *r);

/*
 * Triggers oscillator calibration (RFM9X.pdf 6.2 p93)
 *
 * Must be done outside of LoRa mode, since register 0x24 is aliased.
 */
void rfm9x_trigger_osc_calibration(rfm9x_t *r);

/*
 * Set frequency in hz (RFM9X.pdf 6.4 p102)
 */
void rfm9x_set_frequency(rfm9x_t *r, uint32_t f);

/*
 * Get frequency in hz (RFM9X.pdf 6.4 p102)
 */
uint32_t rfm9x_get_frequency(rfm9x_t *r);

/*
 * Set preamble length (RFM9X.pdf 6.4 p107)
 */
void rfm9x_set_preamble_length(rfm9x_t *r, uint16_t l);

/*
 * Get preamble length (RFM9X.pdf 6.4 p107)
 */
uint16_t rfm9x_get_preamble_length(rfm9x_t *r);

/*
 * Set the coding rate. Takes the denominator under 4. Valid values are [5,8].
 *
 * See RFM9X.pdf 6.4 p106
 */
void rfm9x_set_coding_rate(rfm9x_t *r, uint8_t v);

/*
 * Get the coding rate. Returns the denominator under 4.
 *
 * See RFM9X.pdf 6.4 p106
 */
uint8_t rfm9x_get_coding_rate(rfm9x_t *r);

/*
 * Set the spreading factor as base-2 logarithm. Valid values are [6,12].
 *
 * See RFM9X.pdf 6.4 p107
 */
void rfm9x_set_spreading_factor(rfm9x_t *r, uint8_t v);

/*
 * Get the spreading factor as base-2 logarithm. Returns values [6,12].
 *
 * See RFM9X.pdf 6.4 p107
 */
uint8_t rfm9x_get_spreading_factor(rfm9x_t *r);

/*
 * Enable or disable CRC checking
 *
 * See RFM9X.pdf 6.4 p107
 */
void rfm9x_set_crc(rfm9x_t *r, uint8_t crc);

/*
 * Get CRC checking status
 *
 * See RFM9X.pdf 6.4 p107
 */
uint8_t rfm9x_is_crc_enabled(rfm9x_t *r);

/*
 * check if we had a CRC error
 */
uint8_t rfm9x_crc_error(rfm9x_t *r);

/*
 * Set raw output power. (RFM9X.pdf 6.4 p103)
 */
void rfm9x_set_output_power(rfm9x_t *r, uint8_t power);

/*
 * Get raw output power. (RFM9X.pdf 6.4 p103)
 */
uint8_t rfm9x_get_output_power(rfm9x_t *r);

/*
 * Set max power. (RFM9X.pdf 6.4 p103)
 */
void rfm9x_set_max_power(rfm9x_t *r, uint8_t power);

/*
 * Get max power. (RFM9X.pdf 6.4 p103)
 */
uint8_t rfm9x_get_max_power(rfm9x_t *r);

/*
 * Enable or disable PA output pin
 *
 * See RFM9X.pdf 6.4 p103
 */
void rfm9x_set_pa_output_pin(rfm9x_t *r, uint8_t select);

/*
 * Get PA output pin setting
 *
 * See RFM9X.pdf 6.4 p103
 */
uint8_t rfm9x_get_pa_output_pin(rfm9x_t *r);

/*
 * Set LNA gain.
 *
 * See RFM9X.pdf 6.5 p108
 */
void rfm9x_set_lna_gain(rfm9x_t *r, uint8_t gain);

/*
 * Get LNA gain.
 *
 * See RFM9X.pdf 6.5 p108
 */
uint8_t rfm9x_get_lna_gain(rfm9x_t *r);

/*
 * Set AGC auto-on
 *
 * See RFM9X.pdf 6.5 p108
 */
void rfm9x_set_agc_auto(rfm9x_t *r, uint8_t auto_on);

/*
 * Get AGC auto-on
 *
 * See RFM9X.pdf 6.5 p108
 */
uint8_t rfm9x_get_agc_auto(rfm9x_t *r);

/*
 * Set OCP mode
 *
 * See RFM9X.pdf 6.5 p104
 */
void rfm9x_set_ocp_mode(rfm9x_t *r, uint8_t mode);

/*
 * Get OCP mode
 *
 * See RFM9X.pdf 6.5 p104
 */
uint8_t rfm9x_get_ocp_mode(rfm9x_t *r);

/*
 * Set OCP enable
 *
 * See RFM9X.pdf 6.5 p104
 */
void rfm9x_set_ocp(rfm9x_t *r, uint8_t enable);

/*
 * Get OCP enable
 *
 * See RFM9X.pdf 6.5 p104
 */
uint8_t rfm9x_get_ocp(rfm9x_t *r);

/*
 * Set FIFO base address for TX
 *
 * See RFM9X.pdf 6.6 p108
 */
void rfm9x_set_tx_base_address(rfm9x_t *r, uint8_t addr);

/*
 * Get FIFO base address for TX
 *
 * See RFM9X.pdf 6.6 p108
 */
uint8_t rfm9x_get_tx_base_address(rfm9x_t *r);

/*
 * Set FIFO base address for RX
 *
 * See RFM9X.pdf 6.6 p108
 */
void rfm9x_set_rx_base_address(rfm9x_t *r, uint8_t addr);

/*
 * Get FIFO base address for RX
 *
 * See RFM9X.pdf 6.6 p108
 */
uint8_t rfm9x_get_rx_base_address(rfm9x_t *r);

/*
  Set the current FIFO address pointer
*/
void rfm9x_set_fifo_address_pointer(rfm9x_t *r, uint8_t addr);

/*
  Get the current FIFO address pointer
*/
uint8_t rfm9x_get_fifo_address_pointer(rfm9x_t *r);

/*
 * Set the IRQ mask
 *
 * See RFM9X.pdf 6.7 p109
 */
void rfm9x_set_irq_mask(rfm9x_t *r, uint8_t mask);

/*
 * Get the IRQ mask
 *
 * See RFM9X.pdf 6.7 p109
 */
uint8_t rfm9x_get_irq_mask(rfm9x_t *r);

/*
 * Get the IRQ flags
 *
 * See RFM9X.pdf 6.7 p109
 */
uint8_t rfm9x_get_irq_flags(rfm9x_t *r);

/*
 * Clear the IRQ flags
 *
 * See RFM9X.pdf 6.7 p109
 */
void rfm9x_clear_irq_flags(rfm9x_t *r, uint8_t flags);

/*
 * Get the number of received bytes
 *
 * See RFM9X.pdf 6.7 p110
 */
uint8_t rfm9x_get_rx_bytes(rfm9x_t *r);

/*
 * Get the current RSSI value
 *
 * See RFM9X.pdf 6.8 p112
 */
int16_t rfm9x_get_rssi(rfm9x_t *r);

/*
 * Get the packet SNR value
 *
 * See RFM9X.pdf 6.8 p112
 */
int8_t rfm9x_get_packet_snr(rfm9x_t *r);

/*
 * Get the packet RSSI value
 *
 * See RFM9X.pdf 6.8 p112
 */
int16_t rfm9x_get_packet_rssi(rfm9x_t *r);

/*
 * Get the radio version. Returns 0x12 for semtech.
 *
 * See RFM9X.pdf 6.10 p114
 */
uint8_t rfm9x_version(rfm9x_t *r);

#endif /* RFM9X_REGISTERS_H_ */

