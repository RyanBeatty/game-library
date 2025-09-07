#pragma once

#include <assert.h>
#include <dirent.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LCORE static __attribute__((unused))

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#define U32_MAX UINT_MAX

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef f32 rad;
typedef f32 deg;
typedef double f64;
#define F32_MAX FLT_MAX
// Min value a float can differ by.
#define F32_EPSILON FLT_EPSILON

typedef ptrdiff_t isize;
typedef size_t usize;
typedef uintptr_t uptr;
typedef unsigned char byte;

#define GB(x) ((u64)1024 * 1024 * 1024 * (x))

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

////////////////////////////////////////////////////////////
// S:Memory

byte* PlatformAllocate(isize cap);

typedef struct {
    // TODO: I might want to include a stable pointer to the start if I ever want to free() arenas.
    byte* beg;
    byte* end;
} Arena;

void* ArenaAlloc(Arena* a, isize count, isize size, isize alignment);
Arena NewArena(byte* buffer, isize cap);
#define TempArena(a) ((a))

// :a = arena, :count = num elems to alloc, :type = type to allocate
#define New(a, count, type) ((type*)ArenaAlloc((a), (count), sizeof(type), _Alignof(type)))

// Init array static. Inits a static buffer.
#define InitArrS(s, d, c) \
    do {                  \
        (s)->data = (d);  \
        (s)->len = 0;     \
        (s)->cap = (c);   \
    } while (0)

// Assumes that :s: is a struct with members
// struct {
//  T *data;
//  isize len;
//  isize cap;
// }
// Will crash if you try to allocate more than cap. PushS = Push Static
#define PushS(s) (assert((s)->len < (s)->cap), (s)->data + (s)->len++)
#define PushArrS(s, v) (assert((s)->len < (s)->cap), (s)->data[(s)->len++] = (v))
#define ArrPush(s, v) (PushArrS(s, v))
#define ArrGet(a, idx) ((a)->data[(idx)])
// Get reference
#define ArrGetR(a, idx) (&ArrGet(a, idx))
#define ArrReserve(a) (PushS(s))
#define ArrPop(a) (assert((a)->len != 0), (a)->data[(a)->len--])

typedef struct {
    byte* data;
    isize len;
    isize cap;
} ByteBuffer;

typedef struct {
    char* data;
    isize len;
} Str8;

#define Str8(s) ((Str8){.data = (s), .len = (ArraySizeOf((s)) - 1)})
