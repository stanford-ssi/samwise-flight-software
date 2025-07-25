#pragma once

#include "diskio.h"
#include "ff.h"

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

/*
 * Creates a Micro SD Card helper struct.
 */
sd_card_t micro_sd_mk();

/*
 * Initializes an Micro SD Card Instance
 */
void micro_sd_init(sd_card_t *s);

/*
 * Writes a packet of information
 * onto a File on the Micro SD Card
 */
bool micro_sd_write_packet(sd_card_t *s, char *ptf, uint8_t *buf);

/*
 * Mounts the Micro SD card
 */
bool micro_sd_mount(sd_card_t *s);

/*
 * Unmounts the Micro SD card
 */
bool micro_sd_unmount(sd_card_t *s);

/*
 * Changes to the given directory
 */
bool micro_sd_cd(sd_card_t *s, char *ptd);

/*
 * Creates a new directory
 */
bool micro_sd_mkdir(sd_card_t *s, char *ptd);

/*
 * Creates an empty file
 */
bool micro_sd_touch(sd_card_t *s, char *ptd);

/*
 * Moves a singular file from an given
 * path, and moves it to another path
 */
bool micro_sd_mv(sd_card_t *s, char *ptf, char *ptd);

/*
 * Copies a singular file from a given
 * path, and copies it to another path
 */
bool micro_sd_cp(sd_card_t *s, char *ptf, char *ptd);

/*
 * Outputs the file contents onto a buffer
 */
void micro_sd_cat(sd_card_t *s, char *ptf, uint8_t *buf);

/*
 * Removes a singular file
 */
bool micro_sd_rm(sd_card_t *s, char *ptf);

/*
 * Removes a singular directory and its contents
 */
bool micro_sd_rm_dir(sd_card_t *s, char *ptd);

/*
 * Outputs the current directory absolute path into a buffer
 */
void micro_sd_pwd(sd_card_t *s, uint8_t *buf);
