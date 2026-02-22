#include "mram_test.h"

void filesys_int_main(void)
{
    stdio_init_all();
    LOG_DEBUG("Starting MRAM integration tests...\n");
    test_mram_write_read();
    test_mram_write_disable_enable();
    test_mram_check_collision();
    test_mram_preserve_data_on_sleep();
    test_mram_clear();
    LOG_DEBUG("All MRAM integration tests passed!\n");
}
