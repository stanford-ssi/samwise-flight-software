#include "adcs_driver.h"

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
