#pragma once

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

#define F32_MAX FLT_MAX
// Min value a float can differ by.
#define F32_EPSILON FLT_EPSILON

typedef f32 rad;
typedef f32 deg;

#define LPI ((f32)3.1415926535)
#define LTAU ((f32)6.2831853071)
#define LPHI ((f32)1.6180339887)  // golden ratio

#define Deg2Rad(d) ((d) * LTAU / 360.0f)
#define Rad2Deg(r) ((r) * 360.0f / LTAU)
#define RGB01(v) ((v) / 255.0f)

#define Assert(expr)        \
    {                       \
        if (!(expr)) {      \
            breakpoint();   \
            assert((expr)); \
        }                   \
    }

void __attribute__((noinline)) breakpoint();

#define ArraySizeOf(x) (sizeof(x) / sizeof(x[0]))

#define TestFlag(flags, flag) ((flags) & (flag))
#define FlagOn(flags, flag) ((flags) |= flag)
#define FlagOff(flags, flag) ((flags) &= ~(flag))
