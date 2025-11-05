#include "filesys.h"

// variables used by the filesystem
lfs_t lfs;
lfs_file_t file;

// configuration of the filesystem is provided by this struct
const struct lfs_config cfg = {
    // block device operations
    // .read  = mram_read,
    // .prog  = mram_read_status,
    // .erase = mram_clear,
    // .sync  = mram_,

    // block device configuration
    .read_size = 16,     .prog_size = 16,   .block_size = 256,
    .block_count = 128,  .cache_size = 256, .lookahead_size = 16,
    .block_cycles = 500,
};

void filesys_task_init(slate_t *slate)
{
}

// entry point
int ex_lfs(void)
{
    // mount the filesystem
    // int err = lfs_mount(&lfs, &cfg);

    // // reformat if we can't mount the filesystem
    // // this should only happen on the first boot
    // if (err) {
    //     lfs_format(&lfs, &cfg);
    //     lfs_mount(&lfs, &cfg);
    // }

    // // read current count
    // uint32_t boot_count = 0;
    // lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    // lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    // // update boot count
    // boot_count += 1;
    // lfs_file_rewind(&lfs, &file);
    // lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    // // remember the storage is not updated until the file is closed
    // successfully lfs_file_close(&lfs, &file);

    // // release any resources we were using
    // lfs_unmount(&lfs);

    // print the boot count
    printf("Filesys_Run_test: \n", "this is a test" /*boot_count*/);
}

void filesys_task_dispatch(slate_t *slate)
{
    onboard_led_toggle(&slate->onboard_led);
    ex_lfs();
}
