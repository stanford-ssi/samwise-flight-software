#include "telemetry_task.h"
#include "hardware/gpio.h"
#include "neopixel.h"
#include "pico/stdlib.h"
#include "pins.h"

// Add power monitor instance
static adm1176_t power_monitor;
// Add MPPT instance
static mppt_t solar_charger_monitor;

#define LOW 0
#define HIGH 1

void telemetry_task_init(slate_t *slate)
{
#ifndef PICO
    LOG_INFO("Scanning the I2C bus...\n");
    bool found_device = false;
    for (uint8_t addr = 0x08; addr < 0x78; ++addr)
    {
        uint8_t rxdata;

        // Scan MPPT I2C
        int ret =
            i2c_read_blocking_until(SAMWISE_MPPT_I2C, addr, &rxdata, 1, false,
                                    make_timeout_time_ms(I2C_TIMEOUT_MS));
        if (ret >= 0)
            found_device = true;

        // Scan power monitor I2C
        ret = i2c_read_blocking_until(SAMWISE_POWER_MONITOR_I2C, addr, &rxdata,
                                      1, false,
                                      make_timeout_time_ms(I2C_TIMEOUT_MS));
        if (ret >= 0)
            found_device = true;
    }

    if (!found_device)
        LOG_ERROR("No I2C devices found.\n");

    // Initialize devices
    power_monitor = adm1176_mk(SAMWISE_POWER_MONITOR_I2C, ADM1176_I2C_ADDR,
                               ADM1176_DEFAULT_SENSE_RESISTOR);
    solar_charger_monitor = mppt_mk(SAMWISE_MPPT_I2C, LT8491_I2C_ADDR);
    mppt_init(&solar_charger_monitor);
#else
    power_monitor = adm1176_mk_mock();
    solar_charger_monitor = mppt_mk_mock();
#endif
}

void telemetry_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(TELEMETRY_TASK_COLOR);

    // Power monitor
    float voltage = adm1176_get_voltage(&power_monitor);
    float current = adm1176_get_current(&power_monitor);
    slate->battery_voltage = (uint16_t)(voltage * 1000);
    slate->battery_current = (uint16_t)(current * 1000);

    // MPPT / Solar charger
    slate->solar_voltage = mppt_get_voltage(&solar_charger_monitor);
    slate->solar_current = mppt_get_current(&solar_charger_monitor);

    // Single hardware pins
    slate->fixed_solar_charge =
        gpio_get(SAMWISE_FIXED_SOLAR_CHARGE) ==
        HIGH; // HIGH = OK; NOTE: was previously LOW, swapped default state to
              // HIGH due to discussion in Issue #86
    slate->fixed_solar_fault =
        gpio_get(SAMWISE_FIXED_SOLAR_FAULT) == LOW; // LOW = Faulty
    slate->panel_A_deployed = gpio_get(SAMWISE_PANEL_A) == HIGH;
    slate->panel_B_deployed = gpio_get(SAMWISE_PANEL_B) == HIGH;
    slate->is_rbf_detected = gpio_get(SAMWISE_RBF_DETECT_PIN) == HIGH;

    // Existing ADCS telemetry
    LOG_INFO("ADCS status: %s", slate->is_adcs_on ? "ON" : "OFF");
    LOG_INFO("ADCS telemetry valid: %s",
             slate->is_adcs_telem_valid ? "VALID" : "INVALID");
    LOG_INFO("ADCS num failed checks: %d", slate->adcs_num_failed_checks);

    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t telemetry_task = {.name = "telemetry",
                               .dispatch_period_ms = 1000,
                               .task_init = &telemetry_task_init,
                               .task_dispatch = &telemetry_task_dispatch,
                               .next_dispatch = 0};