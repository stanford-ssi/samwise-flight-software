#include "telemetry_task.h"
#include "neopixel.h"

// Add power monitor instance
static adm1176_t power_monitor;
// Add MPPT instance
static mppt_t solar_charger_monitor

// GPIO pin definitions
#define CHRG_STATUS 34
#define FAULT_STATUS 33
#define SIDE_PANEL_A 10
#define SIDE_PANEL_B 9
#define RBF_DETECT 42

void telemetry_task_init(slate_t *slate)
{
#ifndef PICO
    LOG_INFO("Scanning the I2C bus...\n");
    bool found_device = false;
    for (uint8_t addr = 0x08; addr < 0x78; ++addr)
    {
        uint8_t rxdata;

        // Scan MPPT I2C
        LOG_INFO("Scanning MPPT_I2C address 0x%02X\n", addr);
        int ret = i2c_read_blocking_until(SAMWISE_MPPT_I2C, addr, &rxdata, 1, false,
                                          make_timeout_time_ms(I2C_TIMEOUT_MS));
        if (ret >= 0)
        {
            LOG_INFO("MPPT Device found at 0x%02X\n", addr);
            found_device = true;
        }

        // Scan power monitor I2C
        LOG_INFO("Scanning POWER_I2C address 0x%02X\n", addr);
        ret = i2c_read_blocking_until(SAMWISE_POWER_MONITOR_I2C, addr, &rxdata, 1, false,
                                      make_timeout_time_ms(I2C_TIMEOUT_MS));
        if (ret >= 0)
        {
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
    solar_charger_monitor = mppt_mk(SAMWISE_MPPT_I2C, LT8491_I2C_ADDR);
    mppt_init(&solar_charger_monitor);
#else
    // Initialize mocked PICO power monitor
    power_monitor = adm1176_mk_mock();

    // Initialize mocked PICO MPPT
    solar_charger_monitor = mppt_mk_mock();
#endif

    // ------------------------------
    // Initialize GPIO pins
    // ------------------------------
    gpio_init(CHRG_STATUS);
    gpio_set_dir(CHRG_STATUS, GPIO_IN);

    gpio_init(FAULT_STATUS);
    gpio_set_dir(FAULT_STATUS, GPIO_IN);

    gpio_init(SIDE_PANEL_A);
    gpio_set_dir(SIDE_PANEL_A, GPIO_IN);

    gpio_init(SIDE_PANEL_B);
    gpio_set_dir(SIDE_PANEL_B, GPIO_IN);

    gpio_init(RBF_DETECT);
    gpio_set_dir(RBF_DETECT, GPIO_IN);
}

void telemetry_task_dispatch(slate_t *slate)
{
    neopixel_set_color_rgb(TELEMETRY_TASK_COLOR);

    // ------------------------------
    // Read power monitor data
    // ------------------------------
    float voltage = adm1176_get_voltage(&power_monitor);
    float current = adm1176_get_current(&power_monitor);

    slate->battery_voltage = (uint16_t)(voltage * 1000); // mV
    slate->battery_current = (uint16_t)(current * 1000); // mA

    // ------------------------------
    // Read MPPT / solar charger data
    // ------------------------------
    uint16_t solar_vin_voltage = mppt_get_vin_voltage(&solar_charger_monitor);
    uint16_t solar_voltage = mppt_get_voltage(&solar_charger_monitor);
    uint16_t solar_current = mppt_get_current(&solar_charger_monitor);
    uint16_t solar_battery_voltage = mppt_get_battery_voltage(&solar_charger_monitor);
    uint16_t solar_battery_current = mppt_get_battery_current(&solar_charger_monitor);

    bool solar_charge = is_fixed_solar_charging();
    bool solar_fault = is_fixed_solar_faulty();

    slate->solar_voltage = solar_voltage;
    slate->solar_current = solar_current;
    slate->fixed_solar_charge = solar_charge;
    slate->fixed_solar_fault = solar_fault;

    // ------------------------------
    // Read single hardware pins
    // ------------------------------
    slate->fixed_solar_charge = gpio_get(CHRG_STATUS) == LOW;   // LOW = OK
    slate->fixed_solar_fault = gpio_get(FAULT_STATUS) == LOW;   // LOW = Faulty
    slate->panel_A_deployed = gpio_get(SIDE_PANEL_A) == HIGH;
    slate->panel_B_deployed = gpio_get(SIDE_PANEL_B) == HIGH;
    slate->is_rbf_detected = gpio_get(RBF_DETECT) == HIGH;

    // ------------------------------
    // ADCS telemetry (existing)
    // ------------------------------
    LOG_INFO("ADCS status: %s", slate->is_adcs_on ? "ON" : "OFF");
    LOG_INFO("ADCS telemetry valid: %s",
             slate->is_adcs_telem_valid ? "VALID" : "INVALID");
    LOG_INFO("ADCS num failed checks: %d", slate->adcs_num_failed_checks);

    neopixel_set_color_rgb(0, 0, 0);
}

sched_task_t telemetry_task = {
    .name = "telemetry",
    .dispatch_period_ms = 1000,
    .task_init = &telemetry_task_init,
    .task_dispatch = &telemetry_task_dispatch,
    .next_dispatch = 0
};
