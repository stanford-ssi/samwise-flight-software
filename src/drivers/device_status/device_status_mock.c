#include "device_status.h"

// Panel deploy detect state, settable by tests (see test_fsm.c)
bool mock_panel_A_deployed = false;
bool mock_panel_B_deployed = false;

void device_status_init()
{
    // TODO: Track init state for test assertions
}
bool is_fixed_solar_charging(void)
{
    // TODO: Allow tests to configure return value
    return false;
}
bool is_fixed_solar_faulty(void)
{
    // TODO: Allow tests to configure return value
    return false;
}
bool is_flex_panel_A_deployed(void)
{
    return mock_panel_A_deployed;
}
bool is_flex_panel_B_deployed(void)
{
    return mock_panel_B_deployed;
}
bool is_rbf_pin_detected(void)
{
    // TODO: Allow tests to configure return value
    return false;
}
