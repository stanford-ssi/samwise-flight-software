#ifndef ADM1176_H
#define ADM1176_H

#include "hardware/i2c.h"

typedef struct {
    i2c_inst_t *i2c;
    uint8_t address;
} adm1176_t;

// Register addresses
#define ADM1176_VOLTAGE_REG  0x02
#define ADM1176_CURRENT_REG  0x03
#define ADM1176_ALERT_REG    0x00

// Function prototypes
adm1176_t adm1176_mk(i2c_inst_t *i2c, uint8_t address);
float adm1176_get_voltage(adm1176_t *device);
float adm1176_get_current(adm1176_t *device);
void adm1176_config_alert(adm1176_t *device, uint8_t config);

#endif
