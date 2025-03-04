#pragma once
#include "hardware/pwm.h"
#include "macros.h"

#ifdef PICO
/*
 * Pins only the pico here...
 */

#define SPI0_CLK (18)
#define SPI0_TX (19)
#define SPI0_RX (16)

#define SAMWISE_RF_SPI (0)
#define SAMWISE_RF_SCK_PIN (18)
#define SAMWISE_RF_MOSI_PIN (19)
#define SAMWISE_RF_MISO_PIN (16)
#define SAMWISE_RF_RST_PIN (21)
#define SAMWISE_RF_CS_PIN (20)
#define SAMWISE_RF_D0_PIN (28)

// Random unused pin, since on the PICO we don't have a watchdog attached.
#define SAMWISE_WATCHDOG_FEED_PIN (6)

#define SAMWISE_ENAB_BURN_A (36)
#define SAMWISE_BURN_A_PWM_CHANNEL (PWM_CHAN_A)
#define SAMWISE_ENAB_BURN_B (35)
#define SAMWISE_BURN_B_PWM_CHANNEL (PWM_CHAN_B)
#define SAMWISE_BURN_RELAY (44)

#else

/*
 * Pins on the PiCubed
 * IMPORTANT: Must be updated to keep in sync with the schematic/avionics!
 */
#define SAMWISE_NEOPIXEL_PIN (0)

#define SAMWSIE_MPPT_SHDN_2_PIN (1)
#define SAMWISE_MPPT_STAT_2_PIN (2)

#define SAMWISE_BAT_HEATER_PIN (3)

#define SAMWISE_MPPT_I2C (I2C_INSTANCE(0))
#define SAMWISE_MPPT_SDA_PIN (4)
#define SAMWISE_MPPT_SCL_PIN (5)

#define SAMWISE_POWER_MONITOR_I2C (I2C_INSTANCE(1))
#define SAMWISE_POWER_MONITOR_SDA_PIN (38)
#define SAMWISE_POWER_MONITOR_SCL_PIN (39)
#define SAMWISE_WATCHDOG_FEED_PIN (6)

#define SAMWISE_MPPT_STAT_1_PIN (7)
#define SAMWISE_MPPT_SHDN_1_PIN (8)

#define SAMWISE_SIDE_DEPLOY_DETECT_B_PIN (9)
#define SAMWISE_SIDE_DEPLOY_DETECT_A_PIN (10)

#define SAMWISE_RF_SPI (1)
#define SAMWISE_RF_ENABLE_PIN (21)
#define SAMWISE_RF_RST_PIN (11)
#define SAMWISE_RF_MISO_PIN (12)
#define SAMWISE_RF_CS_PIN (13)
#define SAMWISE_RF_SCK_PIN (14)
#define SAMWISE_RF_MOSI_PIN (15)
#define SAMWISE_RF_D0_PIN (20)

#define SAMWISE_ENAB_BURN_A (36)
#define SAMWISE_BURN_A_PWM_CHANNEL (PWM_CHAN_A)
#define SAMWISE_ENAB_BURN_B (35)
#define SAMWISE_BURN_B_PWM_CHANNEL (PWM_CHAN_B)
#define SAMWISE_BURN_RELAY (44)

#endif
