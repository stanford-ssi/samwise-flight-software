#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define PICO_DEFAULT_LED_PIN 0

typedef unsigned int uint;

void sleep_ms(uint32_t);
void sleep_us(uint64_t);
void busy_wait_ms(uint32_t delay_ms);
void busy_wait_us(uint64_t delay_us);

#define GPIO_OUT 0
#define GPIO_IN 1
#define GPIO_IRQ_EDGE_RISE 12345
#define GPIO_FUNC_SPI 0

void gpio_pull_down(uint gpio);
void gpio_init(uint gpio);
void gpio_set_dir(uint gpio, bool out);
void gpio_put(uint gpio, int value);
void gpio_disable_pulls(uint gpio);
void gpio_set_function(uint gpio, uint fn);

/** \brief Enumeration of SPI CPHA (clock phase) values.
 *  \ingroup hardware_spi
 */
typedef enum {
    SPI_CPHA_0 = 0,
    SPI_CPHA_1 = 1
} spi_cpha_t;

/** \brief Enumeration of SPI CPOL (clock polarity) values.
 *  \ingroup hardware_spi
 */
typedef enum {
    SPI_CPOL_0 = 0,
    SPI_CPOL_1 = 1
} spi_cpol_t;

/** \brief Enumeration of SPI bit-order values.
 *  \ingroup hardware_spi
 */
typedef enum {
    SPI_LSB_FIRST = 0,
    SPI_MSB_FIRST = 1
} spi_order_t;

typedef struct spi_inst spi_inst_t;
typedef void (*gpio_irq_callback_t)(uint, uint32_t);

#define spi0 ((spi_inst_t *)0x40080000)

uint spi_init(spi_inst_t *spi, uint baudrate);
void spi_write_blocking(spi_inst_t *spi, const uint8_t *src, size_t len);
void spi_read_blocking(spi_inst_t *spi, uint8_t repeated_tx_data, uint8_t *dst, size_t len);
void spi_set_format(spi_inst_t *spi, uint data_bits, spi_cpol_t cpol, spi_cpha_t cpha, spi_order_t order);
void gpio_set_irq_enabled_with_callback(uint gpio, uint32_t event_mask, bool enabled, gpio_irq_callback_t callback);
