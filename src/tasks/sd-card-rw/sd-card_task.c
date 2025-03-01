#include "sd-card_task.h"

FRESULT fr;
FATFS fs;
FIL fil;
char filename[] = "test02.txt";

void sd-card_task_init() {
    stdio_init_all();
    if (!sd_init_driver()) {
        ERROR("Could not initialize SD card\r\n");
    }
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        ERROR("Could not mount filesystem (%d)\r\n", fr);
    }
}

void sd-card_task_test_rw(){
    fr = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        ERROR("Could not open file (%d)\r\n", fr);
    }

    // Write something to file
    int ret = f_printf(&fil, "This is another test\r\n");
    if (ret < 0) {
        f_close(&fil);
       ERROR("Could not write to file (%d)\r\n", ret);
    }
    ret = f_printf(&fil, "of writing to an SD card.\r\n");
    if (ret < 0) {
        f_close(&fil);
        ERROR("Could not write to file (%d)\r\n", ret);
    }

    // Close file
    fr = f_close(&fil);
    if (fr != FR_OK) {
        ERROR("Could not close file (%d)\r\n", fr);
    }

    // Open file for reading
    fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK) {
        ERROR("Could not open file (%d)\r\n", fr);
    }

    // Print every line in file over serial
    printf("Reading from file '%s':\r\n", filename);
    printf("---\r\n");
    while (f_gets(buf, sizeof(buf), &fil)) {
        printf(buf);
    }
    printf("\r\n---\r\n");

    // Close file
    fr = f_close(&fil);
    if (fr != FR_OK) {
        ERROR("Could not close file (%d)\r\n", fr);
    }
    f_unmount("0:");
}

sched_task_t sd-card_task = {.name = "sd-card",
                           .dispatch_period_ms = 3000,
                           .task_init = &sd-card_task_init,
                           .task_dispatch = &sd-card_task_test_rw,

                           /* Set to an actual value on init */
                           .next_dispatch = 0};
