#pragma once
#include "macros.h"

<<<<<<< HEAD
=======
#ifdef PICO
/*
 * Pins on the pi pico, used for testing
 */
>>>>>>> 66f7b31 (Added build flag to build for RP2350)

#define SPI0_CLK (18)
#define SPI0_TX (19)
#define SPI0_RX (16)

// TODO: this is a bad, leaky abstraction
#define RFM9X_SPI (spi0)

#define RFM9X_CLK (SPI0_CLK)
#define RFM9X_TX (SPI0_TX)
#define RFM9X_RX (SPI0_RX)
#define RFM9X_RESET (21)
#define RFM9X_CS (20)
#define RFM9X_D0 (28)

#ifdef PICO
/*
 * Pins only the pico here...
 */
#else

/*
 * Pins on the PiCubed
 * IMPORTANT: Must be updated to keep in sync with the schematic/avionics!
 */
#define SAMWISE_NEOPIXEL_PIN (0)

#define SAMWSIE_MPPT_SHDN_2_PIN (1)
#define SAMWISE_MPPT_STAT_2_PIN (2)

#define SAMWISE_BAT_HEATER_PIN (3)

#define SAMWISE_SDA_PIN (4)
#define SAMWISE_SCL_PIN (5)

#define SAMWISE_WATCHDOG_FEED_PIN (6)

#define SAMWISE_MPPT_STAT_1_PIN (7)
#define SAMWISE_MPPT_SHDN_1_PIN (8)

#define SAMWISE_SIDE_DEPLOY_DETECT_B_PIN (9)
#define SAMWISE_SIDE_DEPLOY_DETECT_A_PIN (10)

#define SAMWISE_RF_RST_PIN (11)
#define SAMWSIE_RF_MISO_PIN (12)
#define SAMWISE_RF_CS_PIN (13)
#define SAMWISE_RF_SCK_PIN (14)
#define SAMWISE_RF_MOSI_PIN (15)

#endif
