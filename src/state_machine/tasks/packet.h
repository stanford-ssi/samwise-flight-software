#include "slate.h"

typedef struct
{
    uint8_t src;
    uint8_t dst;
    uint8_t flags;
    uint8_t seq;
    uint8_t len; // this should be the length of the packet structure being sent over
    uint8_t data[252];
} packet_t;