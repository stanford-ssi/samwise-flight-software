#pragma once
#include "hardware/pwm.h"
#include "macros.h"

/*
 * Board-specific GPIO pin definitions are in boards/samwise_pico.h and
 * boards/samwise_picubed.h.  The build system controls which board is
 * selected via the -DPICO=1 copt in .bazelrc.
 *
 * SDK-runtime-type defines (I2C instances, PWM channels, UART peripherals)
 * that cannot live in assembler-safe board headers are defined below.
 */

#ifdef PICO

#include "samwise_pico.h"

// Mocked I2C pins (these resolve to null pointers and should not be
// dereferenced)
#define SAMWISE_MPPT_I2C 0
#define SAMWISE_POWER_MONITOR_I2C 0

#define SAMWISE_BURN_A_PWM_CHANNEL (PWM_CHAN_A)
#define SAMWISE_BURN_B_PWM_CHANNEL (PWM_CHAN_B)

#define SAMWISE_ADCS_UART 0

#else

#include "samwise_picubed.h"

#define SAMWISE_MPPT_I2C (I2C_INSTANCE(0))
#define SAMWISE_POWER_MONITOR_I2C (I2C_INSTANCE(1))

#define SAMWISE_BURN_A_PWM_CHANNEL (PWM_CHAN_A)
#define SAMWISE_BURN_B_PWM_CHANNEL (PWM_CHAN_B)

#define SAMWISE_ADCS_UART (uart1)

#endif
