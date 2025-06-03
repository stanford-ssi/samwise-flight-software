#pragma once

#include "common/pins.h"
#include "hardware/i2c.h"

// ADM1176 I2C Address (0x94)
#define ADM1176_I2C_ADDR 0x4A

// ADM1176 Data Masks
#define DATA_V_MASK 0xF0
#define DATA_I_MASK 0x0F

// Pre‐allocated I2C buffers
static uint8_t _cmd_buf[1];
static uint8_t _ext_cmd_buf[2] = {0x00, 0x04};
static uint8_t _read_buf[3];
static uint8_t _status_buf[1];

// Default values
#define ADM1176_DEFAULT_SENSE_RESISTOR 0.1f // 0.1 ohm

typedef struct
{
    i2c_inst_t *i2c;
    uint8_t address;
    float sense_resistor; // in ohms
} adm1176_t;

void adm1176_init(adm1176_t *pwm, i2c_inst_t *i2c_bus, uint8_t i2c_addr,
                  float sense_resistor);

/* Mode configuration:
 * 1 -> V-CONT (LSB, set to convert voltage continuously. If readback is
attempted before the first conversion is complete, the ADM1176 asserts an
acknowledge and returns all 0s in the returned data.)

 * 2 -> V-ONCE (Set to convert voltage once. Self-clears. I 2
C asserts a no acknowledge on attempted reads until the ADC
conversion is complete.)

 * 3 -> I_CONT (Set to convert current continuously. If readback is attempted
before the first conversion is complete, the ADM1176 asserts an acknowledge and
returns all 0s in the returned data.)

 * 4 -> I_ONCE (Set to convert current once. Self-clears. I2 C asserts a no
acknowledge on attempted reads until the ADC conversion is complete.)

 * 5 -> V_RANGE (elects different internal attenuation resistor networks for
voltage readback. A 0 in C4 selects a 14:1 voltage divider. A 1 in C4 selects a
7:2 voltage divider. With an ADC full scale of 1.902 V, the voltage at the VCC
pin for an ADC full-scale result is 26.35 V for VRANGE = 0 and 6.65 V for VRANGE
= 1)
 * */
bool adm1176_config(adm1176_t *dev, int *mode, int mode_len);

// Read raw 16-bit voltage register and convert into volts
float adm1176_read_voltage(adm1176_t *dev);

// Read raw 16-bit current register and convert into amps
float adm1176_read_current(adm1176_t *dev);

void adm1176_on(adm1176_t *pwm);

void adm1176_off(adm1176_t *pwm);

bool adm1176_read_status(adm1176_t *pwm, uint8_t *status_out);
