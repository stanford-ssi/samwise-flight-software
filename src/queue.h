#pragma once

#include "pico/stdlib.h"

struct circular_queue {
  size_t element_size;
  void* buf;
  size_t head;
  size_t tail;
};

void init_circular_queue(struct circular_queue) {

}