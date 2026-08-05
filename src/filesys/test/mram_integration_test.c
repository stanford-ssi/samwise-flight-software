#include "mram_test.h"

void main(void)
{
    mram_init();

    test_harness_run("MRAM", mram_tests, mram_tests_len, mram_test_setup);
}
