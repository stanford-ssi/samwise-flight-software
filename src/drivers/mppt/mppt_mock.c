#include "mppt.h"

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
