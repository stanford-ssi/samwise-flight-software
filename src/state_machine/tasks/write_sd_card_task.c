#include "write_sd_card_task.h"
#include "slate.h"
#include "macros.h"

#include "ff_stdio.h"
#include <stdio.h>
#include <stdlib.h>
void write_sd_card_task_init(slate_t *slate) {
    LOG_INFO("Initialized write SD card");
}

// TODO: how do we track errors?
void write_sd_card_task_dispatch(slate_t *slate) {
    const char* pathname = "test.txt";
    static const DWORD data_to_write[] = {0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321};

    FF_FILE *file;

    // Open file for writing
    file = ff_fopen(pathname, "w");
    if (!file) {
        printf("Failed to open file for writing: %s\n", pathname);
    }

    // Write the array to the file
    if (ff_fwrite(data_to_write, sizeof(data_to_write), 1, file) < 1) {
        printf("Failed to write to file: %s\n", pathname);
        ff_fclose(file);
    }

    ff_fclose(file);
    printf("Array written to file successfully.\n");
    LOG_INFO("complete write SD card");
}