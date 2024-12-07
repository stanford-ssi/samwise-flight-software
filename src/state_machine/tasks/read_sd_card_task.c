#include "read_sd_card_task.h"
#include "slate.h"
#include "macros.h"

#include <stdio.h>
#include "f_util.h"
#include "ff_stdio.h"
#include "pico/stdlib.h"
#include "rtc.h"
#include "hw_config.h"
void read_sd_card_task_init(slate_t *slate) {
    stdio_init_all();
    time_init();
    LOG_INFO("Initialized read SD card");
}

// TODO: should we be tracking the returns?
void read_sd_card_task_dispatch(slate_t *slate) {
    FF_FILE *file;
    static const DWORD data_to_write[] = {0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321};
    unsigned long DATA_SIZE = sizeof(data_to_write);
    const char* pathname = "test.txt";

    DWORD buffer[DATA_SIZE/ sizeof(DWORD)];

    // Open file for reading
    file = ff_fopen(pathname, "r");
    if (!file) {
        printf("Failed to open file for reading: %s\n", pathname);
    }

    // Read the data from the file
    if (ff_fread(buffer, DATA_SIZE, 1, file) < 1) {
        printf("Failed to read from file: %s\n", pathname);
        ff_fclose(file);
    } 

    // Print the read data
    printf("Data read from file:\n");
    for (size_t i = 0; i < DATA_SIZE / sizeof(DWORD); ++i) {
        printf("0x%08lx\n", buffer[i]);
    }

    ff_fclose(file);
}

sched_task_t read_sd_card_task = {.name = "read SD card",
                           .dispatch_period_ms = 1000,
                           .task_init = &read_sd_card_task_init,
                           .task_dispatch = &read_sd_card_task_dispatch,

                           /* Set to an actual value on init */
                           .next_dispatch = 0};