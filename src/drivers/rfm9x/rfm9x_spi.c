#include "rfm9x.h"
#include "rfm9x_spi.h"

static inline void cs_select(rfm9x_t *r)
{
    busy_wait_us(5);
    gpio_put(r->spi_cs_pin, 0);
    busy_wait_us(5);
}

static inline void cs_deselect(rfm9x_t *r)
{
    busy_wait_us(5);
    gpio_put(r->spi_cs_pin, 1);
    busy_wait_us(5);
}

/*
 * Read a buffer from a register address.
 */
void rfm9x_get_buf(rfm9x_t *r, rfm9x_reg_t reg, uint8_t *buf, uint32_t n)
{
    cs_select(r);

    // First, configure that we will be GETTING from the Radio Module.
    uint8_t value = reg & 0x7F;

    // WRITES to the radio module the value, of length 1 byte, that says that we
    // are GETTING
    spi_write_blocking(r->spi, &value, 1);

    // GETS from the radio module the buffer.
    // The 0 represents the arbitrary byte that should be passed IN as part of
    // the master/slave interaction.
    spi_read_blocking(r->spi, 0, buf, n);

    cs_deselect(r);
}

/*
 * Write a buffer to a register address.
 */
void rfm9x_put_buf(rfm9x_t *r, rfm9x_reg_t reg, uint8_t *buf, uint32_t n)
{
    cs_select(r);

    // this value will be passed in to tell the radio that we will be writing
    // data
    uint8_t value = reg | 0x80;

    spi_write_blocking(r->spi, &value, 1);

    // Write to the radio that
    spi_write_blocking(r->spi, buf, n);

    cs_deselect(r);
}

/*
 * Write a single byte to an RFM9X register
 */
void rfm9x_put8(rfm9x_t *r, rfm9x_reg_t reg, uint8_t v)
{
    rfm9x_put_buf(r, reg, &v, 1);
}

/*
 * Get a single byte from an RFM9X register
 */
uint8_t rfm9x_get8(rfm9x_t *r, rfm9x_reg_t reg)
{
    uint8_t v = 0;
    rfm9x_get_buf(r, reg, &v, 1);
    return v;
}

void rfm9x_reset(rfm9x_t *r)
{
    // Reset the chip as per RFM9X.pdf 7.2.2 p109

    // set reset pin to output
    gpio_set_dir(r->reset_pin, GPIO_OUT);
    gpio_put(r->reset_pin, 0);

    sleep_us(100);

    // set reset pin to input
    gpio_set_dir(r->reset_pin, GPIO_IN);

    sleep_ms(5);
}

