
#include "slate.h"

#define MAX_BURN_DURATION_MS 500 // Maximum burn duration in milliseconds

void burn_wire_init(slate_t *slate);
void burn_wire_activate(slate_t *slate, uint32_t burn_ms, uint32_t pwm_level,
                        bool activate_A, bool activate_B);