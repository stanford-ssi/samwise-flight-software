#pragma once

#include <stdint.h>
#include <stddef.h>

// Mock SPI types and functions
typedef struct { uint8_t dummy; } spi_inst_t;

#define spi0 ((spi_inst_t *)0)
#define spi1 ((spi_inst_t *)1)

static inline void spi_init(spi_inst_t *spi, uint32_t baudrate) {}
static inline void spi_deinit(spi_inst_t *spi) {}
static inline int spi_write_blocking(spi_inst_t *spi, const uint8_t *src, size_t len) { return len; }
static inline int spi_read_blocking(spi_inst_t *spi, uint8_t repeated_tx_data, uint8_t *dst, size_t len) { return len; }
static inline int spi_write_read_blocking(spi_inst_t *spi, const uint8_t *src, uint8_t *dst, size_t len) { return len; }
