#pragma once

#define SPI0_CLK (18)
#define SPI0_TX (19)
#define SPI0_RX (16)

// TODO: this is a bad, leaky abstraction
#define RFM9X_SPI (spi0)

#define RFM9X_CLK (SPI0_CLK)
#define RFM9X_TX (SPI0_TX)
#define RFM9X_RX (SPI0_RX)
#define RFM9X_RESET (21)
#define RFM9X_CS (20)
#define RFM9X_D0 (28)
