#pragma once
// some simple bit manipulation functions: helps make code clearer.

#include "macros.h"
#include <inttypes.h>

// set x[bit]=0 (leave the rest unaltered) and return the value
static inline uint32_t bit_clr(uint32_t x, uint32_t bit)
{
    return x & ~(1 << bit);
}

// set x[bit]=1 (leave the rest unaltered) and return the value
static inline uint32_t bit_set(uint32_t x, uint32_t bit)
{
    return x | (1 << bit);
}

static inline uint32_t bit_not(uint32_t x, uint32_t bit)
{
    return x ^ (1 << bit);
}

#define bit_isset bit_is_on
#define bit_get bit_is_on

// is x[bit] == 1?
static inline uint32_t bit_is_on(uint32_t x, uint32_t bit)
{
    return (x >> bit) & 1;
}
static inline uint32_t bit_is_off(uint32_t x, uint32_t bit)
{
    return bit_is_on(x, bit) == 0;
}

// return a mask with the <n> low bits set to 1.
//  error if nbits > 32.  ok if nbits = 0.
//
// we use this routine b/c uint32_t shift by a variable greater than
// bit-width gives unexpected results.
// eg. gcc on x86:
//  n = 32;
//  ~0 >> n == ~0
static inline uint32_t bits_mask(uint32_t nbits)
{
    // all bits on.
    if (nbits == 32)
        return ~0;
    return (1 << nbits) - 1;
}

// extract bits [lb:ub]  (inclusive)
static inline uint32_t bits_get(uint32_t x, uint32_t lb, uint32_t ub)
{
    return (x >> lb) & bits_mask(ub - lb + 1);
}

// set bits[off:n]=0, leave the rest unchanged.
static inline uint32_t bits_clr(uint32_t x, uint32_t lb, uint32_t ub)
{
    uint32_t mask = bits_mask(ub - lb + 1);

    // XXX: check that gcc handles shift by more bit-width as expected.
    return x & ~(mask << lb);
}

// set bits [lb:ub] to v (inclusive).  v must fit within the specified width.
static inline uint32_t bits_set(uint32_t x, uint32_t lb, uint32_t ub,
                                uint32_t v)
{
    uint32_t n = ub - lb + 1;
    return bits_clr(x, lb, ub) | (v << lb);
}

// bits [lb:ub] == <val?
static inline uint32_t bits_eq(uint32_t x, uint32_t lb, uint32_t ub,
                               uint32_t val)
{
    return bits_get(x, lb, ub) == val;
}

static inline uint32_t bit_count(uint32_t x)
{
    uint32_t cnt = 0;
    for (uint32_t i = 0; i < 32; i++)
        if (bit_is_on(x, i))
            cnt++;
    return cnt;
}

static inline uint32_t bits_union(uint32_t x, uint32_t y)
{
    return x | y;
}

static inline uint32_t bits_intersect(uint32_t x, uint32_t y)
{
    return x & y;
}
static inline uint32_t bits_not(uint32_t x)
{
    return ~x;
}

// forall x in A and not in B
static inline uint32_t bits_diff(uint32_t A, uint32_t B)
{
    return bits_intersect(A, bits_not(B));
}
