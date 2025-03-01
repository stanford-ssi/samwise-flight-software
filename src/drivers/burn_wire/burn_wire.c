/**
 * @author Niklas Vainio
 * @date 2025-03-01
 */

#include "burn_wire.h"
#include "slate.h"

#include "hardware/pwm.h"

static uint burn_a_slice_num;
static uint burn_b_slice_num;

#define BURN_WIRE_PWM_WRAP (64)

void burn_wire_init(slate_t *slate)
{
    // Initialize all pins
    gpio_init(SAMWISE_ENAB_BURN_A);
    gpio_set_dir(SAMWISE_ENAB_BURN_A, GPIO_PWM);
    burn_a_slice_num = pwm_gpio_to_slice_num(SAMWISE_ENAB_BURN_A);

    pwm_set_wrap(burn_a_slice_num, BURN_WIRE_PWM_WRAP);
    pwm_set_chan_level(burn_a_slice_num, SAMWISE_BURN_A_PWM_CHANNEL, 0);
    pwm_set_enabled(burn_a_slice_num, true);

    gpio_init(SAMWISE_ENAB_BURN_B);
    gpio_set_dir(SAMWISE_ENAB_BURN_B, GPIO_PWM);
    gpio_put(SAMWISE_ENAB_BURN_B, 0);
    burn_b_slice_num = pwm_gpio_to_slice_num(SAMWISE_ENAB_BURN_B);

    pwm_set_wrap(burn_a_slice_num, BURN_WIRE_PWM_WRAP);
    pwm_set_chan_level(burn_b_slice_num, SAMWISE_BURN_B_PWM_CHANNEL, 0);
    pwm_set_enabled(burn_b_slice_num, true);

    LOG_INFO("burn_wire; Initialized buwn wire pwms [A: %d, B: %d]",
             burn_a_slice_num, burn_b_slice_num);

    gpio_init(SAMWISE_BURN_RELAY);
    gpio_set_dir(SAMWISE_BURN_RELAY, GPIO_OUT);
    gpio_put(SAMWISE_BURN_RELAY, 0);
}

void burn_wire_activate(slate_t *slate, uint32_t burn_ms, uint32_t pwm_level,
                        bool activate_A, bool activate_B)
{
    // Turn on MOSFET
    if (activate_A)
    {
        pwm_set_chan_level(burn_a_slice_num, SAMWISE_BURN_A_PWM_CHANNEL,
                           pwm_level);
    }
    if (activate_B)
    {
        pwm_set_chan_level(burn_b_slice_num, SAMWISE_BURN_B_PWM_CHANNEL,
                           pwm_level);
    }

    sleep_ms(100);

    // Activate relay
    gpio_put(SAMWISE_BURN_RELAY, 1);
    sleep_ms(burn_ms);
    gpio_put(SAMWISE_BURN_RELAY, 0);

    sleep_ms(100);

    // Turn off MOSFET
    if (activate_A)
    {
        pwm_set_chan_level(burn_a_slice_num, SAMWISE_BURN_A_PWM_CHANNEL, 0);
    }
    if (activate_B)
    {
        pwm_set_chan_level(burn_b_slice_num, SAMWISE_BURN_B_PWM_CHANNEL, 0);
    }
}