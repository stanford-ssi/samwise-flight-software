#include "util/queue.h"

struct circular_queue circular_queue_mk(size_t element_size, size_t max_size, struct sized_buffer buf) {
  struct circular_queue q = {
    .element_size = element_size,
    .max_size = max_size,
    .head = 0,
    .tail = 0,
    .buf = buf
  };
  return q;
}

size_t circular_queue_get_size(struct circular_queue* q) {
  // TODO
  return 0;
}

void circular_queue_push(struct circular_queue* q, void* item) {
  // TODO
}

void circular_queue_pop(struct circular_queue* q, void* item) {
  // TODO
}

void circular_queue_peek(struct circular_queue* q, void* item) {
  // TODO
}
