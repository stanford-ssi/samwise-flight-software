#pragma once

#include <stdint.h>
#include <stdbool.h>

// Mock queue type
typedef struct {
    uint8_t dummy;
} queue_t;

// Mock queue functions that are no-ops in tests
static inline void queue_init(queue_t *q, unsigned int element_size, unsigned int element_count) {}
static inline bool queue_try_add(queue_t *q, void *data) { return true; }
static inline bool queue_try_remove(queue_t *q, void *data) { return false; }
static inline bool queue_try_peek(queue_t *q, void *data) { return false; }
static inline bool queue_is_empty(queue_t *q) { return true; }
static inline bool queue_is_full(queue_t *q) { return false; }
static inline unsigned int queue_get_level(queue_t *q) { return 0; }
