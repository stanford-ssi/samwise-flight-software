#include "adm1176.h"

adm1176_t adm1176_mk(i2c_inst_t *i2c, uint8_t address) {
    return (adm1176_t){.i2c = i2c, .address = address};
}

static uint16_t adm1176_read_reg(adm1176_t *device, uint8_t reg) {
    uint8_t buf[2];
    i2c_write_blocking(device->i2c, device->address, &reg, 1, true);
    i2c_read_blocking(device->i2c, device->address, buf, 2, false);
    return (buf[0] << 8) | buf[1];
}

float adm1176_get_voltage(adm1176_t *device) {
    uint16_t raw = adm1176_read_reg(device, ADM1176_VOLTAGE_REG);
    return raw * (16.5 / 4096.0); // Scale factor for voltage
}

float adm1176_get_current(adm1176_t *device) {
    uint16_t raw = adm1176_read_reg(device, ADM1176_CURRENT_REG);
    return raw * (0.1 / 4096.0); // Scale factor for current
}

void adm1176_config_alert(adm1176_t *device, uint8_t config) {
    uint8_t buf[2] = {ADM1176_ALERT_REG, config};
    i2c_write_blocking(device->i2c, device->address, buf, 2, false);
}
