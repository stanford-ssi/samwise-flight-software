#include "lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI/sd_driver/hw_config.h"
#include "pins.h"

#define MOUNT_NAME "C:"
#define BAUDRATE 400 * 1000 // 400khz

/* Configuration of hardware SPI object */
static spi_t spi = {
    .hw_inst = SPI_INSTANCE(SAMWISE_SD_SPI),  
    .sck_gpio = SAMWISE_SD_SCK_PIN,           // GPIO 18 (from pin definitions)
    .mosi_gpio = SAMWISE_SD_MOSI_PIN,         // GPIO 19 (from pin definitions) 
    .miso_gpio = SAMWISE_SD_MISO_PIN,         // GPIO 16 (from pin definitions)
    .baud_rate = BAUDRATE                      
};

/* Hardware Configuration of the SD Card  */
static sd_card_t sd_card = {
    .pcName = MOUNT_NAME,                   
    .spi = &spi,                      
    .ss_gpio = SAMWISE_SD_CS_PIN,     // GPIO 17 (from pin definitions)
    .use_card_detect = false,         
    .card_detect_gpio = 0,            
    .card_detected_true = -1         
};

size_t sd_get_num() { return 1; }
sd_card_t *sd_get_by_num(size_t num) {
    if (num < sd_get_num()) {
        return &sd_card;
    } else {
        return NULL;
    }
}
size_t spi_get_num() { return 1; }
spi_t *spi_get_by_num(size_t num) {
    if (num < spi_get_num()) {
        return &spi;
    } else {
        return NULL;
    }
}