#include "mram_test.h"

void main(void)
{
    LOG_DEBUG("Starting MRAM test\n");
    mram_init();
    test_mram_write_read();
    test_mram_write_disable_enable();
    // TODO: For some reason, preserve_data_on_sleep not only makes an infinite
    // loop, but it even locks up the PICO such that you HAVE to click reset in
    // order for it to respond again!

    // test_mram_preserve_data_on_sleep();

    // test_mram_check_collision();
    // test_mram_clear();
    test_mram_read_status();
    LOG_DEBUG("All MRAM tests passed!\n");
    // LOG_DEBUG("Running allocation tests...\n");
    // mram_allocation_init();
}
