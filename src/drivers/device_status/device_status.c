#include "device_status.h"

void device_status_init()
{
    gpio_init(SAMWISE_FIXED_SOLAR_CHARGE);
    gpio_set_dir(SAMWISE_FIXED_SOLAR_CHARGE, GPIO_IN);
    gpio_init(SAMWISE_FIXED_SOLAR_FAULT);
    gpio_set_dir(SAMWISE_FIXED_SOLAR_FAULT, GPIO_IN);
    gpio_init(SAMWISE_PANEL_A);
    gpio_set_dir(SAMWISE_PANEL_A, GPIO_IN);
    gpio_init(SAMWISE_PANEL_B);
    gpio_set_dir(SAMWISE_PANEL_B, GPIO_IN);
    gpio_init(SAMWISE_RBF_DETECT_PIN);
    gpio_set_dir(SAMWISE_RBF_DETECT_PIN, GPIO_IN);
}

bool is_fixed_solar_charging()
{
    return gpio_get(SAMWISE_FIXED_SOLAR_CHARGE) == 0;
}

bool is_fixed_solar_faulty()
{
    return gpio_get(SAMWISE_FIXED_SOLAR_FAULT) == 0;
}

bool is_flex_panel_A_deployed()
{
    return gpio_get(SAMWISE_PANEL_A) == 1;
}

bool is_flex_panel_B_deployed()
{
    return gpio_get(SAMWISE_PANEL_B) == 1;
}

bool is_rbf_pin_detected()
{
    return gpio_get(SAMWISE_RBF_DETECT_PIN) == 0;
}
