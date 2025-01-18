#pragma once

#include "hardware/spi.h"
#include "pico/stdlib.h"

#define PACKET_SIZE 256
#define PAYLOAD_SIZE 251

#define RFM9X_SPI_BAUDRATE (1000 * 1000)
#define RFM9X_FREQUENCY 438100000
#define RFM9X_BANDWIDTH 125000

typedef enum
{
    SLEEP_MODE = 0,
    STANDBY_MODE = 1,
    FS_TX_MODE = 2,
    TX_MODE = 3,
    FS_RX_MODE = 4,
    RX_MODE = 5,
} rfm9x_mode_t;

typedef void (*rfm9x_interrupt_func)(uint, uint32_t);

typedef struct _rfm9x
{
    uint reset_pin;
    uint spi_cs_pin;
    uint spi_rx_pin;
    uint spi_tx_pin;
    uint spi_clk_pin;
    uint d0_pin;

    rfm9x_interrupt_func interrupt_func;

    spi_inst_t *spi;
    uint8_t seq; /* current sequence number */
    uint32_t high_power : 1, max_power : 1, debug : 1;
} rfm9x_t;

/*
 * Creates an RFM9X helper struct. Uninitialized.
 */
rfm9x_t rfm9x_mk(spi_inst_t *spi, uint reset_pin, uint cs_pin, uint spi_tx_pin,
                 uint spi_rx_pin, uint spi_clk_pin, uint d0_pin,
                 rfm9x_interrupt_func interrupt_func);

/*
 * Initializes an RFM9X radio.
 */
void rfm9x_init(rfm9x_t *r);

#define _SAP_FLAGS_ACK_REQUEST 2

/*
 * Returns the chip version from the RFM9X
 */
uint32_t rfm9x_version(rfm9x_t *r);

/*
 * Send a raw transmission from the RFM9X.
 *
 * r: the radio
 * data: the data to send
 * l: the length of the data. Must be less than `PAYLOAD_SIZE`
 * keep_listening: 0 to stop listening after sending, 1 to keep blocking
 * destination: radio to send it to. 255 is broadcast.
 * node: our address
 * identifier: Sequence number — if sending multiple packets, increment by one
 * per packet.
 * flags:
 */
uint8_t rfm9x_send(rfm9x_t *r, char *data, uint32_t l, uint8_t keep_listening,
                   uint8_t destination, uint8_t node, uint8_t identifier,
                   uint8_t flags);

/*
 * Send a transmission.
 *
 * Sends l bytes of data, tagged with the current seq number. Waits for an ack.
 *
 * Returns 1 if an ack was received, 0 otherwise.
 */
uint8_t rfm9x_send_ack(rfm9x_t *r, char *data, uint32_t l, uint8_t destination,
                       uint8_t node, uint8_t max_retries);

/*
 * Receive a transmission.
 */
uint8_t rfm9x_receive(rfm9x_t *r, char *packet, uint8_t node,
                      uint8_t keep_listening, uint8_t with_ack,
                      bool blocking_wait_for_packet);

void rfm9x_listen(rfm9x_t *r);
void rfm9x_transmit(rfm9x_t *r);

uint8_t rfm9x_tx_done(rfm9x_t *r);
uint8_t rfm9x_rx_done(rfm9x_t *r);

uint8_t rfm9x_packet_to_fifo(rfm9x_t *r, uint8_t *buf, uint8_t n);
uint8_t rfm9x_packet_from_fifo(rfm9x_t *r, uint8_t *buf);
void rfm9x_clear_interrupts(rfm9x_t *r);

typedef enum
{
    _RH_RF95_REG_00_FIFO = 0x00,
    _RH_RF95_REG_01_OP_MODE = 0x01,
    _RH_RF95_REG_06_FRF_MSB = 0x06,
    _RH_RF95_REG_07_FRF_MID = 0x07,
    _RH_RF95_REG_08_FRF_LSB = 0x08,
    _RH_RF95_REG_09_PA_CONFIG = 0x09,
    _RH_RF95_REG_0A_PA_RAMP = 0x0A,
    _RH_RF95_REG_0B_OCP = 0x0B,
    _RH_RF95_REG_0C_LNA = 0x0C,
    _RH_RF95_REG_0D_FIFO_ADDR_PTR = 0x0D,
    _RH_RF95_REG_0E_FIFO_TX_BASE_ADDR = 0x0E,
    _RH_RF95_REG_0F_FIFO_RX_BASE_ADDR = 0x0F,
    _RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR = 0x10,
    _RH_RF95_REG_11_IRQ_FLAGS_MASK = 0x11,
    _RH_RF95_REG_12_IRQ_FLAGS = 0x12,
    _RH_RF95_REG_13_RX_NB_BYTES = 0x13,
    _RH_RF95_REG_14_RX_HEADER_CNT_VALUE_MSB = 0x14,
    _RH_RF95_REG_15_RX_HEADER_CNT_VALUE_LSB = 0x15,
    _RH_RF95_REG_16_RX_PACKET_CNT_VALUE_MSB = 0x16,
    _RH_RF95_REG_17_RX_PACKET_CNT_VALUE_LSB = 0x17,
    _RH_RF95_REG_18_MODEM_STAT = 0x18,
    _RH_RF95_REG_19_PKT_SNR_VALUE = 0x19,
    _RH_RF95_REG_1A_PKT_RSSI_VALUE = 0x1A,
    _RH_RF95_REG_1B_RSSI_VALUE = 0x1B,
    _RH_RF95_REG_1C_HOP_CHANNEL = 0x1C,
    _RH_RF95_REG_1D_MODEM_CONFIG1 = 0x1D,
    _RH_RF95_REG_1E_MODEM_CONFIG2 = 0x1E,
    _RH_RF95_REG_1F_SYMB_TIMEOUT_LSB = 0x1F,
    _RH_RF95_REG_20_PREAMBLE_MSB = 0x20,
    _RH_RF95_REG_21_PREAMBLE_LSB = 0x21,
    _RH_RF95_REG_22_PAYLOAD_LENGTH = 0x22,
    _RH_RF95_REG_23_MAX_PAYLOAD_LENGTH = 0x23,
    _RH_RF95_REG_24_HOP_PERIOD = 0x24,
    _RH_RF95_REG_25_FIFO_RX_BYTE_ADDR = 0x25,
    _RH_RF95_REG_26_MODEM_CONFIG3 = 0x26,

    /**
     * In this register:
     * Bits 7-6: Dio0Mapping
     * Bits 5-4: Dio1Mapping
     * Bits 3-2: Dio2Mapping
     * Bits 1-0: Dio3Mapping
     *
     * (p. 100 of documentation)
     */
    _RH_RF95_REG_40_DIO_MAPPING1 = 0x40,
    _RH_RF95_REG_41_DIO_MAPPING2 = 0x41,
    _RH_RF95_REG_42_VERSION = 0x42,

    _RH_RF95_REG_4B_TCXO = 0x4B,
    _RH_RF95_REG_4D_PA_DAC = 0x4D,
    _RH_RF95_REG_5B_FORMER_TEMP = 0x5B,
    _RH_RF95_REG_61_AGC_REF = 0x61,
    _RH_RF95_REG_62_AGC_THRESH1 = 0x62,
    _RH_RF95_REG_63_AGC_THRESH2 = 0x63,
    _RH_RF95_REG_64_AGC_THRESH3 = 0x64,

    _RH_RF95_DETECTION_OPTIMIZE = 0x31,
    _RH_RF95_DETECTION_THRESHOLD = 0x37,

    _RH_RF95_PA_DAC_DISABLE = 0x04,
    _RH_RF95_PA_DAC_ENABLE = 0x07,

    // The Frequency Synthesizer step = RH_RF95_FXOSC / 2^^19
    _RH_RF95_FSTEP = 32000000 / 524288,

    // RadioHead specific compatibility constants.
    _RH_BROADCAST_ADDRESS = 0xFF,

    // The acknowledgement bit in the FLAGS
    // The top 4 bits of the flags are reserved for RadioHead. The lower 4 bits
    // are reserved for application layer use.
    _RH_FLAGS_ACK = 0x80,
    _RH_FLAGS_RETRY = 0x40,
} rfm9x_reg_t;
