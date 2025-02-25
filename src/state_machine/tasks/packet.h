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

/*

Here is where we will define all of the structs that hold arguments for calling certain tasks.

*/
typedef struct 
{

} TASK1_DATA;


typedef struct
{
    bool yes_no;
    uint16_t number;
} TASK2_DATA;
