#include "slate.h"

int clear_and_init_slate(slate_t *slate)
{
    memset(slate, 0, sizeof(slate_t)); // Clear all fields to default values (0,
                                       // false, NULL, etc.)

    // Allocate the buffer on the heap since it's too large for the stack
    slate->filesys_buffer = malloc(FILESYS_BUFFER_SIZE);

    if (slate->filesys_buffer == NULL)
    {
        LOG_ERROR("[slate] Failed to allocate filesys_buffer! This is a "
                  "critical error!");
        return -1; // Indicate failure
    }

    return 0;
}

void free_slate(slate_t *slate)
{
    // Free the filesys buffer if it was allocated - note free(NULL) is a no-op
    // as per C specification, so this is safe even if allocation failed.
    free(slate->filesys_buffer);
    slate->filesys_buffer = NULL;
}

/**
 * Will be initialized at startup.
 */
slate_t slate;
