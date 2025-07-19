#include "deployment.h"

void init_panel_A() {
    gpio_init(SAMWISE_PANEL_A);
    gpio_set_dir(SAMWISE_PANEL_A, GPIO_OUT);
}

bool read_panel_A() {
    int status = gpio_get(SAMWISE_PANEL_A);
}

void init_panel_B() {
    gpio_init(SAMWISE_PANEL_B);
    gpio_set_dir(SAMWISE_PANEL_B, GPIO_OUT);
}

bool read_panel_B() {
    int status = gpio_get(SAMWISE_PANEL_B);
}