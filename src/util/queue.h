#pragma once

#include "pico/stdlib.h"
#include "util/sized_buffer.h"

struct circular_queue {
  size_t element_size;
  size_t max_size;
  struct sized_buffer buf;
  size_t head;
  size_t tail;
};

struct circular_queue circular_queue_mk(size_t element_size, size_t max_size, struct sized_buffer buf);
size_t circular_queue_get_size(struct circular_queue* q);
void circular_queue_push(struct circular_queue* q, void* item);
void circular_queue_pop(struct circular_queue* q, void* item);
void circular_queue_peek(struct circular_queue* q, void* item);
