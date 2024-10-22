#pragma once

#include "util/sized_buffer.h"

struct pool {
  struct sized_buffer buf;
  struct queue free;
};