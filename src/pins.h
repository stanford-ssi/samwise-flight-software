#include "macros.h"

#ifdef PICO
/*
 * Pins on the pi pico, used for testing
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