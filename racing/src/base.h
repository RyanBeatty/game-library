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

#define Hz(x) (1.0f / (x))

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
#define ArrGetPtr(a, idx) (ArrGetR(a, idx))
#define ArrReserve(a) (PushS(s))
#define ArrPop(a) (assert((a)->len != 0), (a)->data[(a)->len--])
#define ArrCap(a) ((a)->cap)
#define ArrLen(a) ((a)->len)
#define ArrClear(a) ((a)->len = 0)

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

////////////////////////////////////////////////////////////
// S:Math

static f32 TOL = 0.001;

#define f32abs fabs
#define f32min fminf
#define f32max fmaxf
#define f32sqrt sqrtf
#define f32mod fmodf
#define fhalf(x) ((x) * 0.5f)
#define f32round(a) (roundf((a)))
#define f32sin sinf
#define f32cos cosf
#define f32tan tanf
#define f32asin asinf
#define f32acos acosf
#define f32atan atanf
#define f32floor floorf
#define f32ceil ceilf
#define f32pow powf
#define f32eq(x, y) (f32abs((x) - (y)) < (TOL))
#define f32isnan(x) (isnan(x))
// Prefer this lerp forumulation as at t=0 we will have exactly start and at t=1 we will have exactly end.
// The other formulation is subject to floating point precision issues where we might not get exactly
// end at t=1.
// Also prefer a function to avoid macro double invocation of the t parameter. If t is lets say a call to a rand
// number then it will cause us to get incorrect results.
LCORE f32 f32lerp(f32 s, f32 e, f32 t) { return ((1.0f - t) * s) + (t * e); }

// #define f32lerp(s, e, t) ((s) + (((e) - (s)) * (t)))
#define f32rand01() ((f32)rand() / (f32)RAND_MAX)
#define f32randab(a, b) (f32lerp((a), (b), (f32rand01())))
#define f32clamp(x, min, max) (f32min(f32max((x), (min)), (max)))
#define f32clamp01(x) (f32clamp((x), 0.0f, 1.0f))
// Returns the percent t is between start and end so range s..e to 0..1
// Note only returns positive pct. To invert direction do 1.0f - res
#define f32lerp_pct(s, e, t) (f32clamp01(((t) - (s)) / ((e) - (s))))

typedef struct {
    union {
        struct {
            f32 x;
            f32 y;
        };

        f32 f[2];
    };
} Vec2;

#define Vec2Lit(a, b) ((Vec2){.x = (a), .y = (b)})

static Vec2 Vec2Zero() {
    static Vec2 v = Vec2Lit(0.0f, 0.0f);
    return v;
}
static Vec2 Vec2Negate(Vec2 v) { return Vec2Lit(-v.x, -v.y); }
static Vec2 Vec2Add(Vec2 v1, Vec2 v2) { return Vec2Lit(v1.x + v2.x, v1.y + v2.y); }
static Vec2 Vec2Sub(Vec2 v1, Vec2 v2) { return Vec2Lit(v1.x - v2.x, v1.y - v2.y); }
static Vec2 Vec2Scale(Vec2 v, f32 s) { return Vec2Lit(v.x * s, v.y * s); }
static Vec2 Vec2Normalize(Vec2 v) {
    Vec2 result = Vec2Lit(v.x, v.y);
    f32 len = f32sqrt((v.x * v.x) + (v.y * v.y));
    if (len > 0.0f) {
        f32 inv_len = 1.0f / len;
        result.x *= inv_len;
        result.y *= inv_len;
    }
    return result;
}
static Vec2 Vec2Clamp(Vec2 v1, Vec2 min, Vec2 max) {
    return Vec2Lit(f32max(min.x, f32min(v1.x, max.x)), f32max(min.y, f32min(v1.y, max.y)));
}
static Vec2 Vec2Cross(Vec2 v) { return Vec2Lit(v.y, -v.x); }
static f32 Vec2Len(Vec2 v) { return f32sqrt((v.x * v.x) + (v.y * v.y)); }
static f32 Vec2Dot(Vec2 v1, Vec2 v2) { return (v1.x * v2.x) + (v1.y * v2.y); }
static bool Vec2Eq(Vec2 a, Vec2 b) { return ((f32abs(a.x - b.x) < TOL) && (f32abs(a.y - b.y) < TOL)); }
static f32 Vec2Angle(Vec2 a, Vec2 b) { return f32acos(Vec2Dot(a, b) / (Vec2Len(a) * Vec2Len(b))); }

typedef struct {
    union {
        struct {
            f32 x;
            f32 y;
            f32 z;
        };

        struct {
            f32 yaw;    // rotation about up (y) axis.
            f32 pitch;  // rotation about (x) axis
            f32 roll;   // rotation about z axis.
        };

        f32 f[3];
    };
} Vec3;

#define Vec3Lit(a, b, c) ((Vec3){.x = (a), .y = (b), .z = (c)})
#define Vec3LitAll(a) (Vec3Lit(a, a, a))
#define Vec3ToVec2(v) (Vec2Lit((v).x, (v).y))
#define Vec2ToVec3(v) (Vec3Lit((v).x, (v).y, 0.0f))
#define Vec3ToRVector3(v) ((Vector3){.x = (v).x, .y = (v).y, .z = (v.z)})

static Vec3 Vec3Zero() {
    static Vec3 v = Vec3Lit(0.0f, 0.0f, 0.0f);
    return v;
}

LCORE Vec3 Vec3Add(Vec3 v1, Vec3 v2) { return Vec3Lit(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z); }
// Remember end - start gives you start -> end.
// formula v1 - v2
LCORE Vec3 Vec3Sub(Vec3 v1, Vec3 v2) { return Vec3Lit(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z); }
LCORE Vec3 Vec3Scale(Vec3 v, f32 n) { return Vec3Lit(v.x * n, v.y * n, v.z * n); }
// Inverts a vector.
LCORE Vec3 Vec3Negate(Vec3 v) { return Vec3Lit(-1.0f * v.x, -1.0f * v.y, -1.0f * v.z); }
LCORE Vec3 Vec3Normalize(Vec3 v) {
    f32 len = f32sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
    if (len > 0.0f) {
        f32 inv_len = 1.0f / len;
        v.x *= inv_len;
        v.y *= inv_len;
        v.z *= inv_len;
    }
    return v;
}

// in right handed notation. a is pointer, b is index. results points towards thumb.
LCORE Vec3 Vec3Cross(Vec3 a, Vec3 b) {
    return Vec3Lit((a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x));
}
LCORE Vec3 Vec3Lerp(Vec3 start, Vec3 end, f32 t) {
    return Vec3Lit(f32lerp(start.x, end.x, t), f32lerp(start.y, end.y, t), f32lerp(start.z, end.z, t));
}

LCORE f32 Vec3Len(Vec3 v) { return f32sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z)); }
LCORE f32 Vec3Dot(Vec3 v1, Vec3 v2) { return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z); }
// Returns the angle between a and b in radians. Assumes a and b are normalized.
LCORE f32 Vec3Angle(Vec3 a, Vec3 b) {
    // acos is only defined on [-1, 1], otherwise it will yield nan. Due to floating point precision,
    // the dot between two very similar vecotrs might be slighlty over/under 1 (actually observed in game), so
    // clamp this to be between the valid range.
    f32 dot = Vec3Dot(a, b) / (Vec3Len(a) * Vec3Len(b));
    return f32acos(f32clamp(dot, -1.0f, 1.0f));
}
LCORE bool Vec3Eq(Vec3 a, Vec3 b) {
    return ((f32abs(a.x - b.x) < TOL) && (f32abs(a.y - b.y) < TOL) && (f32abs(a.z - b.z) < TOL));
}
LCORE bool Vec3EqPro(Vec3 a, Vec3 b, f32 tol) {
    return ((f32abs(a.x - b.x) < tol) && (f32abs(a.y - b.y) < tol) && (f32abs(a.z - b.z) < tol));
}
LCORE Vec3 Vec3RotateAxis(Vec3 v, Vec3 axis, rad rot) {
    axis = Vec3Normalize(axis);
    f32 cosr = f32cos(rot);
    f32 sinr = f32sin(rot);
    Vec3 vcos = Vec3Scale(v, cosr);
    Vec3 axv = Vec3Scale(Vec3Cross(axis, v), sinr);
    Vec3 kk = Vec3Scale(axis, Vec3Dot(axis, v) * (1 - cosr));
    return Vec3Add(vcos, Vec3Add(axv, kk));
}

LCORE Vec3 Vec3Project(Vec3 v, Vec3 axis) { return Vec3Scale(axis, Vec3Dot(v, axis)); }

LCORE Vec3 RandPointInRing(f32 r_inner, f32 r_outer) {
    return Vec3Scale(
        Vec3RotateAxis(Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f), Deg2Rad(f32randab(0.0f, 360.0f))),
        f32randab(r_inner, r_outer));
}

LCORE Vec3 RandPointInCircle(f32 r) { return RandPointInRing(0.0f, r); }

LCORE Vec3 RandPointOnCircle(f32 r) {
    return Vec3Scale(
        Vec3RotateAxis(Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f), Deg2Rad(f32randab(0.0f, 360.0f))), r);
}

// Col major order. Reminder this means that the notation is written in col major order (basis vectors run
// down columns), but the columns are *STORED* in row major order.
// For matrix mult notation wise we will compose from right to left. i.e. X = CBAv.
typedef struct {
    union {
        f32 m[4 * 4];

        // col major order.
        struct {
            f32 m00;
            f32 m10;
            f32 m20;
            f32 m30;
            f32 m01;
            f32 m11;
            f32 m21;
            f32 m31;
            f32 m02;
            f32 m12;
            f32 m22;
            f32 m32;
            f32 m03;
            f32 m13;
            f32 m23;
            f32 m33;
        } mm;

        // a b c d are the basis vec3s
        struct {
            Vec3 a;
            f32 x;
            Vec3 b;
            f32 y;
            Vec3 c;
            f32 z;
            Vec3 d;
            f32 w;
        } v;
    };
} Mat4;

// Take matrix notation as viewed on textbook page and convert to mat4
#define Mat4Lit(m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33)                     \
    ((Mat4){.m = {(m00), (m10), (m20), (m30), (m01), (m11), (m21), (m31), (m02), (m12), (m22), (m32), (m03), (m13), \
                  (m23), (m33)}})

#define Mat4VLit(e0, e1, e2, e3) \
    ((Mat4){.v.a = (e0), .v.x = 0.0f, .v.b = (e1), .v.y = 0.0f, .v.c = (e2), .v.z = 0.0f, .v.d = (e3), .v.w = 1.0f})

// Print in col maj order.
LCORE void Mat4Print(Mat4 m) {
    printf(
        "mat4\n"
        "%f %f %f %f\n"
        "%f %f %f %f\n"
        "%f %f %f %f\n"
        "%f %f %f %f\n",
        m.mm.m00, m.mm.m01, m.mm.m02, m.mm.m03,  //
        m.mm.m10, m.mm.m11, m.mm.m12, m.mm.m13,  //
        m.mm.m20, m.mm.m21, m.mm.m22, m.mm.m23,  //
        m.mm.m30, m.mm.m31, m.mm.m32, m.mm.m33);
}

static Mat4 Mat4Identity() {
    static Mat4 m = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f,  //
                            0.0f, 1.0f, 0.0f, 0.0f,  //
                            0.0f, 0.0f, 1.0f, 0.0f,  //
                            0.0f, 0.0f, 0.0f, 1.0f);
    return m;
}

LCORE Mat4 Mat4Scale(f32 n) {
    // TODO: Should this scale the w component? doing that throws off normal calcs, but thats probably
    // because I'm hackily making the normal matrix at the moment.
    return Mat4Lit(n, 0.0f, 0.0f, 0.0f,  //
                   0.0f, n, 0.0f, 0.0f,  //
                   0.0f, 0.0f, n, 0.0f,  //
                   0.0f, 0.0f, 0.0f, 1.0f);
}

LCORE Mat4 Mat4ScaleV(Vec3 v) {
    return Mat4Lit(v.x, 0.0f, 0.0f, 0.0f,  //
                   0.0f, v.y, 0.0f, 0.0f,  //
                   0.0f, 0.0f, v.z, 0.0f,  //
                   0.0f, 0.0f, 0.0f, 1.0f);
}

LCORE Mat4 Mat4ScaleXYZ(f32 x, f32 y, f32 z) {
    return Mat4Lit(x, 0.0f, 0.0f, 0.0f,  //
                   0.0f, y, 0.0f, 0.0f,  //
                   0.0f, 0.0f, z, 0.0f,  //
                   0.0f, 0.0f, 0.0f, 1.0f);
}

LCORE Mat4 Mat4TranslateV(Vec3 v) {
    return Mat4Lit(1.0f, 0.0f, 0.0f, v.x,  //
                   0.0f, 1.0f, 0.0f, v.y,  //
                   0.0f, 0.0f, 1.0f, v.z,  //
                   0.0f, 0.0f, 0.0f, 1.0f  //
    );
}

LCORE Mat4 Mat4Transpose(Mat4 m) {
    return Mat4Lit(m.mm.m00, m.mm.m10, m.mm.m20, m.mm.m30,  //
                   m.mm.m01, m.mm.m11, m.mm.m21, m.mm.m31,  //
                   m.mm.m02, m.mm.m12, m.mm.m22, m.mm.m32,  //
                   m.mm.m03, m.mm.m13, m.mm.m23, m.mm.m33   //
    );
}

LCORE Mat4 Mat4Multiply(Mat4 a, Mat4 b) {
    return Mat4Lit(
        // Row 0.
        (a.mm.m00 * b.mm.m00) + (a.mm.m01 * b.mm.m10) + (a.mm.m02 * b.mm.m20) + (a.mm.m03 * b.mm.m30),  //
        (a.mm.m00 * b.mm.m01) + (a.mm.m01 * b.mm.m11) + (a.mm.m02 * b.mm.m21) + (a.mm.m03 * b.mm.m31),  //
        (a.mm.m00 * b.mm.m02) + (a.mm.m01 * b.mm.m12) + (a.mm.m02 * b.mm.m22) + (a.mm.m03 * b.mm.m32),  //
        (a.mm.m00 * b.mm.m03) + (a.mm.m01 * b.mm.m13) + (a.mm.m02 * b.mm.m23) + (a.mm.m03 * b.mm.m33),  //
        // Row 1.
        (a.mm.m10 * b.mm.m00) + (a.mm.m11 * b.mm.m10) + (a.mm.m12 * b.mm.m20) + (a.mm.m13 * b.mm.m30),  //
        (a.mm.m10 * b.mm.m01) + (a.mm.m11 * b.mm.m11) + (a.mm.m12 * b.mm.m21) + (a.mm.m13 * b.mm.m31),  //
        (a.mm.m10 * b.mm.m02) + (a.mm.m11 * b.mm.m12) + (a.mm.m12 * b.mm.m22) + (a.mm.m13 * b.mm.m32),  //
        (a.mm.m10 * b.mm.m03) + (a.mm.m11 * b.mm.m13) + (a.mm.m12 * b.mm.m23) + (a.mm.m13 * b.mm.m33),  //
        // Row 2.
        (a.mm.m20 * b.mm.m00) + (a.mm.m21 * b.mm.m10) + (a.mm.m22 * b.mm.m20) + (a.mm.m23 * b.mm.m30),  //
        (a.mm.m20 * b.mm.m01) + (a.mm.m21 * b.mm.m11) + (a.mm.m22 * b.mm.m21) + (a.mm.m23 * b.mm.m31),  //
        (a.mm.m20 * b.mm.m02) + (a.mm.m21 * b.mm.m12) + (a.mm.m22 * b.mm.m22) + (a.mm.m23 * b.mm.m32),  //
        (a.mm.m20 * b.mm.m03) + (a.mm.m21 * b.mm.m13) + (a.mm.m22 * b.mm.m23) + (a.mm.m23 * b.mm.m33),  //
        // Row 3.
        (a.mm.m30 * b.mm.m00) + (a.mm.m31 * b.mm.m10) + (a.mm.m32 * b.mm.m20) + (a.mm.m33 * b.mm.m30),  //
        (a.mm.m30 * b.mm.m01) + (a.mm.m31 * b.mm.m11) + (a.mm.m32 * b.mm.m21) + (a.mm.m33 * b.mm.m31),  //
        (a.mm.m30 * b.mm.m02) + (a.mm.m31 * b.mm.m12) + (a.mm.m32 * b.mm.m22) + (a.mm.m33 * b.mm.m32),  //
        (a.mm.m30 * b.mm.m03) + (a.mm.m31 * b.mm.m13) + (a.mm.m32 * b.mm.m23) + (a.mm.m33 * b.mm.m33)   //
    );
}

LCORE Mat4 Mat4RotateX(rad r) {
    return Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f,             //
                   0.0f, f32cos(r), -f32sin(r), 0.0f,  //
                   0.0f, f32sin(r), f32cos(r), 0.0f,   //
                   0.0f, 0.0f, 0.0f, 1.0f);
}

LCORE Mat4 Mat4RotateY(rad r) {
    return Mat4Lit(f32cos(r), 0.0f, f32sin(r), 0.0f,   //
                   0.0f, 1.0f, 0.0f, 0.0f,             //
                   -f32sin(r), 0.0f, f32cos(r), 0.0f,  //
                   0.0f, 0.0f, 0.0f, 1.0f);
}

LCORE Mat4 Mat4RotateZ(rad r) {
    return Mat4Lit(f32cos(r), -f32sin(r), 0.0f, 0.0f,  //
                   f32sin(r), f32cos(r), 0.0f, 0.0f,   //
                   0.0f, 0.0f, 1.0f, 0.0f,             //
                   0.0f, 0.0f, 0.0f, 1.0f);
}

// Construct rotation matrix for the euler angle rotation representation. angles in radians.
// Will rotate from model to upright.
LCORE Mat4 Mat4RotateEuler(Vec3 v) {
    return Mat4Multiply(Mat4RotateY(v.yaw), Mat4Multiply(Mat4RotateX(v.pitch), Mat4RotateZ(v.roll)));
}

LCORE Mat4 Mat4Rotate(rad t, Vec3 axis) {
    axis = Vec3Normalize(axis);

    f32 c = f32cos(t);
    f32 s = f32sin(t);
    f32 d = 1.0f - c;

    f32 x = axis.x * d;
    f32 y = axis.y * d;
    f32 z = axis.z * d;
    f32 axay = x * axis.y;
    f32 axaz = x * axis.z;
    f32 ayaz = y * axis.z;

    return Mat4Lit(c + (x * axis.x), axay - (s * axis.z), axaz + (s * axis.y), 0.0f,  //
                   axay + (s * axis.z), c + (y * axis.y), ayaz - (s * axis.x), 0.0f,  //
                   axaz - (s * axis.y), ayaz + (s * axis.x), c + (z * axis.z), 0.0f,  //
                   0.0f, 0.0f, 0.0f, 1.0f);
}

// Contstruct matrix that reflects vectors around the plane defined by n. n doesn't need to be unit.
// useful for 3d reflections. see FOGB 2.3
LCORE Mat4 Mat4Reflect(Vec3 n) {
    n = Vec3Normalize(n);

    f32 x = n.x * -2.0f;
    f32 y = n.y * -2.0f;
    f32 z = n.z * -2.0f;
    f32 nxny = x * n.y;
    f32 nxnz = x * n.z;
    f32 nynz = y * n.z;
    return Mat4Lit((x * n.x) + 1.0f, nxny, nxnz, 0,  //
                   nxny, (y * n.y) + 1.0f, nynz, 0,  //
                   nxnz, nynz, (z * n.z) + 1.0f, 0,  //
                   0, 0, 0, 1                        //
    );
}

// gamemath.com 10.3.4
LCORE Mat4 Mat4Perspective(rad fov, f32 aspect, f32 near, f32 far) {
    f32 zoom = 1.0f / (f32)f32tan(fov * 0.5f);
    f32 depth = far - near;
    return Mat4Lit(zoom / aspect, 0.0f, 0.0f, 0.0f,                                  //
                   0.0f, zoom, 0.0f, 0.0f,                                           //
                   0.0f, 0.0f, -(far + near) / depth, (-2.0f * far * near) / depth,  //
                   0.0f, 0.0f, -1.0f, 0.0f);
}

// FoGED 6.3.4
// gamemath.com 10.3.4
// maps (left, bottom) -> (-1, -1)
LCORE Mat4 Mat4Ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    f32 w_inv = 1.0f / (right - left);
    // NOTE: FoGED has bottom - top, but I think I want top - bottom so that +y = up.
    f32 h_inv = 1.0f / (top - bottom);
    f32 d_inv = 1.0f / (far - near);
    // // Most online code has -(near + far) * d_inv instead of just -near * d_inv.
    // // results appear maybe the same?
    // return Mat4Lit(2.0f * w_inv, 0.0f, 0.0f, -(right + left) * w_inv,  //
    //                0.0f, 2.0f * h_inv, 0.0f, -(bottom + top) * h_inv,  //
    //                0.0f, 0.0f, d_inv, -near * d_inv,                   //
    //                0.0f, 0.0f, 0.0f, 1.0f);
    // NOTE: gamemath doesn't have anything for xy for translation column. but that won't
    // remap the origin at all. by having the code from FoGED, it will remap origin to be bottom left
    // screen corner.
    return Mat4Lit(2.0f / (right - left), 0.0f, 0.0f, -(right + left) * w_inv,  //
                   0.0f, 2.0f / (top - bottom), 0.0f, -(bottom + top) * h_inv,  //
                   0.0f, 0.0f, -2.0f * d_inv, -(far + near) * d_inv,            //
                   0.0f, 0.0f, 0.0f, 1.0f);
}

// Eye is world space pos of camera. Target is what it is looking at in worldspace.
// Up just needs to be pointing somewhere along the plane between the +z and where you want the +y
// axis to be.
LCORE Mat4 Mat4LookAt(Vec3 eye, Vec3 target, Vec3 up) {
    // We need target -> eye diAserection vec because we assueme +z is towards the viewer.
    Vec3 z = Vec3Normalize(Vec3Sub(eye, target));

    // Get the right camera axis by using the +z and a vector pointint up (but not necessarily
    // the up axis)
    Vec3 x = Vec3Normalize(Vec3Cross(up, z));

    Vec3 y = Vec3Normalize(Vec3Cross(z, x));

    return Mat4Lit(x.x, x.y, x.z, -Vec3Dot(x, eye),  //
                   y.x, y.y, y.z, -Vec3Dot(y, eye),  //
                   z.x, z.y, z.z, -Vec3Dot(z, eye),  //
                   0.0f, 0.0f, 0.0f, 1.0f);
}

// Construct a rotation matrix such that will point along the forward vector (model->upright space).
// Forward is in upright space. Up can be any arbitrary axis in forward space. We will construct the
// true up vector inside. Normally up will be (0, 1, 0).
LCORE Mat4 Mat4PointAt(Vec3 forward) {
    static const Vec3 UP = Vec3Lit(0.0f, 1.0f, 0.0);

    // We assume the forward vector points in the opposite diAserection of the z axis of the coordinate
    // space (i.e. we assume that forward == -z or inverse(forward) == +z of the rotation we need to construct), so
    // invert the z axis here.
    Vec3 z = Vec3Normalize(Vec3Scale(forward, -1.0f));

    // Get the right camera axis by using the +z and a vector pointing up (but not necessarily
    // the up axis)
    Vec3 up = Vec3Lit(0.0f, 1.0f, 0.0);
    Vec3 x = Vec3Normalize(Vec3Cross(UP, z));
    if (Vec3Eq(x, Vec3Zero())) {
        up = Vec3Lit(-1.0f, 0.0f, 0.0f);
        x = Vec3Normalize(Vec3Cross(up, z));
    }
    Assert(!Vec3Eq(x, Vec3Zero()));

    // compute the true up axis.
    Vec3 y = Vec3Normalize(Vec3Cross(z, x));

    Mat4 m = Mat4Identity();
    // Override basis vectors.
    m.v.a = x;
    m.v.b = y;
    m.v.c = z;
    return m;
}

LCORE Mat4 Mat4Invert(Mat4 m) {
    Vec3 s = Vec3Cross(m.v.a, m.v.b);
    Vec3 t = Vec3Cross(m.v.c, m.v.d);
    // TODO: Something about this is giving the wrong sign. not sure. instead assume
    // that bottom row will always be 0 0 0 1, so u = 0 and v = c.
    // Vec3 u = Vec3Sub(Vec3Scale(m.v.a, m.v.y), Vec3Scale(m.v.b, m.v.x));
    // Vec3 v = Vec3Sub(Vec3Scale(m.v.c, m.v.w), Vec3Scale(m.v.d, m.v.z));
    Vec3 u = Vec3Zero();
    Vec3 v = m.v.c;

    f32 detinv = 1.0f / (Vec3Dot(s, v) + Vec3Dot(t, u));

    s = Vec3Scale(s, detinv);
    t = Vec3Scale(t, detinv);
    u = Vec3Scale(u, detinv);
    v = Vec3Scale(v, detinv);

    Vec3 r0 = Vec3Add(Vec3Cross(m.v.b, v), Vec3Scale(t, m.v.y));
    Vec3 r1 = Vec3Sub(Vec3Cross(v, m.v.a), Vec3Scale(t, m.v.x));
    Vec3 r2 = Vec3Add(Vec3Cross(m.v.d, u), Vec3Scale(s, m.v.w));
    Vec3 r3 = Vec3Sub(Vec3Cross(u, m.v.c), Vec3Scale(s, m.v.z));

    Mat4 m2 = Mat4Lit(r0.x, r0.y, r0.z, -1.0f * Vec3Dot(m.v.b, t),  //
                      r1.x, r1.y, r1.z, Vec3Dot(m.v.a, t),          //
                      r2.x, r2.y, r2.z, -1.0f * Vec3Dot(m.v.d, s),  //
                      r3.x, r3.y, r3.z, Vec3Dot(m.v.c, s));
    return m2;
}

LCORE Mat4 Mat4SRT(f32 scale, Mat4 rotate, Vec3 pos) {
    return Mat4Multiply(Mat4TranslateV(pos), Mat4Multiply(rotate, Mat4Scale(scale)));
}

LCORE Mat4 Mat4SVRT(Vec3 scale, Mat4 rotate, Vec3 pos) {
    return Mat4Multiply(Mat4TranslateV(pos), Mat4Multiply(rotate, Mat4ScaleV(scale)));
}

LCORE bool Mat4Eq(Mat4 a, Mat4 b) {
    bool result = Vec3Eq(a.v.a, b.v.a) && Vec3Eq(a.v.b, b.v.b) && Vec3Eq(a.v.c, b.v.c) && Vec3Eq(a.v.d, b.v.d) &&
                  f32eq(a.v.x, b.v.x) && f32eq(a.v.y, b.v.y) && f32eq(a.v.z, b.v.z) && f32eq(a.v.w, b.v.w);
    return result;
}

typedef struct {
    union {
        f32 fs[4];

        struct {
            f32 m00;
            f32 m10;
            f32 m01;
            f32 m11;
        } mm;

        struct {
            Vec2 a;
            Vec2 b;
        } v;
    };
} Mat2;

// Take matrix notation as viewed on textbook page and convert to mat4. This is in row major format.
#define Mat2Lit(m00, m01, m10, m11) ((Mat2){.mm = {(m00), (m10), (m01), (m11)}})

LCORE Mat2 Mat4ToMat2(Mat4 m) { return Mat2Lit(m.mm.m00, m.mm.m01, m.mm.m10, m.mm.m11); }

static Mat2 Mat2Identity() {
    static Mat2 m = Mat2Lit(1.0f, 0.0f,  //
                            0.0f, 1.0f);
    return m;
}

static Mat2 Mat2Multiply(Mat2 b, Mat2 a) {
    return Mat2Lit((b.mm.m00 * a.mm.m00) + (b.mm.m01 * a.mm.m10), (b.mm.m00 * a.mm.m01) + (b.mm.m01 * a.mm.m11),  //
                   (b.mm.m10 * a.mm.m00) + (b.mm.m11 * a.mm.m10), (b.mm.m10 * a.mm.m01) + (b.mm.m11 * a.mm.m11)   //
    );
}

LCORE Vec2 Vec2Transform(Vec2 v, Mat2 m) {
    return Vec2Lit((m.mm.m00 * v.x) + (m.mm.m01 * v.y),  //
                   (m.mm.m10 * v.x) + (m.mm.m11 * v.y));
}

// Used for debug.
typedef struct {
    union {
        struct {
            f32 x;
            f32 y;
            f32 z;
            f32 w;
        };
        struct {
            f32 r;
            f32 g;
            f32 b;
            f32 a;
        };
        f32 f[4];
    };
} Vec4;

#define Vec4Lit(v0, v1, v2, v3) ((Vec4){.x = (v0), .y = (v1), .z = (v2), .w = (v3)})

LCORE Vec4 Vec4Lerp(Vec4 start, Vec4 end, f32 t) {
    // clang-format off
    return Vec4Lit(f32lerp(start.x, end.x, t), f32lerp(start.y, end.y, t), f32lerp(start.z, end.z, t), f32lerp(start.w, end.w, t));
    // clang-format on
}

// The positionless aspect is why we don't include the w component here.
static Vec3 Vec3Transform(Vec3 v, Mat4 m) {
    return Vec3Lit((v.x * m.mm.m00) + (v.y * m.mm.m01) + (v.z * m.mm.m02),  //
                   (v.x * m.mm.m10) + (v.y * m.mm.m11) + (v.z * m.mm.m12),  //
                   (v.x * m.mm.m20) + (v.y * m.mm.m21) + (v.z * m.mm.m22));
}

// Useful if needing to transform with translation a vector.
static Vec3 Vec3WTransform(Vec3 v, Mat4 m) {
    // Include origin transform. useful for things like transforming points from one space
    // to another.
    // TODO: think about if Vec3Transform should just do this.
    return Vec3Lit((v.x * m.mm.m00) + (v.y * m.mm.m01) + (v.z * m.mm.m02) + (1.0f * m.mm.m03),  //
                   (v.x * m.mm.m10) + (v.y * m.mm.m11) + (v.z * m.mm.m12) + (1.0f * m.mm.m13),  //
                   (v.x * m.mm.m20) + (v.y * m.mm.m21) + (v.z * m.mm.m22) + (1.0f * m.mm.m23));
}

// Combine two colors with component wise multiply. Normally these vec4s are colors.
LCORE Vec4 Vec4Moduluate(Vec4 a, Vec4 b) { return Vec4Lit(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }

// Quaternion. assume unit quaternion use throughout code.
typedef struct {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
} Quat;

#define QuatLit(a, b, c, d) ((Quat){.x = (a), .y = (b), .z = (c), .w = (d)})
#define QuatIdentity() (QuatLit(0.0f, 0.0f, 0.0f, 1.0f))

// r in radians.
LCORE Quat QuatFromAxisAngle(Vec3 n, f32 r) {
    n = Vec3Normalize(n);
    f32 hr = 0.5f * r;
    f32 sinhr = f32sin(hr);
    return QuatLit(sinhr * n.x, sinhr * n.y, sinhr * n.z, f32cos(hr));
}

LCORE bool QuatEq(Quat a, Quat b) { return f32eq(a.x, b.x) && f32eq(a.y, b.y) && f32eq(a.z, b.z) && f32eq(a.w, b.w); }
// Magnitude
LCORE f32 QuatLen(Quat q) { return f32sqrt((q.x * q.x) + (q.y * q.y) + (q.z * q.z) + (q.w * q.w)); }
// Conjugate
LCORE Quat QuatConj(Quat q) { return QuatLit(-q.x, -q.y, -q.z, q.w); }

//  rotations compose from right to left (so q1 gets applied first).
// FoGED 2.10
LCORE Quat QuatMult(Quat q2, Quat q1) {
    return (Quat){
        q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,  //
        q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,  //
        q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,  //
        q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z,  //
    };
}

// FoGED 2.11
LCORE Vec3 QuatTransform(Quat q, Vec3 v) {
    Vec3 b = Vec3Lit(q.x, q.y, q.z);
    f32 b2 = b.x * b.x + b.y * b.y + b.z * b.z;
    return Vec3Add(Vec3Add(Vec3Scale(v, q.w * q.w - b2), Vec3Scale(b, 2.0f * Vec3Dot(v, b))),
                   Vec3Scale(Vec3Cross(b, v), 2.0f * q.w));
}

LCORE Mat4 Quat2Mat4(Quat q) {
    f32 x2 = q.x * q.x;
    f32 y2 = q.y * q.y;
    f32 z2 = q.z * q.z;
    f32 xy = q.x * q.y;
    f32 xz = q.x * q.z;
    f32 yz = q.y * q.z;
    f32 wx = q.w * q.x;
    f32 wy = q.w * q.y;
    f32 wz = q.w * q.z;

    return Mat4Lit(1.0f - 2.0f * (y2 + z2), 2.0f * (xy - wz), 2.0f * (xz + wy), 0.0f,  //
                                                                                       /////////////////////////////////
                   2.0f * (xy + wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz - wx), 0.0f,  //
                   /////////////////////////////////
                   2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (x2 + y2), 0.0f,  //
                   /////////////////////////////////
                   0.0f, 0.0f, 0.0f, 1.0f  //
    );
}
