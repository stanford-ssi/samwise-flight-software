#include "filesys_test.h"

void main(void)
{
    test_harness_run("Filesys", filesys_tests, filesys_tests_len,
                     filesys_test_setup);
}
