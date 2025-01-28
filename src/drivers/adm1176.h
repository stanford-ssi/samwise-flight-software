/**
 * @author  Summit Kawakami
 * @date    2024-01-27
 *
 * File contains typedef declarations for adm1176 (SAMWISE power monitor)
 * driver and associated functions.
 */

#ifndef ADM1176_H
#define ADM1176_H

#include "hardware/i2c.h"

typedef struct {
    i2c_inst_t *i2c;
    uint8_t address;
    float sense_resistor;
    float voltage_range
} adm1176_t;

// Register addresses
#define ADM1176_VOLTAGE_REG  0x00 // Config/Status register
#define ADM1176_CURRENT_REG  0x02 // Voltage MSBs (Table 13 Page 20)
#define ADM1176_ALERT_REG    0x03 // Current MSBs (Table 14 Page 20)

// Function prototypes
adm1176_t adm1176_mk(i2c_inst_t *i2c, uint8_t address, float sense_resistor, float voltage_range);
float adm1176_get_voltage(adm1176_t *device);
float adm1176_get_current(adm1176_t *device);
void adm1176_config_alert(adm1176_t *device, uint8_t config);

#endif
