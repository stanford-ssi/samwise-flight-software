#include "rfm9x.h"
#include "rfm9x_spi.c"
#include "rfm9x_registers.c"
#include "rfm9x_packet.c"
#include "rfm9x_interrupts.c"
#include "rfm9x_utils.c"

// The rest of the original rfm9x.c content (rfm9x_mk, etc.)
rfm9x_t rfm9x_mk()
{
    rfm9x_t r = {.reset_pin = SAMWISE_RF_RST_PIN,
                 .spi_cs_pin = SAMWISE_RF_CS_PIN,
                 .spi_tx_pin = SAMWISE_RF_MOSI_PIN,
                 .spi_rx_pin = SAMWISE_RF_MISO_PIN,
                 .spi_clk_pin = SAMWISE_RF_SCK_PIN,
                 .d0_pin = SAMWISE_RF_D0_PIN,
                 .tx_irq = NULL,
                 .rx_irq = NULL,
                 .spi = SPI_INSTANCE(SAMWISE_RF_SPI),
#ifndef PICO
                 .rf_reg_pin = SAMWISE_RF_REGULATOR_PIN,
#endif
                 /*
                  * Default values
                  */
                 .seq = 0,
                 .high_power = 1,
                 .max_power = 0,
                 .debug = 0};
    return r;
}

void rfm9x_init(rfm9x_t *r)
{
    ASSERT(radio_with_interrupts == NULL);
    radio_with_interrupts = r;

#ifndef PICO
    // Setup RF regulator
    gpio_init(r->rf_reg_pin);
    gpio_set_dir(r->rf_reg_pin, GPIO_OUT);

#ifdef BRINGUP
    gpio_put(r->rf_reg_pin, 0);
#else
    gpio_put(r->rf_reg_pin, 1);
#endif

#endif

    // Setup reset line
    gpio_init(r->reset_pin);
    gpio_set_dir(r->reset_pin, GPIO_IN);
    gpio_disable_pulls(r->reset_pin);
    gpio_put(r->reset_pin, 1);

    // Setup cs line
    gpio_init(r->spi_cs_pin);
    gpio_set_dir(r->spi_cs_pin, GPIO_OUT);
    gpio_disable_pulls(r->spi_cs_pin);
    gpio_put(r->spi_cs_pin, 1);

    // SPI
    gpio_set_function(r->spi_clk_pin, GPIO_FUNC_SPI);
    gpio_set_function(r->spi_tx_pin, GPIO_FUNC_SPI);
    gpio_set_function(r->spi_rx_pin, GPIO_FUNC_SPI);
    gpio_set_function(17, GPIO_FUNC_SPI); // ???

    // Setup interrupt line
    gpio_init(r->d0_pin);
    gpio_set_dir(r->d0_pin, GPIO_IN);
    gpio_pull_down(r->d0_pin);

    // Initialize SPI for the RFM9X
    busy_wait_ms(10);

    // RFM9X.pdf 4.3 p75:
    // CPOL = 0, CPHA = 0 (mode 0)
    // MSB first
    spi_init(r->spi, RFM9X_SPI_BAUDRATE);
    spi_set_format(r->spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // Bring the RFM9X out of reset
    rfm9x_reset(r);

    // Check the version number.
    uint8_t version = rfm9x_version(r);
    if (version != 0x12)
    {
        if (r->debug)
            printf("RFM9X version is incorrect: 0x%02x\r\n", version);
        //while (1)
        //    ; // Loop forever.
    }

    // Configure the RFM9X - these are good default values.
    rfm9x_set_lora(r, 1);
    rfm9x_set_frequency(r, RFM9X_FREQUENCY);
    rfm9x_set_output_power(r, r->high_power ? (r->max_power ? 0x0f : 0x0e) : 0x05); // 17dBm or 13dBm
    rfm9x_set_coding_rate(r, 5);
    rfm9x_set_preamble_length(r, 8);
    rfm9x_set_spreading_factor(r, 7);
    rfm9x_set_bandwidth(r, RFM9X_BANDWIDTH);
    rfm9x_set_crc(r, 1);
    rfm9x_set_lna_gain(r, 1);
    rfm9x_set_agc_auto(r, 1);

    // Set the FIFO base addresses.
    rfm9x_set_tx_base_address(r, 0);
    rfm9x_set_rx_base_address(r, 0);

    // Put the radio into receive mode.
    rfm9x_set_mode(r, RX_MODE);
}

