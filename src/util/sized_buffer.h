#pragma once

struct sized_buffer {
  // Underlying buffer
  void* raw_buff;
  // Size of element in bytes
  size_t element_size;
  // Size of buffer in elements
  size_t buffer_size;
};

#define MK_STATIC_SIZED_BUFFER(name, num_elements, element_type) \
static element_type name_buf[num_elements];\
static struct sized_buffer name = { &name_buf, sizeof(element_type), num_elements };