#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "typedefs.h"

void fatal_error(char *msg);

static inline void queue_init(queue_t *q, unsigned int element_size,
                              unsigned int element_count)
{
    q->element_size = element_size;
    q->element_count = element_count;
    q->head = 0;
    q->tail = 0;
    q->level = 0;
    ASSERT(element_size > 0 && element_count > 0 &&
           element_size * element_count < UINT32_MAX);
    q->data = malloc(element_size * element_count);
    ASSERT(q->data != NULL);
}

static inline bool queue_try_add(queue_t *q, void *data)
{
    if (q->level >= q->element_count)
        return false;

    ASSERT(q->element_count > 0 && q->data != NULL);
    memcpy(q->data + q->tail * q->element_size, data, q->element_size);
    q->tail = (q->tail + 1) % q->element_count;
    q->level++;
    return true;
}

static inline bool queue_try_remove(queue_t *q, void *data)
{
    if (q->level == 0)
        return false;

    ASSERT(q->element_count > 0 && q->data != NULL);
    memcpy(data, q->data + q->head * q->element_size, q->element_size);
    q->head = (q->head + 1) % q->element_count;
    q->level--;
    return true;
}

static inline bool queue_try_peek(queue_t *q, void *data)
{
    if (q->level == 0)
        return false;

    ASSERT(q->element_count > 0 && q->data != NULL);
    memcpy(data, q->data + q->head * q->element_size, q->element_size);
    return true;
}

static inline bool queue_is_empty(queue_t *q)
{
    return q->level == 0;
}

static inline bool queue_is_full(queue_t *q)
{
    return q->level == q->element_count;
}

static inline unsigned int queue_get_level(queue_t *q)
{
    return q->level;
}

static inline void queue_free(queue_t *q)
{
    free(q->data);
    q->data = NULL;
    q->level = 0;
    q->head = 0;
    q->tail = 0;
}
