#include "adcs_driver.h"

adcs_result_t adcs_driver_init(slate_t *slate)
{
    // TODO: Track init state for test assertions
    return ADCS_SUCCESS;
}
adcs_result_t adcs_driver_power_on(slate_t *slate)
{
    // TODO: Track power state for test assertions
    return ADCS_SUCCESS;
}
adcs_result_t adcs_driver_power_off(slate_t *slate)
{
    // TODO: Track power state for test assertions
    return ADCS_SUCCESS;
}
bool adcs_driver_is_alive(slate_t *slate)
{
    // TODO: Allow tests to simulate ADCS connection state
    return true;
}
adcs_result_t adcs_driver_get_telemetry(slate_t *slate, adcs_packet_t *packet)
{
    // TODO: Allow tests to inject mock ADCS telemetry data
    return ADCS_SUCCESS;
}
