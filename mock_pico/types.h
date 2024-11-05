#ifndef MOCK_PICO_TYPES_H
#define MOCK_PICO_TYPES_H

#include <stdbool.h>
#include <sys/types.h>

// Add any basic types or stubs needed for your code here
// typedef unsigned int uint;
// typedef unsigned char uint8_t;
// typedef unsigned long long uint64_t;
// typedef unsigned int uint32_t;
// typedef unsigned long size_t;
// Mock absolute_time_t type to match pico-sdk
#if PICO_OPAQUE_ABSOLUTE_TIME_T
typedef struct {
    uint64_t _private_us_since_boot;
} absolute_time_t;
#else
typedef uint64_t absolute_time_t;
#endif



// Define other Pico types as needed for your code

#endif // MOCK_PICO_TYPES_H
