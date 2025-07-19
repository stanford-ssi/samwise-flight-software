#include "device_status.h"

void init_device_status_drivers()
{
    gpio_init(SAMWISE_FIXED_SOLAR_CHARGE);
    gpio_set_dir(SAMWISE_FIXED_SOLAR_CHARGE, GPIO_OUT);
    gpio_init(SAMWISE_FIXED_SOLAR_FAULT);
    gpio_set_dir(SAMWISE_FIXED_SOLAR_FAULT, GPIO_OUT);
}

bool is_fixed_solar_charging()
{
    return gpio_get(SAMWISE_FIXED_SOLAR_CHARGE) == 0;
}

bool is_fixed_solar_faulty()
{
    return gpio_get(SAMWISE_FIXED_SOLAR_FAULT) == 0;
}
