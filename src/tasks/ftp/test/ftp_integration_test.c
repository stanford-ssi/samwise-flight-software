#include "ftp_test.h"

void main(void)
{
    uint16_t exclude[] = {100}; // Cannot run full file test on hardware!
    test_harness_exclude_run("FTP", ftp_tests, ftp_tests_len, ftp_test_setup,
                             exclude, sizeof(exclude) / sizeof(exclude[0]));
}
