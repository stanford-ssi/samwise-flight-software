#pragma once

#include <stddef.h>

/* ── Test function signature ──────────────────────────────────────── */

typedef void (*hw_test_fn)(void);

typedef struct
{
    const char *name;
    hw_test_fn run;
} hw_test_entry_t;
