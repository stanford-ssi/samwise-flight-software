// Mock driver function stubs for testing
// These are no-op implementations that allow tests to link

#include "adcs_driver.h"
#include "adm1176.h"
#include "device_status.h"
#include "mppt.h"
#include "rfm9x.h"
#include "watchdog.h"
#include <stdbool.h>
#include <stdint.h>

// RFM9X stubs
void rfm9x_print_parameters(rfm9x_t *r)
{
}
uint32_t rfm9x_version(rfm9x_t *r)
{
    return 0x12;
}
uint8_t rfm9x_tx_done(rfm9x_t *r)
{
    return 1;
}
void rfm9x_transmit(rfm9x_t *r)
{
}
void rfm9x_listen(rfm9x_t *r)
{
}

// ADM1176 stubs
adm1176_t adm1176_mk(i2c_inst_t *i2c, uint8_t address, float sense_resistor)
{
    adm1176_t mock = {0};
    return mock;
}
float adm1176_get_voltage(adm1176_t *adm)
{
    return 3.3;
}
float adm1176_get_current(adm1176_t *adm)
{
    return 0.1;
}

// MPPT stubs - return uint16_t
mppt_t mppt_mk(i2c_inst_t *i2c, uint8_t address)
{
    mppt_t mock = {0};
    return mock;
}
void mppt_init(mppt_t *mppt)
{
}
uint16_t mppt_get_vin_voltage(mppt_t *mppt)
{
    return 5000;
}
uint16_t mppt_get_voltage(mppt_t *mppt)
{
    return 4200;
}
uint16_t mppt_get_current(mppt_t *mppt)
{
    return 500;
}
uint16_t mppt_get_battery_voltage(mppt_t *mppt)
{
    return 3700;
}
uint16_t mppt_get_battery_current(mppt_t *mppt)
{
    return 200;
}

// Device status stubs
bool is_fixed_solar_charging(void)
{
    return false;
}
bool is_fixed_solar_faulty(void)
{
    return false;
}
bool is_flex_panel_A_deployed(void)
{
    return false;
}
bool is_flex_panel_B_deployed(void)
{
    return false;
}
bool is_rbf_pin_detected(void)
{
    return false;
}

// Watchdog stubs
void watchdog_feed(struct watchdog *wd)
{
}

// ADCS driver stubs - return ADCS_SUCCESS (0)
adcs_result_t adcs_driver_init(slate_t *slate)
{
    return ADCS_SUCCESS;
}
adcs_result_t adcs_driver_power_on(slate_t *slate)
{
    return ADCS_SUCCESS;
}
adcs_result_t adcs_driver_power_off(slate_t *slate)
{
    return ADCS_SUCCESS;
}
bool adcs_driver_is_alive(slate_t *slate)
{
    return true;
}
adcs_result_t adcs_driver_get_telemetry(slate_t *slate, adcs_packet_t *packet)
{
    return ADCS_SUCCESS;
}

// Onboard LED stubs
void onboard_led_toggle(struct onboard_led *led)
{
}

// Packet authentication stub
bool is_packet_authenticated(packet_t *packet, uint32_t current_boot_count)
{
    return true;
}

// RFM9X packet formatting stub
void rfm9x_format_packet(packet_t *pkt, uint8_t dst, uint8_t src, uint8_t flags,
                         uint8_t seq, uint8_t len, uint8_t *data)
{
}

// Payload control stubs
void payload_turn_on(slate_t *slate)
{
}
void payload_turn_off(slate_t *slate)
{
}

// External state declarations for tests
#include "state_machine.h"
sched_state_t init_state = {.name = "init",
                            .num_tasks = 0,
                            .task_list = {NULL},
                            .get_next_state = NULL};
sched_state_t burn_wire_state = {.name = "burn_wire",
                                 .num_tasks = 0,
                                 .task_list = {NULL},
                                 .get_next_state = NULL};
sched_state_t burn_wire_reset_state = {.name = "burn_wire_reset",
                                       .num_tasks = 0,
                                       .task_list = {NULL},
                                       .get_next_state = NULL};
sched_state_t bringup_state = {.name = "bringup",
                               .num_tasks = 0,
                               .task_list = {NULL},
                               .get_next_state = NULL};

// Additional RFM9X stubs
// Additional RFM9X stubs
void rfm9x_clear_interrupts(rfm9x_t *r)
{
}
uint8_t rfm9x_packet_to_fifo(rfm9x_t *r, uint8_t *buf, uint8_t n)
{
    return n;
}
uint8_t rfm9x_packet_from_fifo(rfm9x_t *r, uint8_t *buf)
{
    return 0;
}
void rfm9x_set_tx_irq(rfm9x_t *r, void (*callback)(void))
{
}
void rfm9x_set_rx_irq(rfm9x_t *r, void (*callback)(void))
{
}

// Payload UART stubs
void payload_uart_init(slate_t *slate)
{
}
uint16_t payload_uart_read_packet(slate_t *slate, uint8_t *packet)
{
    return 0;
}
void payload_uart_write_packet(slate_t *slate, const uint8_t *packet,
                               uint16_t len, uint32_t timeout_ms)
{
}
