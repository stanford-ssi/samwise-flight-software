#pragma once

#include "common/pins.h"
#include "hardware/i2c.h"

// ADM1176 I2C Address (0x94)
#define ADM1176_I2C_ADDR 0x94

// ADM1176 Register Addresses
#define ADM1176_VOLTAGE_REG 0x00
#define ADM1176_CURRENT_REG 0x01
#define ADM1176_ALERT_REG 0x02

// Default values
#define ADM1176_DEFAULT_VOLTAGE_RANGE 15.0f // 15V range
#define ADM1176_DEFAULT_SENSE_RESISTOR 0.1f // 0.1 ohm

typedef struct
{
    i2c_inst_t *i2c;
    uint8_t address;
    float sense_resistor; // in ohms
    float voltage_range;  // in volts
} adm1176_t;

// Function declarations
adm1176_t adm1176_mk_mock();
adm1176_t adm1176_mk(i2c_inst_t *i2c, uint8_t address, float sense_resistor,
                     float voltage_range);
float adm1176_get_voltage(adm1176_t *device);
float adm1176_get_current(adm1176_t *device);
void adm1176_config_alert(adm1176_t *device, uint8_t config);