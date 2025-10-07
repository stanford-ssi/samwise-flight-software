#include <stdio.h>
#include <stdlib.h>
#include "error.h"

void fatal_error()
{
    fprintf(stderr, "Fatal error occurred\n");
    exit(1);
}
