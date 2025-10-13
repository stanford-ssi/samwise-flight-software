#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Mock absolute_time_t (if not already defined)
#ifndef ABSOLUTE_TIME_T_DEFINED
typedef uint64_t absolute_time_t;
#define ABSOLUTE_TIME_T_DEFINED
#endif

// Mock I2C types and functions
typedef struct
{
    uint8_t dummy;
} i2c_inst_t;

#define i2c0 ((i2c_inst_t *)0)
#define i2c1 ((i2c_inst_t *)1)

// Macro to get i2c instance
#define I2C_INSTANCE(n) ((n) == 0 ? i2c0 : i2c1)

static inline void i2c_init(i2c_inst_t *i2c, uint32_t baudrate)
{
}
static inline void i2c_deinit(i2c_inst_t *i2c)
{
}
static inline int i2c_write_blocking(i2c_inst_t *i2c, uint8_t addr,
                                     const uint8_t *src, size_t len,
                                     bool nostop)
{
    return len;
}
static inline int i2c_read_blocking(i2c_inst_t *i2c, uint8_t addr, uint8_t *dst,
                                    size_t len, bool nostop)
{
    return len;
}
static inline int i2c_read_blocking_until(i2c_inst_t *i2c, uint8_t addr,
                                          uint8_t *dst, size_t len, bool nostop,
                                          absolute_time_t until)
{
    return len;
}
