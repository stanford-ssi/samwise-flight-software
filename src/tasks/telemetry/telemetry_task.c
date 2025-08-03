#include "telemetry_task.h"
#include "neopixel.h"

// Add power monitor instance
static adm1176_t power_monitor;
// Add MPPT instance
static mppt_t solar_charger_monitor;

void telemetry_task_init(slate_t *slate)
{
#ifndef PICO
    LOG_INFO("Scanning the I2C bus...\n");
    bool found_device = false;
    for (uint8_t addr = 0x08; addr < 0x78; ++addr)
    { // Valid 7-bit I2C addresses
        uint8_t rxdata;
        // try_lock equivalent: i2c_read_blocking checks for ACK
        // A read of 1 byte is a common way to check for a device
        // For some devices, a write is better. LT8491 should respond to a read
        // attempt to a valid address. However, a more robust check might
        // involve trying to read a known register. For a simple scan, we just
        // see if we get an ACK. The SDK functions return PICO_ERROR_GENERIC if
        // no device responds.
        LOG_INFO("Scanning MPPT_I2C address 0x%02X\n", addr);
        int ret =
            i2c_read_blocking_until(SAMWISE_MPPT_I2C, addr, &rxdata, 1, false,
                                    make_timeout_time_ms(I2C_TIMEOUT_MS));
        if (ret >= 0)
        { // If ret is not an error code (i.e., ACK received)
            LOG_INFO("MPPT Device found at 0x%02X\n", addr);
            found_device = true;
        }
        LOG_INFO("Scanning POWER_I2C address 0x%02X\n", addr);
        ret = i2c_read_blocking_until(SAMWISE_POWER_MONITOR_I2C, addr, &rxdata,
                                      1, false,
                                      make_timeout_time_ms(I2C_TIMEOUT_MS));
        if (ret >= 0)
        { // If ret is not an error code (i.e., ACK received)
            LOG_INFO("Power Monitor Device found at 0x%02X\n", addr);
            found_device = true;
        }
    }
    if (!found_device)
    {
        LOG_ERROR("No I2C devices found.\n");
    }

    // Initialize power monitor
    power_monitor = adm1176_mk(SAMWISE_POWER_MONITOR_I2C, ADM1176_I2C_ADDR,
                               ADM1176_DEFAULT_SENSE_RESISTOR);

    // Initialize MPPT
    solar_charger_monitor = mppt_mk_mock();
    // solar_charger_monitor = mppt_mk(SAMWISE_MPPT_I2C, LT8491_I2C_ADDR);
    mppt_init(&solar_charger_monitor);
#else
    // Initialize mocked PICO power monitor
    power_monitor = adm1176_mk_mock();

    // Initialize mocked PICO MPPT
    solar_charger_monitor = mppt_mk_mock();
#endif
}

void telemetry_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(TELEMETRY_TASK_COLOR);
    // Read power monitor data from ADM1176
    float voltage = adm1176_get_voltage(&power_monitor);
    float current = adm1176_get_current(&power_monitor);
    LOG_INFO("Power Monitor - Voltage: %.3fV, Current: %.3fA", voltage,
             current);

    // Convert float into mV and mA and write to slate
    slate->battery_voltage = (uint16_t)(voltage * 1000); // Convert to mV
    slate->battery_current = (uint16_t)(current * 1000); // Convert to mA

    // Read telemetry data from the LT8491
    uint16_t solar_vin_voltage = mppt_get_vin_voltage(&solar_charger_monitor);
    uint16_t solar_voltage = mppt_get_voltage(&solar_charger_monitor);
    uint16_t solar_current = mppt_get_current(&solar_charger_monitor);
    uint16_t solar_battery_voltage =
        mppt_get_battery_voltage(&solar_charger_monitor);
    uint16_t solar_battery_current =
        mppt_get_battery_current(&solar_charger_monitor);

    bool solar_charge = is_fixed_solar_charging();
    bool solar_fault = is_fixed_solar_faulty();
    bool panel_A = is_flex_panel_A_deployed();
    bool panel_B = is_flex_panel_B_deployed();
    bool rbf_detected = is_rbf_pin_detected();

    // LOG_INFO("Solar Charger - Voltage: %umV, Current: %umA", solar_voltage,
    //          solar_current);
    // LOG_INFO("Solar Charger - VBAT: %umV, Current: %umA",
    // solar_battery_voltage,
    //          solar_battery_current);
    // LOG_INFO("Solar Charger - VIN: %umV", solar_vin_voltage);

    // LOG_INFO("Panel A status: %s", panel_A ? "deployed" : "closed");
    // LOG_INFO("Panel B status: %s", panel_B ? "deployed" : "closed");
    // LOG_INFO("Fixed solar charging: %s", solar_charge ? "on" : "off");
    // LOG_INFO("Fixed solar status: %s", solar_fault ? "faulty" : "okay");

    // Write to slate
    slate->solar_voltage = solar_voltage;
    slate->solar_current = solar_current;
    slate->fixed_solar_charge = solar_charge;
    slate->fixed_solar_fault = solar_fault;
    slate->panel_A_deployed = panel_A;
    slate->panel_B_deployed = panel_B;

    // LOG_INFO("GPIO bits: %16lX", (uint64_t)gpio_get_all64());

    slate->is_rbf_detected = rbf_detected;
    // LOG_INFO("RBF_PIN status: %s",
    //          slate->is_rbf_detected ? "STILL ATTACHED!" : "REMOVED!");

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
                               /* Set to an actual value on init */
                               .next_dispatch = 0};
