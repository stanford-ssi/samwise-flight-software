
#include "slate.h"

#define MAX_BURN_DURATION_MS 5000 // Maximum burn duration in milliseconds
#define BURN_DURATION_MS                                                       \
    800 // Duration for each burn wire activation in milliseconds
#define MAX_BURN_WIRE_ATTEMPTS 5 // Maximum number of burn wire attempts

void burn_wire_init(slate_t *slate);
void burn_wire_activate(slate_t *slate, uint32_t burn_ms, bool activate_A,
                        bool activate_B);
