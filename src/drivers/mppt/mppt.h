// MPPT is using the LT8491 board

#pragma once

#include "common/pins.h"
#include "hardware/i2c.h"
#include "logger.h"
#include "macros.h"
#include "pico/stdlib.h"
#include "pins.h"

/* Python code from test driver

# Tuple (immutable) of configuration register addresses and values
CFG = (
    ("CFG_RSENSE1", 0x28, 0x2710),     # CFG_RSENSE1
    ("CFG_RIMON_OUT", 0x2A, 0x0BC2),     # CFG_RIMON_OUT
    ("CFG_RSENSE2", 0x2C, 0x0CE4),     # CFG_RSENSE2
    ("CFG_RDACO",0x2E, 0x3854),     # CFG_RDACO
    ("CFG_RFBOT1", 0x30, 0x0604),     # CFG_RFBOT1
    ("CFG_RFBOUT2",0x32, 0x0942),     # CFG_RFBOUT2
    ("CFG_RDACI",0x34, 0x0728),     # CFG_RDACI
    ("RFBIN2", 0x36, 0x02DC),     # RFBIN2
    ("RDBIN1", 0x38, 0x03B9),     # RDBIN1
                )

# Tuple of telemetry register addresses
TELE = (
    0x00,               # TELE_TBAT: Battery temperature
    0x02,               # TELE_POUT: Output power
    0x04,               # TELE_PIN: Input power
    0x08,               # TELE_IOUT: Output current
    0x0A,               # TELE_IIN: Input current
        )

    i2c.writeto_then_readfrom(LT8491_ADDR, bytearray([0x12]), result) # Charging
status including Stage #, fault and others. print ("STAT_CHARGER: " +
str(bin(result[0]))) i2c.writeto_then_readfrom(LT8491_ADDR, bytearray([0x19]),
result) # Indicates the source(s) of charging faults. print ("STAT_CHRG_FAULTS:
" + str(bin(result[0]))) i2c.writeto_then_readfrom(LT8491_ADDR,
bytearray([0x08]), result_2) # output current print ("TELE_IOUT: " +
str(result_2[0] + (result_2[1] << 8)) + "mA")
    i2c.writeto_then_readfrom(LT8491_ADDR, bytearray([0x0C]), result_2) # output
voltage print ("TELE_VBAT: " + str((result_2[0] + (result_2[1] << 8))/100) +
"V") i2c.writeto_then_readfrom(LT8491_ADDR, bytearray([0x02]), result_2) #
output power print ("TELE_POUT: " + str((result_2[0] + (result_2[1] << 8))/100)
+ "W" + "\n")


    i2c.writeto_then_readfrom(LT8491_ADDR, bytearray([0x0A]), result_2) # input
current print ("TELE_IIN: " + str(result_2[0] + (result_2[1] << 8)) + "mA")
    i2c.writeto_then_readfrom(LT8491_ADDR, bytearray([0x10]), result_2) # input
voltage print ("TELE_VINR: " + str((result_2[0] + (result_2[1] << 8))/100) +
"V") i2c.writeto_then_readfrom(LT8491_ADDR, bytearray([0x0E]), result_2) # input
voltage (there is a difference, idk what) print ("TELE_VIN: " + str((result_2[0]
+ (result_2[1] << 8))/100) + "V") i2c.writeto_then_readfrom(LT8491_ADDR,
bytearray([0x04]), result_2) # input power print ("TELE_PIN: " +
str((result_2[0] + (result_2[1] << 8))/100) + "W" + "\n")

    i2c.writeto_then_readfrom(LT8491_ADDR, bytearray([0x06]), result_2)
    print ("TELE_EFF: " + str((result_2[0] + (result_2[1] << 8))/100) + "%" +
"\n") # charger efficiency
*/

// LT8491 I2C Address (0x10)
#define LT8491_I2C_ADDR 0x10

// LT8491 Register Addresses
#define LT8491_CTRL_CHRG_EN 0x23

// Configuration Register Data (mirrors Python CFG tuple)
#define NUM_CFG_REGISTERS 9

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
    uint16_t voltage; // in V
    uint16_t current; // in A
} mppt_t;

// Function declarations
mppt_t mppt_mk(i2c_inst_t *i2c, uint8_t address);
uint16_t mppt_get_voltage(mppt_t *device);
uint16_t mppt_get_current(mppt_t *device);
void mppt_init(mppt_t *device);
void mppt_read_telemetry(mppt_t *device);
