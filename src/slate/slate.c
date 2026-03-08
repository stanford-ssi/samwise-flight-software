#include "slate.h"

slate_t create_slate(void)
{
    slate_t new_slate = {0};
    new_slate.filesys_buffer = NULL; // Ensure buffer starts as NULL; will be
                                     // allocated in filesys_initialize
    return new_slate;
}

/**
 * Statically allocate the slate.
 */
slate_t slate;
