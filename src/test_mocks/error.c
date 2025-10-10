#include "error.h"
#include <stdio.h>
#include <stdlib.h>

void fatal_error()
{
    fprintf(stderr, "Fatal error occurred\n");
    exit(1);
}
