// MPPT is using the LT8491 board

#pragma once

#include "common/pins.h"
#include "hardware/i2c.h"
#include "logger.h"
#include "macros.h"
#include "pins.h"
#include "utils.h"

// LT8491 I2C Address (0x10)
#define LT8491_I2C_ADDR 0x10

// LT8491 Register Addresses
#define LT8491_CTRL_CHRG_EN 0x23

// Configuration Register Data (mirrors Python CFG tuple)
#define NUM_CFG_REGISTERS 9

// Telemetry Register Addresses (mirrors Python TELE tuple)
// Not directly used in the provided main loop logic in the same way as CFG,
// but defined for completeness or future use.
#define LT8491_TELE_TBAT 0x00
#define LT8491_TELE_POUT 0x02
#define LT8491_TELE_PIN 0x04
#define LT8491_TELE_IOUT 0x08
#define LT8491_TELE_IIN 0x0A
#define LT8491_TELE_VBAT 0x0C
#define LT8491_TELE_VIN 0x0E
#define LT8491_TELE_VINR 0x10

// Structure for configuration registers
typedef struct
{
    const char *name;
    uint8_t addr;   // Register address (for the LSB)
    uint16_t value; // 16-bit value to write
} lt8491_cfg_register_t;

typedef struct
{
    i2c_inst_t *i2c;
    uint8_t address;
    bool is_charging;
    bool is_initialized;
    uint16_t VIN_mV;      // TELE_VIN in mV
    uint16_t charging_mV; // TELE_VINR in mV
    uint16_t charging_mA; // TELE_IIN in mA
    uint16_t battery_mV;  // TELE_VBAT in mV
    uint16_t battery_mA;  // TELE_IOUT in mA
} mppt_t;

// Function declarations
mppt_t mppt_mk_mock();
mppt_t mppt_mk(i2c_inst_t *i2c, uint8_t address);
uint16_t mppt_get_battery_voltage(mppt_t *device);
uint16_t mppt_get_battery_current(mppt_t *device);
uint16_t mppt_get_voltage(mppt_t *device);
uint16_t mppt_get_vin_voltage(mppt_t *device);
uint16_t mppt_get_current(mppt_t *device);
void mppt_init(mppt_t *device);
void mppt_read_telemetry(mppt_t *device);
