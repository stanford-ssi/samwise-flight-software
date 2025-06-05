#include "fixed_solar.h"

void init_fixed_solar_charge() {
    gpio_init(SAMWISE_FIXED_SOLAR_CHARGE);
    gpio_set_dir(SAMWISE_FIXED_SOLAR_CHARGE, GPIO_OUT);
}

bool read_fixed_solar_charge() {
    int status = gpio_get(SAMWISE_FIXED_SOLAR_CHARGE);
}

void init_fixed_solar_fault() {
    gpio_init(SAMWISE_FIXED_SOLAR_FAULT);
    gpio_set_dir(SAMWISE_FIXED_SOLAR_FAULT, GPIO_OUT);
}

bool read_fixed_solar_fault(void) {
    int status = gpio_get(SAMWISE_FIXED_SOLAR_FAULT);
}


