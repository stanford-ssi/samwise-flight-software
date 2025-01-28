#include "adm1176.h"

// Read 2-byte register
static uint16_t adm1176_read_reg(adm1176_t *device, uint8_t reg) {
    uint8_t buf[2];
    i2c_write_blocking(device->i2c, device->address, &reg, 1, true);
    i2c_read_blocking(device->i2c, device->address, buf, 2, false);
    return (buf[0] << 4) | (buf[1] >> 4); // 12 bit ADC form
}

// Read 3 byte (voltage + current)
static void adm1176_read_voltage_current(adm1176_t *device, uint16_t *voltage_raw, uint16_t *current_raw) {
    uint8_t buf[3];
    i2c_read_blocking(device->i2c, device->address, buf, 3, false);
    *current_raw = (buf[0] << 4 | buf[1] >> 4); // Current MSBs + partial LSBs
    *voltage_raw = ((buf[1] & 0x0F) << 8) | buf[2]; // Voltage MSBs + LSBs
}

adm1176_t adm1176_mk(i2c_inst_t *i2c, uint8_t address, float sense_resistor, float voltage_range) {
    return (adm1176_t){.i2c = i2c, .address = address, .sense_resistor = sense_resistor, .voltage_range = voltage_range};
}

// Scale factor for raw to voltage is in ADM1176 page 20
float adm1176_get_voltage(adm1176_t *device) {
    uint16_t raw = adm1176_read_reg(device, ADM1176_VOLTAGE_REG);
    return raw * (device->voltage_range / 4096.0);
}

float adm1176_get_current(adm1176_t *device) {
    uint16_t raw_current = adm1176_read_reg(device, ADM1176_CURRENT_REG);
    return raw_current * (0.10584 / 4096.0); // numerator is I_fullscale, page 20 ADM1176
}

void adm1176_config_alert(adm1176_t *device, uint8_t config) {
    uint8_t buf[2] = {ADM1176_ALERT_REG, config};
    i2c_write_blocking(device->i2c, device->address, buf, 2, false);
}
