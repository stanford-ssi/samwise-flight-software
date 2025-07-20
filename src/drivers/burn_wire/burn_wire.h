
#include "slate.h"

#define MAX_BURN_DURATION_MS 5000 // Maximum burn duration in milliseconds

void burn_wire_init(slate_t *slate);
void burn_wire_activate(slate_t *slate, uint32_t burn_ms, bool activate_A,
                        bool activate_B);