/**
 * @author Niklas Vainio
 * @date 2025-03-01
 */

#include "burn_wire.h"

#include "hardware/pwm.h"
#include "pico/stdlib.h"

#include "safe_sleep.h"

void burn_wire_init(slate_t *slate)
{
    // Initialize all pins
    gpio_init(SAMWISE_ENAB_BURN_A);
    gpio_set_dir(SAMWISE_ENAB_BURN_A, GPIO_OUT);
    gpio_put(SAMWISE_ENAB_BURN_A, 0);

    gpio_init(SAMWISE_ENAB_BURN_B);
    gpio_set_dir(SAMWISE_ENAB_BURN_B, GPIO_OUT);
    gpio_put(SAMWISE_ENAB_BURN_B, 0);

    LOG_INFO("burn_wire; Initialized burn wire GPIO [A: %d, B: %d]",
             SAMWISE_ENAB_BURN_A, SAMWISE_ENAB_BURN_B);

    gpio_init(SAMWISE_BURN_RELAY);
    gpio_set_dir(SAMWISE_BURN_RELAY, GPIO_OUT);
    gpio_put(SAMWISE_BURN_RELAY, 0);
}

void burn_wire_activate(slate_t *slate, uint32_t burn_ms, bool activate_A,
                        bool activate_B)
{
    // Activate relay
    gpio_put(SAMWISE_BURN_RELAY, 1);

    // Turn on MOSFET
    if (activate_A)
    {
        gpio_put(SAMWISE_ENAB_BURN_A, 1);
    }
    if (activate_B)
    {
        gpio_put(SAMWISE_ENAB_BURN_B, 1);
    }

    // Actually burn the wire
    safe_sleep_ms(burn_ms);

    // Deactivate relay and MOSFETs
    gpio_put(SAMWISE_BURN_RELAY, 0);
    gpio_put(SAMWISE_ENAB_BURN_A, 0);
    gpio_put(SAMWISE_ENAB_BURN_B, 0);
    safe_sleep_ms(1); // Give some time for the MOSFET to turn off
}