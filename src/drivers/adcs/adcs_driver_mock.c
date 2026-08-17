#include "adcs_driver.h"
#include "logger.h"
#include "slate.h"

adcs_result_t adcs_driver_init()
{
    LOG_INFO("ADCS MOCK");
    return ADCS_SUCCESS;
}

adcs_result_t adcs_driver_power_on()
{
    LOG_INFO("ADCS MOCK");
    return ADCS_SUCCESS;
}

adcs_result_t adcs_driver_power_off()
{
    LOG_INFO("ADCS MOCK");
    return ADCS_SUCCESS;
}

adcs_result_t adcs_driver_get_telemetry(adcs_packet_t *packet)
{
    LOG_INFO("ADCS MOCK");
    return ADCS_SUCCESS;
}

void adcs_print_telemetry(adcs_packet_t *packet)
{
    LOG_INFO("ADCS MOCK");
}

bool adcs_driver_is_alive()
{
    LOG_INFO("ADCS MOCK");
    return true;
}

uint32_t receive_msg(msg_t *msg, uint8_t *rx_buf)
{
    LOG_INFO("ADCS MOCK");
    return 0;
}

void send_msg(msg_t *msg, uint32_t len)
{
    LOG_INFO("ADCS MOCK");
}

void send_ping()
{
    LOG_INFO("ADCS MOCK");
}

void send_pong()
{
    LOG_INFO("ADCS MOCK");
}

void send_command(uint8_t command)
{
    LOG_INFO("ADCS MOCK");
}
