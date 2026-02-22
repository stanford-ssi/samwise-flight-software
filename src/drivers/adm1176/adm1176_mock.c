#include "adm1176.h"

adm1176_t adm1176_mk(i2c_inst_t *i2c, uint8_t address, float sense_resistor)
{
    adm1176_t mock = {0};
    return mock;
}
float adm1176_get_voltage(adm1176_t *dev)
{
    return 3.3;
}
float adm1176_get_current(adm1176_t *dev)
{
    return 0.1;
}
