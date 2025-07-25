#include "sd_card.h"

#define SPI_DATA_TRANSFER 8

#define MOUNT_NAME "C:"

sd_card_t micro_sd_mk()
{
    sd_card_t s = {.pcName = MOUNT_NAME,
                   .spi = &(spi_t){.hw_inst = SPI_INSTANCE(SAMWISE_SD_SPI),
                                   .miso_gpio = SAMWISE_SD_MISO_PIN,
                                   .mosi_gpio = SAMWISE_SD_MOSI_PIN,
                                   .sck_gpio = SAMWISE_SD_SCK_PIN,
                                   .baud_rate = MICRO_SD_INIT_BAUDRATE},
                   .ss_gpio = SAMWISE_SD_CS_PIN,
                   .use_card_detect = false,
                   .card_detect_gpio = 0,
                   .card_detected_true = -1};

    return s;
}

void micro_sd_init(sd_card_t *s)
{
    // Setup CS line
    gpio_init(SAMWISE_SD_CS_PIN);
    gpio_set_dir(SAMWISE_SD_CS_PIN, GPIO_OUT);
    gpio_disable_pulls(SAMWISE_SD_CS_PIN);
    gpio_set_function(SAMWISE_SD_CS_PIN,
                      GPIO_FUNC_SIO); // Explicit just in case
    gpio_put(SAMWISE_SD_CS_PIN, 1);

    // SPI
    gpio_set_function(SAMWISE_SD_SCK_PIN, GPIO_FUNC_SPI);  // CLK
    gpio_set_function(SAMWISE_SD_MOSI_PIN, GPIO_FUNC_SPI); // MOSI
    gpio_set_function(SAMWISE_SD_MISO_PIN, GPIO_FUNC_SPI); // MISO

    // Initialize SPI for the Micro SD Card
    spi_init(s->spi, MICRO_SD_SPI_BAUDRATE);
    spi_set_format(s->spi, SPI_DATA_TRANSFER, SPI_CPOL_0, SPI_CPHA_0,
                   SPI_MSD_FIRST);
}

bool micro_sd_write_packet(sd_card_t *s, char *ptf, uint8_t *buf)
{
    // Ensures that the file exists
    FRESULT fr = f_mkdir(ptf);
    if (FR_OK != fr && FR_EXIST != fr)
    {
        LOG_ERROR("f_mkdir error: %s (%d)", FRESULT_str(fr), fr);
        return false;
    }

    // Opens the file

    // Writes the packet onto the file

    return true;
}
