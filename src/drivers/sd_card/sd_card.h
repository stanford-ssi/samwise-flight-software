#pragma once

#include "hardware/reset.h"
#include "hardware/spi.h"

#include "pico/stdlib.h"

#include "bit-support.h"
#include "logger.h"
#include "macros.h"
#include "packet.h"
#include "pins.h"

#define MICRO_SD_INIT_BAUDRATE 400 * 1000   // 400 Khz
#define MICRO_SD_SPI_BAUDRATE 125 * 1000000 // 12.5 Mhz

typedef struct _micro_sd
{
    uint spi_cs_pin;
    uint spi_clk_pin;
    uint spi_tx_pin;
    uint spi_rx_pin;

    spi_inst_t *spi;
} micro_sd_t;

/*
 * Creates a Micro SD Card helper struct. Uninitialized.
 */
micro_sd_t micro_sd_mk();

/*
 * Initializes an Micro SD Card Instance
 */
void micro_sd_init(micro_sd_t *s);
