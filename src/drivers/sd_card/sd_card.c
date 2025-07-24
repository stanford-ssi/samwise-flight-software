#include "sd_card.h"

#define SPI_DATA_TRANSFER 8

micro_sd_t micro_sd_mk()
{
    micro_sd_t s = {.spi_clk_pin = SAMWISE_SD_SCK_PIN,
                    .spi_cs_pin = SAMWISE_SD_CS_PIN,
                    .spi_tx_pin = SAMWISE_SD_MOSI_PIN,
                    .spi_rx_pin = SAMWISE_SD_MISO_PIN,
                    .spi = SPI_INSTANCE(SAMWISE_SD_SPI)};

    return s;
}

void micro_sd_init(micro_sd_t *s)
{
    // Setup CS line
    gpio_init(s->spi_cs_pin);
    gpio_set_dir(s->spi_cs_pin, GPIO_OUT);
    gpio_disable_pulls(s->spi_cs_pin);
    gpio_set_function(s->spi_cs_pin, GPIO_FUNC_SIO); // Explicit just in case
    gpio_put(s->spi_cs_pin, 1);

    // SPI
    gpio_set_function(s->spi_clk_pin, GPIO_FUNC_SPI); // CLK
    gpio_set_function(s->spi_tx_pin, GPIO_FUNC_SPI);  // MOSI
    gpio_set_function(s->spi_rx_pin, GPIO_FUNC_SPI);  // MISO

    // Initialize SPI for the Micro SD Card
    spi_init(s->spi, MICRO_SD_SPI_BAUDRATE);
    spi_set_format(s->spi, SPI_DATA_TRANSFER, SPI_CPOL_0, SPI_CPHA_0,
                   SPI_MSD_FIRST);
}
