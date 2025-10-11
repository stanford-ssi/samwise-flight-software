#include "error.h"
#include <stdio.h>
#include <stdlib.h>

void fatal_error(char *msg)
{
    fprintf(stderr, "Fatal error occurred: %s\n", msg ? msg : "unknown");
    exit(1);
}
