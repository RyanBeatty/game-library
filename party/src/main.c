#ifndef UNITY_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

////////////////////////////////////////////////////////////
// Core types.

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

typedef f32 rad;
typedef f32 deg;

#define LPI ((f32)3.1415926535)
#define LTAU ((f32)6.2831853071)

#define Deg2Rad(d) ((d) * LTAU / 360.0f)
#define Rad2Deg(r) ((r) * 360.0f / LTAU)
#define RGB01(v) ((v) / 255.0f)

////////////////////////////////////////////////////////////
// S:Math

static f32 TOL = 0.001;

#define f32abs fabs
#define f32min fminf
#define f32max fmaxf
#define f32sqrt sqrtf
#define fhalf(x) ((x) * 0.5f)
#define f32sin sin
#define f32cos cos
#define f32tan tan
#define f32asin asin
#define f32acos acos
#define f32atan atan
#define f32eq(x, y) (f32abs((x) - (y)) < (TOL))

static f32 f32clamp(f32 v, f32 min, f32 max) { return f32min(f32max(v, min), max); }

typedef struct Vec2 {
    f32 x;
    f32 y;
} Vec2;

#define Vec2Lit(a, b) ((Vec2){.x = (a), .y = (b)})
#define Vec2ToRVector2(v) ((Vector2){.x = (v).x, .y = (v).y})

static Vec2 Vec2Zero() {
    static Vec2 v = Vec2Lit(0.0f, 0.0f);
    return v;
}

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

typedef struct Vec3 {
    union {
        struct {
            f32 x;
            f32 y;
            f32 z;
        };

        struct {
            f32 yaw;
            f32 pitch;
            f32 roll;
        };

        f32 f[3];
    };
} Vec3;

#define Vec3Lit(a, b, c) ((Vec3){.x = (a), .y = (b), .z = (c)})
#define Vec3ToVec2(v) (Vec2Lit((v).x, (v).y))
#define Vec2ToVec3(v) (Vec3Lit((v).x, (v).y, 0.0f))
#define Vec3ToRVector3(v) ((Vector3){.x = (v).x, .y = (v).y, .z = (v.z)})

static Vec3 Vec3Zero() {
    static Vec3 v = Vec3Lit(0.0f, 0.0f, 0.0f);
    return v;
}

static Vec3 Vec3Add(Vec3 v1, Vec3 v2) { return Vec3Lit(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z); }
// Remember end - start gives you start -> end.
static Vec3 Vec3Sub(Vec3 v1, Vec3 v2) { return Vec3Lit(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z); }
static Vec3 Vec3Scale(Vec3 v, f32 n) { return Vec3Lit(v.x * n, v.y * n, v.z * n); }
static Vec3 Vec3Normalize(Vec3 v) {
    f32 len = f32sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
    if (len > 0.0f) {
        f32 inv_len = 1.0f / len;
        v.x *= inv_len;
        v.y *= inv_len;
        v.z *= inv_len;
    }
    return v;
}

// in right handed notation. a is index, b is pointer.
static Vec3 Vec3Cross(Vec3 a, Vec3 b) {
    return Vec3Lit((a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x));
}
static f32 Vec3Len(Vec3 v) { return f32sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z)); }
static f32 Vec3Dot(Vec3 v1, Vec3 v2) { return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z); }
static f32 Vec3Angle(Vec3 a, Vec3 b) { return f32acos(Vec3Dot(a, b) / (Vec3Len(a) * Vec3Len(b))); }
static bool Vec3Eq(Vec3 a, Vec3 b) {
    return ((f32abs(a.x - b.x) < TOL) && (f32abs(a.y - b.y) < TOL) && (f32abs(a.z - b.z) < TOL));
}
static bool Vec3EqPro(Vec3 a, Vec3 b, f32 tol) {
    return ((f32abs(a.x - b.x) < tol) && (f32abs(a.y - b.y) < tol) && (f32abs(a.z - b.z) < tol));
}
static Vec3 Vec3RotateAxis(Vec3 v, Vec3 axis, rad rot) {
    axis = Vec3Normalize(axis);
    f32 cosr = f32cos(rot);
    f32 sinr = f32sin(rot);
    Vec3 vcos = Vec3Scale(v, cosr);
    Vec3 axv = Vec3Scale(Vec3Cross(axis, v), sinr);
    Vec3 kk = Vec3Scale(axis, Vec3Dot(axis, v) * (1 - cosr));
    return Vec3Add(vcos, Vec3Add(axv, kk));
}

// Col major order. Reminder this means that the notation is written in col major order (basis vectors run
// down columns), but the columns are *STORED* in row major order.
// For matrix mult notation wise we will compose from right to left. i.e. X = CBAv.
typedef struct Mat4 {
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

#define Mat4ToRMatrix(m)                                                                                      \
    ((Matrix){(m).mm.m00, (m).mm.m10, (m).mm.m20, (m).mm.m30, (m).mm.m01, (m).mm.m11, (m).mm.m21, (m).mm.m31, \
              (m).mm.m02, (m).mm.m12, (m).mm.m22, (m).mm.m32, (m).mm.m03, (m).mm.m13, (m).mm.m23, (m).mm.m33})

static Mat4 MatrixToMat4(Matrix mx) {
    return Mat4Lit(mx.m0, mx.m1, mx.m2, mx.m3,    //
                   mx.m4, mx.m5, mx.m6, mx.m7,    //
                   mx.m8, mx.m9, mx.m10, mx.m11,  //
                   mx.m12, mx.m13, mx.m14, mx.m15);
}

// Print in col maj order.
static void Mat4Print(Mat4 m) {
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

static Mat4 Mat4Scale(f32 n) {
    // TODO: Should this scale the w component? doing that throws off normal calcs, but thats probably
    // because I'm hackily making the normal matrix at the moment.
    return Mat4Lit(n, 0.0f, 0.0f, 0.0f,  //
                   0.0f, n, 0.0f, 0.0f,  //
                   0.0f, 0.0f, n, 0.0f,  //
                   0.0f, 0.0f, 0.0f, 1.0f);
}

static Mat4 Mat4ScaleXYZ(f32 x, f32 y, f32 z) {
    return Mat4Lit(x, 0.0f, 0.0f, 0.0f,  //
                   0.0f, y, 0.0f, 0.0f,  //
                   0.0f, 0.0f, z, 0.0f,  //
                   0.0f, 0.0f, 0.0f, 1.0f);
}

static Mat4 Mat4TranslateV(Vec3 v) {
    return Mat4Lit(1.0f, 0.0f, 0.0f, v.x,  //
                   0.0f, 1.0f, 0.0f, v.y,  //
                   0.0f, 0.0f, 1.0f, v.z,  //
                   0.0f, 0.0f, 0.0f, 1.0f  //
    );
}

static Mat4 Mat4Transpose(Mat4 m) {
    return Mat4Lit(m.mm.m00, m.mm.m10, m.mm.m20, m.mm.m30,  //
                   m.mm.m01, m.mm.m11, m.mm.m21, m.mm.m31,  //
                   m.mm.m02, m.mm.m12, m.mm.m22, m.mm.m32,  //
                   m.mm.m03, m.mm.m13, m.mm.m23, m.mm.m33   //
    );
}

static Mat4 Mat4Multiply(Mat4 a, Mat4 b) {
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

static Mat4 Mat4RotateX(rad r) {
    return Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f,             //
                   0.0f, f32cos(r), -f32sin(r), 0.0f,  //
                   0.0f, f32sin(r), f32cos(r), 0.0f,   //
                   0.0f, 0.0f, 0.0f, 1.0f);
}

static Mat4 Mat4RotateY(rad r) {
    return Mat4Lit(f32cos(r), 0.0f, f32sin(r), 0.0f,   //
                   0.0f, 1.0f, 0.0f, 0.0f,             //
                   -f32sin(r), 0.0f, f32cos(r), 0.0f,  //
                   0.0f, 0.0f, 0.0f, 1.0f);
}

static Mat4 Mat4RotateZ(rad r) {
    return Mat4Lit(f32cos(r), -f32sin(r), 0.0f, 0.0f,  //
                   f32sin(r), f32cos(r), 0.0f, 0.0f,   //
                   0.0f, 0.0f, 1.0f, 0.0f,             //
                   0.0f, 0.0f, 0.0f, 1.0f);
}

// Construct rotation matrix for the euler angle rotation representation. angles in radians.
// Will rotate from model to upright.
static Mat4 Mat4RotateEuler(Vec3 v) {
    return Mat4Multiply(Mat4RotateY(v.yaw), Mat4Multiply(Mat4RotateX(v.pitch), Mat4RotateZ(v.roll)));
}

static Mat4 Mat4Rotate(rad t, Vec3 axis) {
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

static Mat4 Mat4Perspective(rad fov, f32 aspect, f32 near, f32 far) {
    f32 zoom = 1.0f / (f32)f32tan(fov * 0.5f);
    f32 depth = far - near;
    return Mat4Lit(zoom / aspect, 0.0f, 0.0f, 0.0f,                                  //
                   0.0f, zoom, 0.0f, 0.0f,                                           //
                   0.0f, 0.0f, -(far + near) / depth, (-2.0f * far * near) / depth,  //
                   0.0f, 0.0f, -1.0f, 0.0f);
}

// Eye is world space pos of camera. Target is what it is looking at in worldspace.
// Up just needs to be pointing somewhere along the plane between the +z and where you want the +y
// axis to be.
static Mat4 Mat4LookAt(Vec3 eye, Vec3 target, Vec3 up) {
    // We need target -> eye direction vec because we assueme +z is towards the viewer.
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
static Mat4 Mat4PointAt(Vec3 forward) {
    static const Vec3 UP = Vec3Lit(0.0f, 1.0f, 0.0);

    // We assume the forward vector points in the opposite direction of the z axis of the coordinate
    // space, so invert the z axis here.
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

static Mat4 Mat4Invert(Mat4 m) {
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

static Mat4 Mat4SRT(f32 scale, Mat4 rotate, Vec3 pos) {
    return Mat4Multiply(Mat4TranslateV(pos), Mat4Multiply(rotate, Mat4Scale(scale)));
}

static bool Mat4Eq(Mat4 a, Mat4 b) {
    bool result = Vec3Eq(a.v.a, b.v.a) && Vec3Eq(a.v.b, b.v.b) && Vec3Eq(a.v.c, b.v.c) && Vec3Eq(a.v.d, b.v.d) &&
                  f32eq(a.v.x, b.v.x) && f32eq(a.v.y, b.v.y) && f32eq(a.v.z, b.v.z) && f32eq(a.v.w, b.v.w);
    // #ifdef L_TEST
    if (!result) {
        TraceLog(LOG_INFO, "expected Mat4 b:");
        Mat4Print(b);
        TraceLog(LOG_INFO, "got Mat4 a:");
        Mat4Print(a);
    }
    // #endif
    return result;
}

// Used for debug.
typedef struct Vec4 {
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

////////////////////////////////////////////////////////////
// S:Color

static const Vec4 LRED = Vec4Lit(1.0f, 0.0f, 0.0f, 1.0f);
static const Vec4 LGREEN = Vec4Lit(0.0f, 1.0f, 0.0f, 1.0f);
static const Vec4 LBLUE = Vec4Lit(0.0f, 0.0f, 1.0f, 1.0f);
static const Vec4 LBLACK = Vec4Lit(0.0f, 0.0f, 0.0f, 1.0f);
static const Vec4 LPURPLE = Vec4Lit(1.0f, 0.0f, 1.0f, 1.0f);
static const Vec4 LORANGE = Vec4Lit(1.0f, 0.5f, 0.0f, 1.0f);

////////////////////////////////////////////////////////////
// S:Render Data

typedef struct LShader {
    u32 pid;  // program id.
    u32 vid;
    u32 fid;

    // vert shader.
    u32 vpos_loc;
    u32 vnormal_loc;
    u32 vcolor_loc;
    u32 vmodel_loc;
    u32 vview_loc;
    u32 vproj_loc;
    u32 vucolor_loc;
    u32 vcolor_blend_loc;

    // frag shader.
    u32 flightdirin_loc;
    u32 flight_color_loc;
} LShader;

static void LShaderInit(LShader* shader, const char* vsrc, const char* fsrc) {
    // vertex shader.
    {
        i32 success = 0;
        shader->vid = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(shader->vid, 1, &vsrc, NULL);
        glCompileShader(shader->vid);
        glGetShaderiv(shader->vid, GL_COMPILE_STATUS, &success);
        Assert(success);
    }

    // fragment shader.
    {
        i32 success = 0;
        shader->fid = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(shader->fid, 1, &fsrc, NULL);
        glCompileShader(shader->fid);
        glGetShaderiv(shader->fid, GL_COMPILE_STATUS, &success);
        Assert(success);
    }

    // linking.
    {
        i32 success = 0;
        shader->pid = glCreateProgram();
        glAttachShader(shader->pid, shader->vid);
        glAttachShader(shader->pid, shader->fid);
        glLinkProgram(shader->pid);
        glGetProgramiv(shader->pid, GL_LINK_STATUS, &success);
        Assert(success);
    }
}

static LShader GetShader() {
    static const char* VSRC =
        "#version 330 core\n"
        "layout (location = 0) in vec3 vPos;\n"
        "layout (location = 1) in vec3 vColor;\n"
        "layout (location = 2) in vec3 vNormal;\n"
        "\n"
        "uniform mat4 vModel;\n"
        "uniform mat4 vView;\n"
        "uniform mat4 vProj;\n"
        "\n"
        "uniform vec4 vUColor;\n"
        // TODO: remove this.
        "uniform float vColorBlend;\n"
        "\n"
        "out vec3 fPosEye;\n"
        "out vec4 fColor;\n"
        "out vec3 fNormal;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);\n"
        "   fPosEye = vec3(vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0));\n"
        "   fColor = vUColor;\n"
        // TODO: This is hacky and doesn't work in all cases (e.g. non uniform scale). come up with
        // more robust way to do this. also consider moving this calc to CPU.
        "   fNormal = mat3(transpose(inverse(vView * vModel))) * vNormal;\n"
        "}\0";

    static const char* FSRC =
        "#version 330 core\n"
        "\n"
        "in vec3 fPosEye;\n"
        "in vec4 fColor;\n"
        "in vec3 fNormal;\n"
        "\n"
        "uniform vec4 fLightDirIn;\n"
        "uniform vec4 fLightColor;\n"
        "\n"
        "out vec4 FragColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "   vec4 nLightDirIn = normalize(fLightDirIn);\n"
        "   vec4 norm = vec4(normalize(fNormal).xyz, 0.0f);\n"
        "   vec3 eyePos = vec3(0.0, 0.0, 0.0);\n"
        "\n"
        // Ambient.
        "\n"
        "   float mat_ambient = 0.1;\n"
        "   vec3 ambient = (mat_ambient * fLightColor).xyz;\n"
        // Diffuse.
        "\n"
        "   float diff = max(dot(norm, -nLightDirIn), 0.0);\n"
        "   vec3 diffuse = diff * fLightColor.xyz;\n"
        "\n"
        // specular.
        "\n"
        // controls wide specular highlight is. higher = tighter highlight with faster falloff.
        "   float mat_glossy = 32.0;\n"
        // controls intensity of specular highlight (i.e. how bright/dark).
        // TODO: Some part of this is not woking I think.
        "   float mat_spec = 0.5;\n"
        "   vec4 eyeDirOut = vec4(normalize(eyePos - fPosEye).xyz, 0.0);\n"
        "   vec4 nLightDirOut = normalize(reflect(nLightDirIn, norm));\n"
        "   float spec = pow(max(dot(nLightDirOut, eyeDirOut), 0.0), mat_glossy);\n"
        "   vec3 specular = spec * mat_spec * fLightColor.xyz;\n"
        "\n"
        "   FragColor = vec4(fColor.xyz * (ambient + diffuse + specular), 1.0);\n"
        "   FragColor = fColor;\n"
        "}\0";
    static bool INIT = false;
    static LShader SHADER = {0};

    if (!INIT) {
        INIT = true;
        LShaderInit(&SHADER, VSRC, FSRC);

        // Att lookup.
        {
            SHADER.vpos_loc = glGetAttribLocation(SHADER.pid, "vPos");
            SHADER.vnormal_loc = glGetAttribLocation(SHADER.pid, "vNormal");
            SHADER.vcolor_loc = glGetAttribLocation(SHADER.pid, "vColor");

            SHADER.flightdirin_loc = glGetUniformLocation(SHADER.pid, "fLightDirIn");
            SHADER.flight_color_loc = glGetUniformLocation(SHADER.pid, "fLightColor");
            SHADER.vmodel_loc = glGetUniformLocation(SHADER.pid, "vModel");
            SHADER.vview_loc = glGetUniformLocation(SHADER.pid, "vView");
            SHADER.vproj_loc = glGetUniformLocation(SHADER.pid, "vProj");
            SHADER.vucolor_loc = glGetUniformLocation(SHADER.pid, "vUColor");
            SHADER.vcolor_blend_loc = glGetUniformLocation(SHADER.pid, "vColorBlend");
        }
    }

    return SHADER;
}

// // Shader that will draw light sources without lighting calculations effecting the color.
// static LShader GetUnlitShader() {
//     static const char* VSRC =
//         "#version 330 core\n"
//         "layout (location = 0) in vec3 vPos;\n"
//         "\n"
//         "uniform mat4 vModel;\n"
//         "uniform mat4 vView;\n"
//         "uniform mat4 vProj;\n"
//         "\n"
//         "void main()\n"
//         "{\n"
//         "   gl_Position = vProj * vView * vModel * vec4(vPos, 1.0);\n"
//         "}\0";

//     static const char* FSRC =
//         "#version 330 core\n"
//         "out vec4 FragColor;\n"
//         "void main()\n"
//         "{\n"
//         "    FragColor = vec4(0.0, 0.0, 0.5, 1.0);\n"
//         "}\0";
//     static bool INIT = false;
//     static LShader SHADER = {0};

//     if (!INIT) {
//         INIT = true;
//         LShaderInit(&SHADER, VSRC, FSRC);

//         // Att lookup.
//         {
//             SHADER.vpos_loc = glGetAttribLocation(SHADER.pid, "vPos");
//             SHADER.vmodel_loc = glGetUniformLocation(SHADER.pid, "vModel");
//             SHADER.vview_loc = glGetUniformLocation(SHADER.pid, "vView");
//             SHADER.vproj_loc = glGetUniformLocation(SHADER.pid, "vProj");
//         }
//     }

//     return SHADER;
// }

typedef enum AttLoc {
    LOC_POS_X = 0,
    LOC_POS_Y,
    LOC_POS_Z,

    LOC_COLOR_R,
    LOC_COLOR_G,
    LOC_COLOR_B,

    LOC_NORM_X,
    LOC_NORM_Y,
    LOC_NORM_Z,

    LOC_COUNT
} AttLoc;

typedef struct VAtt {
    f32 atts[LOC_COUNT];
} VAtt;

typedef struct Vertex {
    union {
        f32 f[sizeof(Vec3) * 3];

        struct {
            Vec3 pos;
            Vec3 color;
            Vec3 normal;
        };
    };
} Vertex;

typedef struct Triangle {
    union {
        Vertex vs[3];
        struct {
            Vertex a;
            Vertex b;
            Vertex c;
        };
    };
} Triangle;

#define TriangleLit(n0, n1, n2) ((Triangle){.a = (n0), .b = (n1), .c = (n2)})

#define MAX_VERTICES 256

static u32 VAO = 0;
static u32 VBO = 0;
static u32 VIO = 0;  // Index buffer.

static Vertex VBUFFER[MAX_VERTICES] = {};  // Vertex buffer
static u32 VBUFFER_SIZE = 0;

#define MAX_TRIANGLES 256
static u32 IBUFFER[MAX_TRIANGLES] = {};  // Index buffer
static u32 IBUFFER_SIZE = 0;

typedef struct LMesh {
    u32 ibufferidx;  // Offset into index buffer
    u32 ntriangles;
} LMesh;

#define MAX_MESHES 256
static LMesh MESHES[MAX_MESHES] = {0};
static u32 MESHES_SIZE = 0;

typedef enum MID { M_CUBE = 0, M_QUAD, M_ISO_TRIANGLE, M_PYRAMID, M_2S_ISO_TRIANGLE, M_COUNT } MID;

static LMesh GetMesh(u32 mid) {
    Assert(mid < MAX_MESHES);
    return MESHES[mid];
}

static void AddMesh(Vertex* vs, u32 vsize, f32* ts, u32 tsize) {
    for (u32 i = 0; i < vsize; ++i) {
        VBUFFER[VBUFFER_SIZE + i] = vs[i];
    }
    u32 vbufferidx = VBUFFER_SIZE;
    VBUFFER_SIZE += vsize;
    Assert(VBUFFER_SIZE <= MAX_VERTICES);

    for (u32 i = 0; i < tsize; ++i) {
        IBUFFER[IBUFFER_SIZE + i] = vbufferidx + ts[i];
    }
    u32 ibufferidx = IBUFFER_SIZE;
    IBUFFER_SIZE += tsize;
    Assert(IBUFFER_SIZE <= MAX_TRIANGLES);

    u32 ntriangles = tsize / 3;

    LMesh* mesh = &MESHES[MESHES_SIZE];
    ++MESHES_SIZE;
    mesh->ibufferidx = ibufferidx;
    mesh->ntriangles = ntriangles;
}

static void RenderDataInit() {
    static bool INIT = false;
    Assert(!INIT);
    INIT = true;

    // Important!!!! Init the buffer before sending to GPU or else no vertex data!!!!!

    // Cube.
    {
        Vertex vs[] = {
            {{{0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f}}},
            {{{-0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f}}},
            {{{-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f}}},
            {{{0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f}}},

            // Front.
            {{{-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},
            {{{0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},
            {{{0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},
            {{{-0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},

            // Left.
            {{{-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f}}},
            {{{-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f}}},
            {{{-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f}}},
            {{{-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f}}},

            // Right.
            {{{0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f}}},
            {{{0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f}}},
            {{{0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f}}},
            {{{0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f}}},

            // Bottom.
            {{{-0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f}}},
            {{{0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f}}},
            {{{0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f}}},
            {{{-0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f}}},

            // Top.
            {{{0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f}}},
            {{{-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f}}},
            {{{-0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f}}},
            {{{0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f}}},
        };
        f32 ts[] = {
            // Back.
            0, 1, 2,  //
            2, 3, 0,  //

            // Front.
            4, 5, 6,  //
            6, 7, 4,  //

            // Left.
            8, 9, 10,   //
            10, 11, 8,  //

            // Right.
            12, 13, 14,  //
            14, 15, 12,  //

            // Bottom.
            16, 17, 18,  //
            18, 19, 16,  //

            // Top.
            20, 21, 22,  //
            22, 23, 20,  //
        };
        AddMesh(vs, ArraySizeOf(vs), ts, ArraySizeOf(ts));
    }

    // Quad.
    {
        Vertex vs[] = {
            {{{-0.5f, 0.5f, 0.0f, 1.0f, 0.5f, 0.31f, 0.0f, 0.0f, 1.0f}}},
            {{{-0.5f, -0.5f, 0.0f, 1.0f, 0.5f, 0.31f, 0.0f, 0.0f, 1.0f}}},
            {{{0.5f, -0.5f, 0.0f, 1.0f, 0.5f, 0.31f, 0.0f, 0.0f, 1.0f}}},
            {{{0.5f, 0.5f, 0.0f, 1.0f, 0.5f, 0.31f, 0.0f, 0.0f, 1.0f}}},
        };
        f32 ts[] = {
            0, 1, 2,  //
            2, 3, 0   //
        };
        AddMesh(vs, ArraySizeOf(vs), ts, ArraySizeOf(ts));
    }

    // ISO TRIANGLE Mesh.
    {
        Vertex vs[] = {
            {{{0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},
            {{{-1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},
            {{{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},
        };
        f32 ts[] = {0, 1, 2};
        AddMesh(vs, ArraySizeOf(vs), ts, ArraySizeOf(ts));
    }

    // PYRAMID Mesh.
    {
        Vertex vs[] = {
            // Base
            {{{-1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},   //
            {{{-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},  //
            {{{1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}}},   //
            {{{1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},    //

            // Top point.
            {{{0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f}}},  //
        };
        f32 ts[] = {
            // Base
            0, 1, 2,  //
            2, 3, 0,  //
            // Top
            0, 3, 4,  //
            // Left
            1, 0, 4,  //
            // Right
            3, 2, 4,  //
            // Bottom
            2, 1, 4,  //
        };
        AddMesh(vs, ArraySizeOf(vs), ts, ArraySizeOf(ts));
    }

    // 2S ISO TRIANGLE Mesh.
    {
        Vertex vs[] = {
            {{{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},
            {{{-0.5f, 0.866f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},
            {{{-0.5f, -0.866f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},
            {{{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f}}},
            {{{-0.5f, -0.866f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f}}},
            {{{-0.5f, 0.866f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f}}},
        };
        f32 ts[] = {
            0, 1, 2,  //
            3, 4, 5,  //
        };
        AddMesh(vs, ArraySizeOf(vs), ts, ArraySizeOf(ts));
    }

    LShader shader = GetShader();

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VBUFFER), VBUFFER, GL_STATIC_DRAW);

    glGenBuffers(1, &VIO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VIO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(IBUFFER), IBUFFER, GL_STATIC_DRAW);

    glVertexAttribPointer(shader.vpos_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(shader.vpos_loc);
    glVertexAttribPointer(shader.vcolor_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(shader.vcolor_loc);
    glVertexAttribPointer(shader.vnormal_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(shader.vnormal_loc);
}

static Vertex LMeshGetVertex(LMesh mid, u32 vid) { return VBUFFER[IBUFFER[mid.ibufferidx + vid]]; }

static Triangle LMeshGetTriangle(LMesh mid, u32 tid) {
    u32 offset = tid * 3;
    Vertex v0 = LMeshGetVertex(mid, offset + 0);
    Vertex v1 = LMeshGetVertex(mid, offset + 1);
    Vertex v2 = LMeshGetVertex(mid, offset + 2);
    return TriangleLit(v0, v1, v2);
}

typedef struct DrawCmd {
    LMesh meshid;
    Mat4 modelm;  // converts from model space to world space.
    Vec4 color;
    bool wireframe;
} DrawCmd;

#define MAX_CMDS 4092
static DrawCmd CBUFFER[MAX_CMDS] = {0};
static u32 CBUFFER_SIZE = 0;

static DrawCmd* AddCmd(LMesh meshid, Mat4 modelm, Vec4 color) {
    Assert(CBUFFER_SIZE < MAX_CMDS);
    DrawCmd* cmd = &CBUFFER[CBUFFER_SIZE];
    ++CBUFFER_SIZE;
    memset(cmd, 0, sizeof(CBUFFER[0]));
    cmd->meshid = meshid;
    cmd->modelm = modelm;
    cmd->color = color;
    cmd->wireframe = false;
    return cmd;
}

static void CmdBufferZero() {
    CBUFFER_SIZE = 0;
    memset(CBUFFER, 0, ArraySizeOf(CBUFFER) * sizeof(CBUFFER[0]));
}

////////////////////////////////////////////////////////////

const float RenderWidth = 1280;
const float RenderHeight = 720;

static const Vec3 ORIGIN = Vec3Lit(0.0f, 0.0f, 0.0f);
static const Vec2 ORIGIN2D = {ORIGIN.x, ORIGIN.y};

////////////////////////////////////////////////////////////
typedef enum EntityFlag {
    EFLAG_NONE = 0,
    EFLAG_ACTIVE = 1 << 0,
    EFLAG_DRAW = 1 << 1,
    EFLAG_PLAYER_CONTROLLED = 1 << 2,
    EFLAG_PHYSICS_ON = 1 << 3
} EntityFlag;

typedef u32 EntityId;

typedef struct Entity {
    /////////////////////////////////////////////////////////////////
    // Core field.
    EntityId id;
    EntityFlag flags;

    // Decomposition of model matrix.
    Vec3 pos;
    f32 scale;  // only allow uniform scaling.
    Mat4 orient;

    Vec3 v;

    LMesh mesh;
    Vec4 color;

    /////////////////////////////////////////////////////////////////
    // Gameplay fields.

    struct {
        struct Entity* prev;
        struct Entity* next;
    } monster_link;

} Entity;

#define ENTITY_CAP 1024

typedef struct EntityManager {
    Entity entities[ENTITY_CAP];
} EntityManager;

static EntityManager EntityM = {0};

#define GetEntityManager() &EntityM

static void EntitySystemInit() { memset(&EntityM, 0, sizeof(EntityM)); }

static Entity* GetEntity(EntityId id) {
    EntityManager* em = GetEntityManager();
    Entity* entity = &em->entities[id];
    return entity;
}

static EntityId NextEntityId() {
    EntityManager* em = GetEntityManager();
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
        Entity* entity = &em->entities[i];
        if (!TestFlag(entity->flags, EFLAG_ACTIVE)) {
            return i;
        }
    }
    Assert(false);
    return 0;
}

static Entity* AllocateEntity() {
    EntityId id = NextEntityId();
    Entity* entity = GetEntity(id);
    Assert(entity != NULL);
    memset(entity, 0, sizeof(*entity));
    entity->id = id;
    entity->flags = EFLAG_ACTIVE;
    entity->scale = 1.0f;
    entity->orient = Mat4Identity();
    entity->color = Vec4Lit(0.0f, 0.0f, 0.0f, 1.0f);
    FlagOn(entity->flags, EFLAG_DRAW);
    return entity;
}

typedef struct GameState {
    /////////////////////////////////////////
    // Core fields
    bool debug;
    bool game_over;

    float timer;

    /////////////////////////////////////////
    // Gameplay fields
} GameState;

static GameState GState;

#define GetGameState() (&GState)

static void GameStateInit(GameState* gstate) {
    memset(gstate, 0, sizeof(GameState));

    InitWindow(RenderWidth, RenderHeight, "Game");
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetConfigFlags(FLAG_VSYNC_HINT);
    SetTargetFPS(60);
    EntitySystemInit();
    // DisableCursor();

    { glEnable(GL_DEPTH_TEST); }
}

static Mat4 EntityGetMat(Entity* e) {
    Mat4 scale = Mat4Scale(e->scale);
    Mat4 translate = Mat4TranslateV(e->pos);
    Mat4 model = Mat4Multiply(translate, Mat4Multiply(e->orient, scale));
    return model;
}

static bool CollideEntityEntity(Entity* e0, Entity* e1, Vec3* dirout, f32* dout) {
    Mat4 e0m = EntityGetMat(e0);
    Mat4 e1m = EntityGetMat(e1);

    // NOTE: We do collision detection in world space.
    Vec3 m0[MAX_VERTICES] = {0};
    u32 m0size = 0;
    for (u32 i = 0; i < e0->mesh.ntriangles * 3; ++i, ++m0size) {
        // Ensure both meshes are in same frame of reference.
        Vec3 pos = LMeshGetVertex(e0->mesh, i).pos;
        m0[i] = Vec3WTransform(pos, e0m);
    }

    Vec3 m1[MAX_VERTICES] = {0};
    u32 m1size = 0;
    for (u32 i = 0; i < e0->mesh.ntriangles * 3; ++i, ++m1size) {
        // Ensure both meshes are in same frame of reference.
        Vec3 pos = LMeshGetVertex(e1->mesh, i).pos;
        m1[i] = Vec3WTransform(pos, e1m);
    }

    // Will collide two convex polyhedra (given as transformed mesh buffer) by using the SAT to test
    // all faces of mesh A against mesh B. atob represents a vector (i.e. no position) pointing from
    // A's center to B's center to properly orient the output separating axis (which will point from A -> B).
    Vec3 dir = Vec3Zero();
    f32 dmin = F32_MAX;
    bool collides = false;

    // Check faces A against B and vice versa.
    Vec3* mas[] = {m0, m1};
    f32 masizes[] = {m0size, m1size};
    Vec3* mbs[] = {m1, m0};
    f32 mbsizes[] = {m1size, m0size};
    for (u32 midx = 0; midx < ArraySizeOf(mas); ++midx) {
        Vec3* ma = mas[midx];
        f32 masize = masizes[midx];
        Vec3* mb = mbs[midx];
        f32 mbsize = mbsizes[midx];
        for (u32 i = 0; i < masize; i += 3) {
            Vec3 a = ma[i + 0];
            Vec3 b = ma[i + 1];
            Vec3 c = ma[i + 2];

            // Calc face normal. Fine to do so in the transformed space of the mesh
            // as we will adjust orientation to be A -> B at end.
            Vec3 ab = Vec3Sub(b, a);
            Vec3 ac = Vec3Sub(c, a);
            Vec3 axis = Vec3Normalize(Vec3Cross(ab, ac));

            // Project mesh A along line and get interval.
            f32 tamin = Vec3Dot(ma[0], axis);
            f32 tamax = tamin;
            for (u32 k = 1; k < masize; ++k) {
                f32 n = Vec3Dot(ma[k], axis);
                tamin = f32min(tamin, n);
                tamax = f32max(tamax, n);
            }

            // Project mesh B along line and get interval.
            f32 tbmin = Vec3Dot(mb[0], axis);
            f32 tbmax = tbmin;
            for (u32 k = 1; k < mbsize; ++k) {
                f32 n = Vec3Dot(mb[k], axis);
                tbmin = f32min(tbmin, n);
                tbmax = f32max(tbmax, n);
            }

            // TODO: Add some margin here.
            if (tamax < tbmin || tamin > tbmax) {
                collides = false;
                dir = axis;  // Useful to keep axis of separationg for debugging.
                goto collideret;
            }

            // Check for overlap. this will give magnitude of displacement if positive
            // (dir of displacement is the separating axis).
            // Negative value here means no overlap.
            f32 imin = f32max(tamin, tbmin);
            f32 imax = f32min(tamax, tbmax);
            f32 d = imax - imin;
            Assert(d > 0.0f);

            collides = true;
            if (d < dmin) {
                dir = axis;
                dmin = d;
            }
        }
    }

    // Check all edge/edge combinations between all of the triangles of A and B.
    {
        Vec3* ma = m0;
        f32 masize = m0size;
        Vec3* mb = m1;
        f32 mbsize = m1size;
        for (u32 ti = 0; ti < masize; ti += 3) {
            Vec3 a = ma[ti + 0];
            Vec3 b = ma[ti + 1];
            Vec3 c = ma[ti + 2];

            Vec3 ab = Vec3Sub(b, a);
            Vec3 ac = Vec3Sub(c, a);
            Vec3 bc = Vec3Sub(c, b);

            for (u32 tj = 0; tj < mbsize; tj += 3) {
                Vec3 d = mb[tj + 0];
                Vec3 e = mb[tj + 1];
                Vec3 f = mb[tj + 2];

                Vec3 de = Vec3Sub(e, d);
                Vec3 df = Vec3Sub(f, d);
                Vec3 ef = Vec3Sub(f, e);

                // Compute all edge/edge combinations between A and B.
                Vec3 ns[] = {
                    Vec3Normalize(Vec3Cross(ab, de)),  //
                    Vec3Normalize(Vec3Cross(ab, df)),  //
                    Vec3Normalize(Vec3Cross(ab, ef)),  //

                    Vec3Normalize(Vec3Cross(ac, de)),  //
                    Vec3Normalize(Vec3Cross(ac, df)),  //
                    Vec3Normalize(Vec3Cross(ac, ef)),  //

                    Vec3Normalize(Vec3Cross(bc, de)),  //
                    Vec3Normalize(Vec3Cross(bc, df)),  //
                    Vec3Normalize(Vec3Cross(bc, ef)),  //
                };
                for (u32 ni = 0; ni < ArraySizeOf(ns); ++ni) {
                    Vec3 axis = ns[ni];
                    // TODO: Handle this case using Ericson p159. It does happen.
                    // Assert(!Vec3Eq(axis, Vec3Zero()));
                    if (Vec3Eq(axis, Vec3Zero())) {
                        continue;
                    }

                    // Project mesh A along line and get interval.
                    f32 tamin = Vec3Dot(ma[0], axis);
                    f32 tamax = tamin;
                    for (u32 k = 1; k < masize; ++k) {
                        f32 n = Vec3Dot(ma[k], axis);
                        tamin = f32min(tamin, n);
                        tamax = f32max(tamax, n);
                    }

                    // Project mesh B along line and get interval.
                    f32 tbmin = Vec3Dot(mb[0], axis);
                    f32 tbmax = tbmin;
                    for (u32 k = 1; k < mbsize; ++k) {
                        f32 n = Vec3Dot(mb[k], axis);
                        tbmin = f32min(tbmin, n);
                        tbmax = f32max(tbmax, n);
                    }

                    // TODO: Add some margin here.
                    if (tamax < tbmin || tamin > tbmax) {
                        collides = false;
                        dir = axis;  // Useful to keep axis of separationg for debugging.
                        goto collideret;
                    }

                    // Check for overlap. this will give magnitude of displacement if positive
                    // (dir of displacement is the separating axis).
                    // Negative value here means no overlap.
                    f32 imin = f32max(tamin, tbmin);
                    f32 imax = f32min(tamax, tbmax);
                    f32 d = imax - imin;
                    Assert(d > 0.0f);

                    collides = true;
                    if (d < dmin) {
                        dir = axis;
                        dmin = d;
                    }
                }
            }
        }
    }

collideret:
    // By convention, we need dir to point towards B. It might not if the shapes
    // have normals pointing in opposite directions (the magnitude will be the same)
    // TODO: Is this really necessary? shouldn't the above tests give me the correct direction
    // vector? this also really only works if both e0 and e1 are convex shapes since we are
    // guaranteed their centers will be on either side of the separating plane.
    // TODO: Is the above even true?
    Vec3 atob = Vec3Sub(e1->pos, e0->pos);
    f32 flip = Vec3Dot(dir, atob) < 0.0f ? -1.0f : 1.0f;
    dir = Vec3Scale(dir, flip);

    *dirout = dir;
    *dout = dmin;
    return collides;
}

static void DrawEntities(LShader shader, EntityId* entities, u32 size) {
    for (u32 i = 0; i < size; ++i) {
        Entity* entity = GetEntity(entities[i]);
        Mat4 scale = Mat4Scale(entity->scale);
        Mat4 translate = Mat4TranslateV(entity->pos);
        Mat4 model = Mat4Multiply(translate, Mat4Multiply(entity->orient, scale));
        AddCmd(entity->mesh, model, entity->color);
    }
}

// TODO: Consider deriving partial orientation from arrow direction and then having passed in orient
// be the upright orientation of the entitie's rotation. Maybe this function here could be the Pro version
// and the other could be default.
static void DrawArrow(Vec3 forward, Vec3 pos, Mat4 orient, f32 scale, Vec4 color) {
    forward = Vec3Normalize(forward);
    // tail.
    AddCmd(GetMesh(M_CUBE), Mat4SRT(scale, orient, pos), color);
    pos = Vec3Add(pos, Vec3Scale(forward, 1.0f * scale));
    // tip.
    AddCmd(GetMesh(M_PYRAMID), Mat4SRT(scale, orient, pos), color);
}

static void GameStateUpdate(GameState* gstate, float dt) {
    gstate->timer += dt;

    EntityManager* em = GetEntityManager();
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
        Entity* entity = GetEntity(i);
        if (!TestFlag(entity->flags, EFLAG_ACTIVE)) {
            continue;
        }

        if (TestFlag(entity->flags, EFLAG_PLAYER_CONTROLLED)) {
            static const u8 GAMEPAD_ID = 0;

            f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_X);
            f32 y = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_Y);

            // TODO: auto align movement direction to camera position.
            Vec3 dir = Vec3Normalize(Vec3Lit(x, 0.0f, y));
            entity->pos = Vec3Add(entity->pos, Vec3Scale(dir, 30.0f * dt));
        }
    }

    gstate->debug ^= (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_ONE));
}

#define MAX_ENTITY_BATCH 256

static void GameStateDraw(GameState* gstate, float delta) {
    static Vec3 viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);
    static Vec3 vieworient = Vec3Lit(0.0f, 0.0f, 0.0f);

    static bool INIT = false;
    static Vec3 LIGHT_POS = Vec3Lit(10.0f, 10.0f, -10.0f);

    static EntityId LIGHTS[MAX_ENTITY_BATCH] = {0};
    // static u32 LIGHTS_SIZE = 0;

    static EntityId SPINNING[MAX_ENTITY_BATCH] = {0};
    static u32 SPINNING_SIZE = 0;

    static EntityId SHADOW_DEBUG[MAX_ENTITY_BATCH] = {0};
    static u32 SHADOW_DEBUG_SIZE = 0;

    static EntityId PHYSICS[MAX_ENTITY_BATCH] = {0};
    static u32 PHYSICS_SIZE = 0;

    static EntityId ISO_TRIANGLES[MAX_ENTITY_BATCH] = {0};
    static u32 ISO_TRIANGLES_SIZE = 0;

    static EntityId COLLISIONS[MAX_ENTITY_BATCH] = {0};
    static u32 COLLISIONS_SIZE = 0;

    static EntityId PYRAMIDS[MAX_ENTITY_BATCH] = {0};
    static u32 PYRAMIDS_SIZE = 0;

    static EntityId WALLS[MAX_ENTITY_BATCH] = {0};
    static u32 WALLS_SIZE = 0;

    // Init logic.
    if (!INIT) {
        INIT = true;

        RenderDataInit();
        vieworient.yaw = Deg2Rad(90.0f);

        {
            Vec3 positions[] = {LIGHT_POS};
            Vec4 colors[] = {LBLUE};

            EntityId* entities = LIGHTS;
            // LIGHTS_SIZE = ArraySizeOf(positions);
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_CUBE);
                entity->color = colors[i];
                entities[i] = entity->id;
            }
        }

        {
            Vec3 positions[] = {Vec3Lit(0.0f, 0.0f, 0.0f), Vec3Lit(-5.0f, -3.0f, -3.0f), Vec3Lit(15.0f, 3.0f, -50.0f),
                                Vec3Lit(-15.0f, 0.0f, 0.0f)};
            Vec4 colors[] = {LBLUE, LBLUE, LBLUE, LBLUE};

            EntityId* entities = SPINNING;
            SPINNING_SIZE = ArraySizeOf(positions);
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_CUBE);
                entity->color = colors[i];
                entities[i] = entity->id;
            }
        }

        {
            Vec3 positions[] = {Vec3Lit(10.0f, 3.0f, 0.0f), Vec3Lit(0.0f, 3.0f, 5.0f)};
            f32 rotations[] = {Deg2Rad(45.0f), Deg2Rad(0.0f)};
            Vec4 colors[] = {LGREEN, LGREEN};

            EntityId* entities = SHADOW_DEBUG;
            SHADOW_DEBUG_SIZE = ArraySizeOf(positions);
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_CUBE);
                entity->orient = Mat4Rotate(rotations[i], Vec3Lit(0.0f, 1.0f, 0.0f));
                entity->color = colors[i];
                entity->scale = 3.0f;
                entities[i] = entity->id;
            }
        }

        {
            static Vec3 positions[] = {Vec3Lit(-20.0f, 3.0f, 0.0f)};
            static Vec3 velocities[] = {Vec3Lit(0.0f, 0.0f, -10.0f)};
            static f32 rotations[] = {Deg2Rad(0.0f)};
            Vec4 colors[] = {LPURPLE};

            EntityId* entities = PHYSICS;
            PHYSICS_SIZE = ArraySizeOf(positions);
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_CUBE);
                entity->orient = Mat4Rotate(rotations[i], Vec3Lit(0.0f, 1.0f, 0.0f));
                entity->color = colors[i];
                entity->scale = 3.0f;
                entity->v = velocities[i];
                entities[i] = entity->id;
            }
        }

        // ISO TRIANGLES Init.
        {
            static Vec3 positions[] = {Vec3Lit(-20.0f, 3.0f, 15.0f), Vec3Lit(-20.0f, 3.0f, 20.0f)};
            static f32 rotations[] = {Deg2Rad(0.0f), Deg2Rad(90.0f)};
            Vec4 colors[] = {LBLACK, LBLACK};

            EntityId* entities = ISO_TRIANGLES;
            ISO_TRIANGLES_SIZE = ArraySizeOf(positions);
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_2S_ISO_TRIANGLE);
                entity->orient = Mat4Rotate(rotations[i], Vec3Lit(0.0f, 1.0f, 0.0f));
                entity->color = colors[i];
                entity->scale = 3.0f;
                entities[i] = entity->id;
            }
        }

        // Collisions Init.
        {
            static const Vec3 row_margin = Vec3Lit(0.0f, 0.0f, 5.0f);
            static const Vec3 col_margin = Vec3Lit(-10.0f, 0.0f, 0.0f);
            static const Vec3 origin = Vec3Lit(20.0f, 3.0f, 15.0f);

            EntityId* entities = COLLISIONS;
            u32 size = 0;
            u32 ncols = 0;

            // Test all face-face combinations.
            {
                static Vec3 axis[] = {
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

                };
                static f32 rotations[] = {
                    Deg2Rad(0.0f),   Deg2Rad(0.0f),    //
                    Deg2Rad(0.0f),   Deg2Rad(90.0f),   //
                    Deg2Rad(0.0f),   Deg2Rad(180.0f),  //
                    Deg2Rad(0.0f),   Deg2Rad(270.0f),  //
                    Deg2Rad(0.0f),   Deg2Rad(90.0f),   //
                    Deg2Rad(0.0f),   Deg2Rad(-90.0f),  //

                    Deg2Rad(90.0f),  Deg2Rad(0.0f),    //
                    Deg2Rad(90.0f),  Deg2Rad(90.0f),   //
                    Deg2Rad(90.0f),  Deg2Rad(180.0f),  //
                    Deg2Rad(90.0f),  Deg2Rad(270.0f),  //
                    Deg2Rad(90.0f),  Deg2Rad(90.0f),   //
                    Deg2Rad(90.0f),  Deg2Rad(-90.0f),  //

                    Deg2Rad(180.0f), Deg2Rad(0.0f),    //
                    Deg2Rad(180.0f), Deg2Rad(90.0f),   //
                    Deg2Rad(180.0f), Deg2Rad(180.0f),  //
                    Deg2Rad(180.0f), Deg2Rad(270.0f),  //
                    Deg2Rad(180.0f), Deg2Rad(90.0f),   //
                    Deg2Rad(180.0f), Deg2Rad(-90.0f),  //

                    Deg2Rad(180.0f), Deg2Rad(0.0f),    //
                    Deg2Rad(270.0f), Deg2Rad(90.0f),   //
                    Deg2Rad(270.0f), Deg2Rad(180.0f),  //
                    Deg2Rad(270.0f), Deg2Rad(270.0f),  //
                    Deg2Rad(270.0f), Deg2Rad(90.0f),   //
                    Deg2Rad(270.0f), Deg2Rad(-90.0f),  //

                    Deg2Rad(90.0f),  Deg2Rad(0.0f),    //
                    Deg2Rad(90.0f),  Deg2Rad(90.0f),   //
                    Deg2Rad(90.0f),  Deg2Rad(180.0f),  //
                    Deg2Rad(90.0f),  Deg2Rad(270.0f),  //
                    Deg2Rad(90.0f),  Deg2Rad(90.0f),   //
                    Deg2Rad(90.0f),  Deg2Rad(-90.0f),  //

                    Deg2Rad(-90.0f), Deg2Rad(0.0f),    //
                    Deg2Rad(-90.0f), Deg2Rad(90.0f),   //
                    Deg2Rad(-90.0f), Deg2Rad(180.0f),  //
                    Deg2Rad(-90.0f), Deg2Rad(270.0f),  //
                    Deg2Rad(-90.0f), Deg2Rad(90.0f),   //
                    Deg2Rad(-90.0f), Deg2Rad(-90.0f),  //

                };

                {
                    Vec3 pos = Vec3Add(origin, Vec3Scale(col_margin, ncols));
                    ++ncols;

                    for (u32 i = 0; i < ArraySizeOf(rotations); i += 2, size += 2) {
                        {
                            u32 j = i;
                            Entity* entity = AllocateEntity();
                            entity->pos = pos;
                            entity->mesh = GetMesh(M_CUBE);
                            entity->orient = Mat4Rotate(rotations[j], axis[j]);
                            entity->color = LBLUE;  //: LORANGE;
                            entity->color.a = 0.5f;
                            entity->scale = 3.0f;  // 1.0f;
                            entities[size + 0] = entity->id;
                        }
                        {
                            u32 j = i + 1;
                            Entity* entity = AllocateEntity();
                            entity->pos = Vec3Add(pos, row_margin);
                            entity->mesh = GetMesh(M_CUBE);
                            entity->orient = Mat4Rotate(rotations[j], axis[j]);
                            entity->color = LORANGE;
                            entity->color.a = 0.5f;
                            entity->scale = 1.0f;
                            entities[size + 1] = entity->id;
                        }

                        {
                            Entity* e0 = GetEntity(entities[size + 0]);
                            Entity* e1 = GetEntity(entities[size + 1]);

                            // Collide in half sec.
                            // NOTE: this will just collide the centers, but actual collision might occur sooner.
                            Vec3 v = Vec3Sub(e0->pos, e1->pos);
                            f32 sec = 3.0f;
                            v = Vec3Scale(v, 1.0f / sec);
                            e1->v = v;
                        }

                        pos = Vec3Add(pos, Vec3Scale(row_margin, 2.0f));
                    }
                }

                // Col 1 rotation comb + obj B on top.
                {
                    Vec3 pos = Vec3Add(origin, Vec3Scale(col_margin, ncols));
                    ++ncols;

                    for (u32 i = 0; i < ArraySizeOf(rotations); i += 2, size += 2) {
                        {
                            u32 j = i;
                            Entity* entity = AllocateEntity();
                            entity->pos = pos;
                            entity->mesh = GetMesh(M_CUBE);
                            entity->orient = Mat4Rotate(rotations[j], axis[j]);
                            entity->color = LBLUE;  //: LORANGE;
                            entity->color.a = 0.5f;
                            entity->scale = 3.0f;  // 1.0f;
                            entities[size + 0] = entity->id;
                        }
                        {
                            u32 j = i + 1;
                            Entity* entity = AllocateEntity();
                            entity->pos = Vec3Add(pos, Vec3Lit(0.0f, 5.0f, 0.0f));
                            entity->mesh = GetMesh(M_CUBE);
                            entity->orient = Mat4Rotate(rotations[j], axis[j]);
                            entity->color = LORANGE;
                            entity->color.a = 0.5f;
                            entity->scale = 1.0f;
                            entities[size + 1] = entity->id;
                        }

                        {
                            Entity* e0 = GetEntity(entities[size + 0]);
                            Entity* e1 = GetEntity(entities[size + 1]);

                            // Collide in half sec.
                            // NOTE: this will just collide the centers, but actual collision might occur sooner.
                            Vec3 v = Vec3Sub(e0->pos, e1->pos);
                            f32 sec = 3.0f;
                            v = Vec3Scale(v, 1.0f / sec);
                            e1->v = v;
                        }

                        pos = Vec3Add(pos, Vec3Scale(row_margin, 2.0f));
                    }
                }
            }

            // Test some basic cube rotations.
            {
                static Vec3 axis[] = {
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.3f, 0.5f),                //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.707106781f, -0.707106781f, 0),  //

                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //

                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f),  //

                    // Reverse.
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //

                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //

                    Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                    Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
                };
                static f32 rotations[] = {
                    Deg2Rad(45.0f), Deg2Rad(45.0f),    //
                    Deg2Rad(53.0f), Deg2Rad(53.0f),    //
                    Deg2Rad(-72.0f), Deg2Rad(-72.0f),  //
                    Deg2Rad(0.1f), Deg2Rad(0.1f),      //
                    Deg2Rad(-12.0f), Deg2Rad(-12.0f),  //

                    Deg2Rad(0.0f), Deg2Rad(120.0f),   //
                    Deg2Rad(0.0f), Deg2Rad(125.264),  //

                    Deg2Rad(0.0f), Deg2Rad(45.0f),   //
                    Deg2Rad(0.0f), Deg2Rad(53.0f),   //
                    Deg2Rad(0.0f), Deg2Rad(-72.0f),  //
                    Deg2Rad(0.0f), Deg2Rad(0.1f),    //
                    Deg2Rad(0.0f), Deg2Rad(-12.0f),  //

                    Deg2Rad(0.0f), Deg2Rad(45.0f),   //
                    Deg2Rad(0.0f), Deg2Rad(53.0f),   //
                    Deg2Rad(0.0f), Deg2Rad(-72.0f),  //
                    Deg2Rad(0.0f), Deg2Rad(0.1f),    //
                    Deg2Rad(0.0f), Deg2Rad(-12.0f),  //

                    Deg2Rad(0.0f), Deg2Rad(45.0f),   //
                    Deg2Rad(0.0f), Deg2Rad(53.0f),   //
                    Deg2Rad(0.0f), Deg2Rad(-72.0f),  //
                    Deg2Rad(0.0f), Deg2Rad(0.1f),    //
                    Deg2Rad(0.0f), Deg2Rad(-12.0f),  //

                    // Reverse
                    Deg2Rad(45.0f), Deg2Rad(0.0f),   //
                    Deg2Rad(53.0f), Deg2Rad(0.0f),   //
                    Deg2Rad(-72.0f), Deg2Rad(0.0f),  //
                    Deg2Rad(0.1f), Deg2Rad(0.0f),    //
                    Deg2Rad(-12.0f), Deg2Rad(0.0f),  //

                    Deg2Rad(45.0f), Deg2Rad(0.0f),   //
                    Deg2Rad(53.0f), Deg2Rad(0.0f),   //
                    Deg2Rad(-72.0f), Deg2Rad(0.0f),  //
                    Deg2Rad(0.1f), Deg2Rad(0.0f),    //
                    Deg2Rad(-12.0f), Deg2Rad(0.0f),  //

                    Deg2Rad(45.0f), Deg2Rad(0.0f),   //
                    Deg2Rad(53.0f), Deg2Rad(0.0f),   //
                    Deg2Rad(-72.0f), Deg2Rad(0.0f),  //
                    Deg2Rad(0.1f), Deg2Rad(0.0f),    //
                    Deg2Rad(-12.0f), Deg2Rad(0.0f),  //
                };

                {
                    Vec3 pos = Vec3Add(origin, Vec3Scale(col_margin, ncols));
                    ++ncols;

                    for (u32 i = 0; i < ArraySizeOf(rotations); i += 2, size += 2) {
                        {
                            u32 j = i;
                            Entity* entity = AllocateEntity();
                            entity->pos = pos;
                            entity->mesh = GetMesh(M_CUBE);
                            entity->orient = Mat4Rotate(rotations[j], axis[j]);
                            entity->color = LBLUE;  //: LORANGE;
                            entity->color.a = 0.5f;
                            entity->scale = 3.0f;  // 1.0f;
                            entities[size + 0] = entity->id;
                        }
                        {
                            u32 j = i + 1;
                            Entity* entity = AllocateEntity();
                            entity->pos = Vec3Add(pos, row_margin);
                            entity->mesh = GetMesh(M_CUBE);
                            entity->orient = Mat4Rotate(rotations[j], axis[j]);
                            entity->color = LORANGE;
                            entity->color.a = 0.5f;
                            entity->scale = 1.0f;
                            entities[size + 1] = entity->id;
                        }

                        {
                            Entity* e0 = GetEntity(entities[size + 0]);
                            Entity* e1 = GetEntity(entities[size + 1]);

                            // Collide in half sec.
                            // NOTE: this will just collide the centers, but actual collision might occur sooner.
                            Vec3 v = Vec3Sub(e0->pos, e1->pos);
                            f32 sec = 3.0f;
                            v = Vec3Scale(v, 1.0f / sec);
                            e1->v = v;
                        }

                        pos = Vec3Add(pos, Vec3Scale(row_margin, 2.0f));
                    }
                }
            }

            COLLISIONS_SIZE = size;
            Assert(COLLISIONS_SIZE < MAX_ENTITY_BATCH);
        }

        // PYRAMIDS init
        {
            static Vec3 positions[] = {Vec3Lit(20.0f, 3.0f, -15.0f)};
            static f32 rotations[] = {Deg2Rad(0.0f)};
            Vec4 colors[] = {LORANGE};

            EntityId* entities = PYRAMIDS;
            PYRAMIDS_SIZE = ArraySizeOf(positions);
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_PYRAMID);
                entity->orient = Mat4Rotate(rotations[i], Vec3Lit(1.0f, 0.0f, 0.0f));
                entity->color = colors[i];
                entity->scale = 3.0f;
                entities[i] = entity->id;
            }
        }

        {
            static Vec3 positions[] = {Vec3Lit(0.0f, -10.0f, 0.0f)};
            static f32 rotations[] = {Deg2Rad(-90.0f)};
            static Vec3 axis[] = {Vec3Lit(1.0f, 0.0f, 0.0f)};
            Vec4 colors[] = {Vec4Lit(1.0f, 0.5f, 0.31f, 1.0f)};

            EntityId* entities = WALLS;
            WALLS_SIZE = ArraySizeOf(positions);
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_QUAD);
                entity->orient = Mat4Rotate(rotations[i], axis[i]);
                entity->color = colors[i];
                entity->scale = 10000.0f;
                entities[i] = entity->id;
            }
        }
    }

    // Update logic.
    {
        // Camera movement.
        {
            static const u8 GAMEPAD_ID = 0;

            // Left stick movement.
            {
                static const f32 WALK_VEL = 20.0f;
                static const f32 RUN_VEL = 2.0f * WALK_VEL;

                f32 vel = WALK_VEL;
                if (IsGamepadButtonDown(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                    vel = RUN_VEL;
                }

                f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_X);
                f32 y = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_Y);
                Vec3 dir_model = Vec3Lit(x, 0.0f, y);

                Vec3 bodyorient = vieworient;
                bodyorient.pitch = 0.0f;  // fps style movement rather than free cam movement.
                Mat4 rotation = Mat4RotateEuler(bodyorient);

                Vec3 dir_world = Vec3Transform(dir_model, rotation);
                Vec3 mov_world = Vec3Scale(dir_world, vel * delta);
                viewpos = Vec3Add(viewpos, mov_world);
            }

            // Right stick movement.
            {
                f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_X);
                f32 y = -1.0f * GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_Y);
                if (x != 0.0f || y != 0.0f) {
                    static const rad VEL = Deg2Rad(90.0f);

                    // rotate orient.
                    vieworient.yaw += -1.0f * x * VEL * delta;
                    vieworient.pitch += y * VEL * delta;
                    // Clamp pitch to avoid looking too high/low which would sign flip things.
                    vieworient.pitch = f32clamp(vieworient.pitch, Deg2Rad(-89.0f), Deg2Rad(89.0f));
                }
            }
        }

        {
            LShader shader = GetShader();
            {
                EntityId* entities = SPINNING;
                u32 size = SPINNING_SIZE;
                for (u32 i = 0; i < size; ++i) {
                    Entity* entity = GetEntity(entities[i]);
                    entity->orient = Mat4Rotate(gstate->timer, Vec3Lit(1.0f, 0.3f, 0.5f));
                }
                DrawEntities(shader, entities, size);
            }

            // Draw non rotating cube for easier lighting visual debug.
            {
                EntityId* entities = SHADOW_DEBUG;
                u32 size = SHADOW_DEBUG_SIZE;
                DrawEntities(shader, entities, size);
            }

            // Physics cube.
            {
                EntityId* entities = PHYSICS;
                u32 size = PHYSICS_SIZE;
                for (u32 i = 0; i < size; ++i) {
                    Entity* entity = GetEntity(entities[i]);
                    entity->pos = Vec3Add(entity->pos, Vec3Scale(entity->v, delta));
                }
                DrawEntities(shader, entities, size);
            }

            // ISO TRIANGLES DRAW Draw.
            {
                EntityId* entities = ISO_TRIANGLES;
                u32 size = ISO_TRIANGLES_SIZE;

                {
                    Entity* e0 = GetEntity(entities[0]);
                    Entity* e1 = GetEntity(entities[1]);
                    Mat4 e0m = EntityGetMat(e0);
                    Mat4 e1m = EntityGetMat(e1);
                    Mat4 e1toe0m = Mat4Multiply(Mat4Invert(e0m), e1m);

                    Vec3 a = LMeshGetVertex(e0->mesh, 0).pos;
                    Vec3 b = LMeshGetVertex(e0->mesh, 1).pos;
                    Vec3 c = LMeshGetVertex(e0->mesh, 2).pos;

                    Vec3 ab = Vec3Sub(b, a);
                    Vec3 ac = Vec3Sub(c, a);
                    Vec3 bc = Vec3Sub(c, b);
                    Vec3 f0 = Vec3Normalize(Vec3Cross(ab, ac));

                    // Bring other triangle's vertexes into e0's coordinate space.
                    Vec3 d = Vec3WTransform(LMeshGetVertex(e1->mesh, 0).pos, e1toe0m);
                    Vec3 e = Vec3WTransform(LMeshGetVertex(e1->mesh, 1).pos, e1toe0m);
                    Vec3 f = Vec3WTransform(LMeshGetVertex(e1->mesh, 2).pos, e1toe0m);

                    Vec3 de = Vec3Sub(e, d);
                    Vec3 df = Vec3Sub(f, d);
                    Vec3 ef = Vec3Sub(f, e);
                    Vec3 f1 = Vec3Normalize(Vec3Cross(de, df));

                    Vec3 norm_abde = Vec3Normalize(Vec3Cross(ab, de));
                    Vec3 norm_abdf = Vec3Normalize(Vec3Cross(ab, df));
                    Vec3 norm_abef = Vec3Normalize(Vec3Cross(ab, ef));

                    Vec3 norm_acde = Vec3Normalize(Vec3Cross(ac, de));
                    Vec3 norm_acdf = Vec3Normalize(Vec3Cross(ac, df));
                    Vec3 norm_acef = Vec3Normalize(Vec3Cross(ac, ef));

                    Vec3 norm_bcde = Vec3Normalize(Vec3Cross(bc, de));
                    Vec3 norm_bcdf = Vec3Normalize(Vec3Cross(bc, df));
                    Vec3 norm_bcef = Vec3Normalize(Vec3Cross(bc, ef));

                    Vec3 half_ab = Vec3Add(a, Vec3Scale(ab, 0.5f));
                    Vec3 half_ac = Vec3Add(a, Vec3Scale(ac, 0.5f));
                    Vec3 half_bc = Vec3Add(b, Vec3Scale(bc, 0.5f));

                    // NOTE: All drawing is done in e0 local coordinate space.
                    // Do not include translation in normal transform because those are direction vectors
                    // only.
                    {
                        // Draw vertexes of triangles.
                        {
                            Mat4 em = e0m;
                            Vec3 vs[] = {a, b, c, d, e, f};
                            for (u32 i = 0; i < ArraySizeOf(vs); ++i) {
                                Vec3 pos = vs[i];
                                Mat4 model = Mat4Multiply(em, Mat4Multiply(Mat4TranslateV(pos), Mat4Scale(0.06f)));
                                AddCmd(GetMesh(M_CUBE), model, LRED);
                            }
                        }
                        // Draw normals
                        {
                            {
                                Mat4 em = e0m;
                                Mat4 enormm = e0->orient;
                                Vec3 vs[] = {Vec3Lit(0.0f, 0.0f, 0.0f),
                                             Vec3WTransform(Vec3Zero(), e1toe0m),
                                             half_ab,
                                             half_ab,
                                             half_ab,
                                             half_ac,
                                             half_ac,
                                             half_ac,
                                             half_bc,
                                             half_bc};
                                Vec3 ns[] = {f0,        f1,        norm_abde, norm_abdf, norm_abef, norm_acde,
                                             norm_acdf, norm_acef, norm_bcde, norm_bcdf, norm_bcef};
                                for (u32 i = 0; i < ArraySizeOf(vs); ++i) {
                                    // Get rot matrix that will point mesh's forward along the normal. We get this
                                    // rotation by computing the rotation matrix of the normal direction. Note that we
                                    // need to calculate what the direction our mesh should point in upright space so we
                                    // concat the normal rotation matrix by the upright rotation matrix of the e0's
                                    // orientation.
                                    Vec3 norm = ns[i];
                                    Mat4 normrot = Mat4PointAt(norm);
                                    Mat4 orient_world = Mat4Multiply(enormm, normrot);

                                    // Need to take the model space position and figure out the world coords, that
                                    // means we need to use the entity model mat to transform into world space
                                    // (since it will correctly place the point based on scale, rot, translation).
                                    Vec3 pos_world = Vec3WTransform(vs[i], em);

                                    DrawArrow(ns[i], pos_world, orient_world, 0.3f, LRED);
                                }
                            }
                        }
                    }
                }

                DrawEntities(shader, entities, size);
            }

            // COLLISIONS Update.
            {
                EntityId* entities = COLLISIONS;
                u32 size = COLLISIONS_SIZE;

                for (u32 i = 0; i < size; ++i) {
                    Entity* e = GetEntity(entities[i]);
                    e->pos = Vec3Add(e->pos, Vec3Scale(e->v, delta));
                }

                for (u32 eidx = 0; eidx < size - 1; eidx += 2) {
                    Entity* e0 = GetEntity(entities[eidx + 0]);
                    Entity* e1 = GetEntity(entities[eidx + 1]);

                    Vec3 dir = Vec3Zero();
                    f32 dmin = 0.0f;
                    bool collides = CollideEntityEntity(e0, e1, &dir, &dmin);

                    if (collides) {
                        e0->v = Vec3Zero();
                        e1->v = Vec3Zero();
                        {
                            Vec3 arrow = Vec3Scale(dir, -1.0f);
                            Mat4 orient = Mat4PointAt(arrow);
                            DrawArrow(arrow, e0->pos, orient, 0.3f, LPURPLE);
                        }
                        {
                            Vec3 arrow = dir;
                            Mat4 orient = Mat4PointAt(arrow);
                            DrawArrow(arrow, e1->pos, orient, 0.3f, LBLACK);
                        }
                    } else {
                        {
                            Vec3 arrow = Vec3Scale(dir, -1.0f);
                            Mat4 orient = Mat4PointAt(arrow);
                            DrawArrow(arrow, e0->pos, orient, 0.3f, LGREEN);
                        }
                        {
                            Vec3 arrow = dir;
                            Mat4 orient = Mat4PointAt(arrow);
                            DrawArrow(arrow, e1->pos, orient, 0.3f, LGREEN);
                        }
                        // // Draw face normals.
                        // Entity* es[] = {e0, e1};
                        // for (u32 j = 0; j < ArraySizeOf(es); j++) {
                        //     Entity* e = es[j];
                        //     Mat4 model = EntityGetMat(e);
                        //     Mat4 rot = e->orient;
                        //     LMesh mid = e->mesh;
                        //     for (u32 i = 0; i < mid.ntriangles; i += 1) {
                        //         Triangle t = LMeshGetTriangle(mid, i);

                        //         Vec3 ab = Vec3Sub(t.b.pos, t.a.pos);
                        //         Vec3 ac = Vec3Sub(t.c.pos, t.a.pos);
                        //         Vec3 norm = Vec3Cross(ab, ac);

                        //         Vec3 center = Vec3Scale(Vec3Add(t.a.pos, Vec3Add(t.b.pos, t.c.pos)), 1.0f / 3.0f);
                        //         center = Vec3WTransform(center, model);

                        //         Mat4 orient = Mat4PointAt(norm);
                        //         orient = Mat4Multiply(rot, orient);
                        //         DrawArrow(norm, center, orient, 0.3f, LRED);
                        //     }
                        // }
                    }
                }

                DrawEntities(shader, entities, size);
            }

            // PYRAMIDS Draw.
            {
                EntityId* entities = PYRAMIDS;
                u32 size = PYRAMIDS_SIZE;
                for (u32 i = 0; i < size; ++i) {
                    Entity* entity = GetEntity(entities[i]);
                    entity->orient = Mat4Rotate(gstate->timer, Vec3Lit(1.0f, 0.3f, 0.5f));
                }
                DrawEntities(shader, entities, size);
            }

            // Walls.
            {
                EntityId* entities = WALLS;
                u32 size = WALLS_SIZE;
                DrawEntities(shader, entities, size);
            }

            if (gstate->debug) {
                DrawFPS(5, 5);
            }
        }
    }

    // Drawing
    {
        // Begin.
        {
            BeginDrawing();
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindVertexArray(VAO);
        }

        Mat4 view = Mat4Invert(Mat4Multiply(Mat4TranslateV(viewpos), Mat4RotateEuler(vieworient)));
        // Keep this around for debugging for a bit.
        // Mat4 vieworientm = Mat4RotateEuler(vieworient);
        // Vec3 viewforward = Vec3Transform(Vec3Lit(0.0f, 0.0f, -1.0f), vieworientm);
        // // // Vec3 viewforward = Vec3Lit(f32cos(vieworient.yaw) * f32cos(vieworient.pitch), f32sin(vieworient.pitch),
        // // //                            -f32sin(vieworient.yaw) * f32cos(vieworient.pitch));
        // Mat4 view2 = Mat4LookAt(viewpos, Vec3Add(viewpos, viewforward), Vec3Lit(0.0f, 1.0f, 0.0f));
        // Assert(Mat4Eq(view, view2));
        Mat4 proj = Mat4Perspective(Deg2Rad(45.0f), (f32)GetScreenWidth() / (f32)GetScreenHeight(), 0.1f, 1000.0f);

        // Enable/Disable culling.
        // TODO: Make this nicer at some point. Useful for debugging.
        {
            if (gstate->debug) {
                glDisable(GL_CULL_FACE);
            } else {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glFrontFace(GL_CCW);
            }
        }

        // Use shader + setup norm + view + proj matrix.
        LShader shader = GetShader();
        {
            glUseProgram(shader.pid);
            {
                // Calculate the sun direction in view space (only factoring in rotation).
                Vec3 lightdir = Vec3Lit(-0.2f, -1.0f, -0.3f);
                // Vec3 lightdir = Vec3Lit(0.0f, 0.0f, -1.0f);
                Mat4 norm = Mat4Transpose(Mat4Invert(view));
                norm.v.d = Vec3Zero();
                Vec3 lightdir_eye = Vec3Transform(lightdir, norm);
                glUniform4f(shader.flightdirin_loc, lightdir_eye.x, lightdir_eye.y, lightdir_eye.z, 0.0f);
            }
            glUniform4f(shader.flight_color_loc, 1.0f, 1.0f, 1.0f, 1.0f);
            glUniformMatrix4fv(shader.vview_loc, 1, GL_FALSE, (const f32*)&view.m);
            glUniformMatrix4fv(shader.vproj_loc, 1, GL_FALSE, (const f32*)&proj.m);
        }

        for (u32 i = 0; i < CBUFFER_SIZE; ++i) {
            DrawCmd* cmd = &CBUFFER[i];
            glUniformMatrix4fv(shader.vmodel_loc, 1, GL_FALSE, (const f32*)&cmd->modelm.m);
            glUniform4fv(shader.vucolor_loc, 1, (const f32*)&cmd->color.f);
            glUniform1f(shader.vcolor_blend_loc, cmd->color.a != 0.0f ? 1.0f : 0.0f);
            if (cmd->wireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }
            glDrawElements(GL_TRIANGLES, cmd->meshid.ntriangles * 3, GL_UNSIGNED_INT,
                           (void*)(cmd->meshid.ibufferidx * sizeof(GLuint)));
            if (cmd->wireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
        }

        EndDrawing();
        CmdBufferZero();  // Maybe consider moving this to be at the top of update func.
    }
}

////////////////////////////////////////////////////////////

static void TestL();

int main(void) {
    TestL();

    GameStateInit(&GState);

    while (!WindowShouldClose()) {
        float delta = GetFrameTime();
        GameStateUpdate(&GState, delta);
        GameStateDraw(&GState, delta);
    }

    CloseWindow();

    return 0;
}

////////////////////////////////////////////////////////////

static bool TestFail = false;

static void TAssert(bool result, const char* msg) {
    if (!result) {
        TraceLog(LOG_ERROR, "Failed: %s", msg);
    }

    TestFail |= !result;
}

static void TestMath() {
    // Test Vec2.
    {
        TAssert(Vec2Eq(Vec2Lit(1.0f, 1.0f), Vec2Lit(1.0f, 1.0f)), "Vec2Eq");
        TAssert(Vec2Len(Vec2Lit(1.0f, 0.0)) == 1.0f, "Vec2Len");
        TAssert(Vec2Len(Vec2Zero()) == 0.0f, "Vec2Len Zero");
        TAssert(Vec2Eq(Vec2Add(Vec2Zero(), Vec2Lit(1.0f, 1.0f)), Vec2Lit(1.0f, 1.0f)), "Vec2Add");
        TAssert(Vec2Eq(Vec2Sub(Vec2Zero(), Vec2Lit(1.0f, 1.0f)), Vec2Lit(-1.0f, -1.0f)), "Vec2Sub");
        TAssert(Vec2Eq(Vec2Scale(Vec2Lit(1.0f, 1.0f), 2.0f), Vec2Lit(2.0f, 2.0f)), "Vec2Scale");
        TAssert(Vec2Eq(Vec2Clamp(Vec2Zero(), Vec2Zero(), Vec2Zero()), Vec2Zero()), "Vec2Clamp");
        TAssert(f32eq(Vec2Dot(Vec2Lit(1.0f, 0.0f), Vec2Lit(1.0f, 0.0f)), 1.0f), "Vec2Dot");
        TAssert(Vec2Eq(Vec2Normalize(Vec2Lit(3.0f, 0.0f)), Vec2Lit(1.0f, 0.0f)), "Vec2Normalize");
        TAssert(Vec2Eq(Vec2Cross(Vec2Lit(1.0f, 0.0f)), Vec2Lit(0.0f, -1.0f)), "Vec2Cross");
    }

    // Test Vec3.
    {
        TAssert(Vec3EqPro(Vec3Lit(1.0f, 1.0f, 1.0f), Vec3Lit(1.0f, 1.0f, 1.0f), 0.001), "Vec3EqPro");
        TAssert(Vec3Eq(Vec3Lit(1.0f, 1.0f, 0.9998f), Vec3Lit(1.0f, 1.0f, 1.0f)), "Vec3EqPro approx");

        TAssert(f32eq(Vec3Len(Vec3Lit(0.0f, 0.0f, 0.0f)), 0.0f), "Vec3Len 0");
        char* msg[] = {"Vec3Len x", "Vec3Len y", "Vec3Len z"};
        for (u32 i = 0; i < 3; ++i) {
            Vec3 v = Vec3Zero();
            v.f[i] = 1.0f;
            TAssert(f32eq(Vec3Len(v), 1.0f), msg[i]);
        }

        TAssert(Vec3Eq(Vec3Normalize(Vec3Lit(0.0f, 0.0f, 0.0f)), Vec3Zero()), "Vec3Normalize 0");
        TAssert(Vec3Eq(Vec3Normalize(Vec3Lit(0.5f, 0.0f, 0.0f)), Vec3Lit(1.0f, 0.0f, 0.0f)), "Vec3Normalize x");
        TAssert(Vec3Eq(Vec3Normalize(Vec3Lit(0.0f, 199.0f, 0.0f)), Vec3Lit(0.0f, 1.0f, 0.0f)), "Vec3Normalize y");
        TAssert(Vec3Eq(Vec3Normalize(Vec3Lit(0.0f, 0.0f, 1.0f)), Vec3Lit(0.0f, 0.0f, 1.0f)), "Vec3Normalize z");

        TAssert(f32eq(Vec3Dot(Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f)), 0.0f), "Vec3Dot |_");
        TAssert(f32eq(Vec3Dot(Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 0.0f, 1.0f)), 1.0f), "Vec3Dot ||");
        TAssert(f32eq(Vec3Dot(Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 0.0f, -1.0f)), -1.0f), "Vec3Dot neg ||");

        TAssert(f32eq(Vec3Angle(Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 0.0f, 1.0f)), 0.0f), "Vec3Angle ||");

        TAssert(Vec3Eq(Vec3RotateAxis(Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(1.0f, 0.0f, 0.0f), Deg2Rad(-90.0f)),
                       Vec3Lit(0.0f, 1.0f, 0.0f)),
                "Vec3RotateAxis z->y");

        // Vec3Cross
        {
            TAssert(Vec3Eq(Vec3Cross(Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f)), Vec3Lit(0.0f, 0.0f, 1.0f)),
                    "Vec3Cross X x Y");
            TAssert(Vec3Eq(Vec3Cross(Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f)), Vec3Lit(0.0f, 0.0f, -1.0f)),
                    "Vec3Cross Y x X");
            TAssert(Vec3Eq(Vec3Cross(Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f)), Vec3Lit(0.0f, -1.0f, 0.0f)),
                    "Vec3Cross X x Z");

            // Cross product of two parallel vectors should result in a zero vector.
            TAssert(Vec3Eq(Vec3Cross(Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(2.0f, 0.0f, 0.0f)), Vec3Zero()),
                    "Vec3Cross Parallel X");

            // Cross product of two opposite vectors should result in a zero vector.
            TAssert(Vec3Eq(Vec3Cross(Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(-1.0f, 0.0f, 0.0f)), Vec3Zero()),
                    "Vec3Cross Opposite X");

            // Cross product of arbitrary vectors.
            TAssert(
                Vec3Eq(Vec3Cross(Vec3Lit(1.0f, 2.0f, 3.0f), Vec3Lit(4.0f, 5.0f, 6.0f)), Vec3Lit(-3.0f, 6.0f, -3.0f)),
                "Vec3Cross Arbitrary");
        }
    }

    // Test Mat4
    {
        {
            Mat4 m = Mat4Lit(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
            TAssert(Mat4Eq(MatrixToMat4(Mat4ToRMatrix(m)), m), "Mat4 to Matrix");
        }
        // Mat4ScaleXYZ
        {
            // Test uniform scaling.
            Mat4 scaleUniform = Mat4ScaleXYZ(2.0f, 2.0f, 2.0f);
            Mat4 expectedUniform =
                Mat4Lit(2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(scaleUniform, expectedUniform), "Mat4ScaleXYZ Uniform");

            // Test non-uniform scaling.
            Mat4 scaleNonUniform = Mat4ScaleXYZ(3.0f, 2.0f, 1.0f);
            Mat4 expectedNonUniform =
                Mat4Lit(3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(scaleNonUniform, expectedNonUniform), "Mat4ScaleXYZ Non-Uniform");

            // Test identity scaling (scaling by 1 on all axes).
            Mat4 scaleIdentity = Mat4ScaleXYZ(1.0f, 1.0f, 1.0f);
            Mat4 expectedIdentity =
                Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(scaleIdentity, expectedIdentity), "Mat4ScaleXYZ Identity");

            // Test scaling with zero on one axis.
            Mat4 scaleZeroX = Mat4ScaleXYZ(0.0f, 1.0f, 1.0f);
            Mat4 expectedZeroX =
                Mat4Lit(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(scaleZeroX, expectedZeroX), "Mat4ScaleXYZ Zero X");
        }

        // Test Mat4TranslateV
        {
            // Test translating by a positive vector.
            Vec3 translatePos = Vec3Lit(3.0f, 4.0f, 5.0f);
            Mat4 translationMatrixPos = Mat4TranslateV(translatePos);
            Mat4 expectedPos =
                Mat4Lit(1.0f, 0.0f, 0.0f, 3.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f, 0.0f, 1.0f, 5.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(translationMatrixPos, expectedPos), "Mat4TranslateV Positive Vector");

            // Test translating by a negative vector.
            Vec3 translateNeg = Vec3Lit(-1.0f, -2.0f, -3.0f);
            Mat4 translationMatrixNeg = Mat4TranslateV(translateNeg);
            Mat4 expectedNeg = Mat4Lit(1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -2.0f, 0.0f, 0.0f, 1.0f, -3.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(translationMatrixNeg, expectedNeg), "Mat4TranslateV Negative Vector");

            // Test translating by a zero vector (no translation).
            Vec3 translateZero = Vec3Lit(0.0f, 0.0f, 0.0f);
            Mat4 translationMatrixZero = Mat4TranslateV(translateZero);
            Mat4 expectedZero =
                Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(translationMatrixZero, expectedZero), "Mat4TranslateV Zero Vector");

            // Test translating by a large vector.
            Vec3 translateLarge = Vec3Lit(1000.0f, 2000.0f, 3000.0f);
            Mat4 translationMatrixLarge = Mat4TranslateV(translateLarge);
            Mat4 expectedLarge = Mat4Lit(1.0f, 0.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 0.0f, 2000.0f, 0.0f, 0.0f, 1.0f,
                                         3000.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(translationMatrixLarge, expectedLarge), "Mat4TranslateV Large Vector");
        }

        // Mat4RotateX
        {
            // 1. Test 90-degree rotation (π/2 radians) around the X-axis
            Mat4 rotation90 = Mat4RotateX(LPI / 2.0f);
            Mat4 expected90 = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(rotation90, expected90), "Mat4RotateX 90 degrees");

            // 2. Test 180-degree rotation (π radians) around the X-axis
            Mat4 rotation180 = Mat4RotateX(LPI);
            Mat4 expected180 = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(rotation180, expected180), "Mat4RotateX 180 degrees");

            // 3. Test 0-degree rotation (0 radians), should produce the identity matrix
            Mat4 rotation0 = Mat4RotateX(0.0f);
            Mat4 expected0 =
                Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(rotation0, expected0), "Mat4RotateX 0 degrees");

            // 4. Test 33-degree rotation (approx. 0.57596 radians) around the X-axis
            f32 angle33 = 33.0f * (LPI / 180.0f);
            Mat4 rotation33 = Mat4RotateX(angle33);
            Mat4 expected33 = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.83867f, -0.54464f, 0.0f, 0.0f, 0.54464f, 0.83867f,
                                      0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(rotation33, expected33), "Mat4RotateX 33 degrees");
        }

        // Mat4RotateY
        {
            // Test 1: Rotate by 0 degrees (identity matrix)
            Mat4 result0 = Mat4RotateY(0.0f);
            Mat4 expected0 =
                Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result0, expected0));

            // Test 2: Rotate by 90 degrees around Y-axis
            Mat4 result90 = Mat4RotateY(LPI / 2);
            Mat4 expected90 = Mat4Lit(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result90, expected90));

            // Test 3: Rotate by 180 degrees around Y-axis
            Mat4 result180 = Mat4RotateY(LPI);
            Mat4 expected180 = Mat4Lit(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result180, expected180));

            // Test 4: Rotate by 33 degrees around Y-axis
            float radians33 = 33.0f * (LPI / 180.0f);  // Convert degrees to radians
            Mat4 result33 = Mat4RotateY(radians33);
            Mat4 expected33 = Mat4Lit(0.83867f, 0.0f, 0.54464f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -0.54464f, 0.0f, 0.83867f,
                                      0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result33, expected33));
        }

        // Mat4RotateZ
        {
            // Test 1: Rotate by 0 degrees (identity matrix)
            Mat4 result0 = Mat4RotateZ(0.0f);
            Mat4 expected0 =
                Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result0, expected0));

            // Test 2: Rotate by 90 degrees around Z-axis
            Mat4 result90 = Mat4RotateZ(LPI / 2);
            Mat4 expected90 = Mat4Lit(0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result90, expected90));

            // Test 3: Rotate by 180 degrees around Z-axis
            Mat4 result180 = Mat4RotateZ(LPI);
            Mat4 expected180 = Mat4Lit(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result180, expected180));

            // Test 4: Rotate by 33 degrees around Z-axis
            float radians33 = 33.0f * (LPI / 180.0f);  // Convert degrees to radians
            Mat4 result33 = Mat4RotateZ(radians33);
            Mat4 expected33 = Mat4Lit(0.83867f, -0.54464f, 0.0f, 0.0f, 0.54464f, 0.83867f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                      0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result33, expected33));
        }

        // Mat4RotateAxis
        {
            // Test 1: Rotate by 0 degrees around an arbitrary axis (should return the identity matrix)
            Vec3 axis1 = Vec3Lit(1.0f, 1.0f, 1.0f);
            Mat4 result0 = Mat4Rotate(0.0f, axis1);
            Mat4 expected0 =
                Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result0, expected0));

            // Test 2: Rotate by 90 degrees around the X-axis
            Vec3 axis2 = Vec3Lit(1.0f, 0.0f, 0.0f);
            Mat4 result90 = Mat4Rotate(M_PI / 2, axis2);
            Mat4 expected90 = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result90, expected90));

            // Test 3: Rotate by 180 degrees around the Y-axis
            Vec3 axis3 = Vec3Lit(0.0f, 1.0f, 0.0f);
            Mat4 result180 = Mat4Rotate(M_PI, axis3);
            Mat4 expected180 = Mat4Lit(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result180, expected180));

            // Test 4: Rotate by 33 degrees around an arbitrary axis (1, 1, 1)
            Vec3 axis4 = Vec3Lit(1.0f, 1.0f, 1.0f);
            float radians33 = 33.0f * (M_PI / 180.0f);  // Convert degrees to radians
            Mat4 result33 = Mat4Rotate(radians33, axis4);
            Mat4 expected33 =
                Mat4Lit(0.89244705f, -0.26067102f, 0.36822397f, 0.0f, 0.36822397f, 0.89244705f, -0.26067102f, 0.0f,
                        -0.26067102f, 0.36822397f, 0.89244705f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result33, expected33));
        }

        // Mat4Multiply
        {
            // Test case 1: Multiplying with identity matrix
            Mat4 identity = Mat4Identity();
            Mat4 matrix = Mat4Lit(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f,
                                  14.0f, 15.0f, 16.0f);
            Mat4 result1 = Mat4Multiply(matrix, identity);
            TAssert(Mat4Eq(result1, matrix), "Mat4Multiply - Multiplying with identity should return the same matrix");

            // Test case 2: Multiplying two arbitrary matrices
            Mat4 a = Mat4Lit(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f,
                             15.0f, 16.0f);
            Mat4 b = Mat4Lit(16.0f, 15.0f, 14.0f, 13.0f, 12.0f, 11.0f, 10.0f, 9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f,
                             2.0f, 1.0f);
            Mat4 expected2 = Mat4Lit(80.0f, 70.0f, 60.0f, 50.0f, 240.0f, 214.0f, 188.0f, 162.0f, 400.0f, 358.0f, 316.0f,
                                     274.0f, 560.0f, 502.0f, 444.0f, 386.0f);
            Mat4 result2 = Mat4Multiply(a, b);
            TAssert(Mat4Eq(result2, expected2), "Mat4Multiply - Multiplying two arbitrary matrices");

            // Test case 3: Multiplying by a scaling matrix
            Mat4 scale = Mat4ScaleXYZ(2.0f, 3.0f, 4.0f);
            Mat4 expected3 = Mat4Lit(2.0f, 6.0f, 12.0f, 4.0f, 10.0f, 18.0f, 28.0f, 8.0f, 18.0f, 30.0f, 44.0f, 12.0f,
                                     26.0f, 42.0f, 60.0f, 16.0f);
            Mat4 result3 = Mat4Multiply(matrix, scale);
            TAssert(Mat4Eq(result3, expected3), "Mat4Multiply - Multiplying by a scaling matrix");

            // Test case 4: Multiplying by a rotation matrix (e.g., a 90-degree rotation around Z-axis)
            Mat4 rotation = Mat4RotateZ(1.5708f);  // ~90 degrees in radians
            Mat4 initial =
                Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            Mat4 expected4 = Mat4Lit(0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f);
            Mat4 result4 = Mat4Multiply(initial, rotation);
            TAssert(Mat4Eq(result4, expected4),
                    "Mat4Multiply - Multiplying by a 90-degree rotation matrix around Z-axis");
        }

        // Mat4Perspective
        {
            // Test case 1: Basic perspective matrix with 45-degree FOV and 16:9 aspect ratio
            Mat4 result1 = Mat4Perspective(Deg2Rad(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
            Mat4 expected1 = Mat4Lit(1.3579952f, 0.0f, 0.0f, 0.0f, 0.0f, 2.4142137f, 0.0f, 0.0f, 0.0f, 0.0f, -1.002002f,
                                     -0.2002002f, 0.0f, 0.0f, -1.0f, 0.0f);
            TAssert(Mat4Eq(result1, expected1), "Mat4Perspective - 45-degree FOV, 16:9 aspect ratio");

            // Test case 2: Perspective matrix with square aspect ratio (45-degree FOV, 1:1 aspect ratio)
            Mat4 result2 = Mat4Perspective(Deg2Rad(45.0f), 1.0f, 0.1f, 100.0f);
            Mat4 expected2 = Mat4Lit(2.4142137f, 0.0f, 0.0f, 0.0f, 0.0f, 2.4142137f, 0.0f, 0.0f, 0.0f, 0.0f, -1.002002f,
                                     -0.2002002f, 0.0f, 0.0f, -1.0f, 0.0f);
            TAssert(Mat4Eq(result2, expected2), "Mat4Perspective - 45-degree FOV, 1:1 aspect ratio");

            // Test case 3: Perspective matrix with wide aspect ratio (60-degree FOV, 21:9 aspect ratio)
            Mat4 result3 = Mat4Perspective(Deg2Rad(60.0f), 21.0f / 9.0f, 0.1f, 100.0f);
            Mat4 expected3 = Mat4Lit(0.7423, 0.0f, 0.0f, 0.0f, 0.0f, 1.73205f, 0.0f, 0.0f, 0.0f, 0.0f, -1.002002f,
                                     -0.2002002f, 0.0f, 0.0f, -1.0f, 0.0f);
            TAssert(Mat4Eq(result3, expected3), "Mat4Perspective - 60-degree FOV, 21:9 aspect ratio");

            // Test case 4: Perspective matrix with small near and large far planes (30-degree FOV, 4:3 aspect ratio)
            Mat4 result4 = Mat4Perspective(Deg2Rad(30.0f), 4.0f / 3.0f, 0.01f, 1000.0f);
            Mat4 expected4 = Mat4Lit(2.79904f, 0.0f, 0.0f, 0.0f, 0.0f, 3.73205f, 0.0f, 0.0f, 0.0f, 0.0f, -1.00002f,
                                     -0.0200002f, 0.0f, 0.0f, -1.0f, 0.0f);
            TAssert(Mat4Eq(result4, expected4),
                    "Mat4Perspective - 30-degree FOV, 4:3 aspect ratio, small near/large far");
        }

        // Mat4LookAt
        {
            // Test Case 1: Simple Case - Looking down the -Z axis from the origin
            Vec3 eye1 = Vec3Lit(0.0f, 0.0f, 0.0f);
            Vec3 target1 = Vec3Lit(0.0f, 0.0f, -1.0f);
            Vec3 up1 = Vec3Lit(0.0f, 1.0f, 0.0f);
            Mat4 expected1 = Mat4Identity();
            TAssert(Mat4Eq(Mat4LookAt(eye1, target1, up1), expected1), "Mat4LookAt look down -z from origin");

            // Test Case 2: Offset Eye Position - Looking at the origin from (0, 0, 5)
            Vec3 eye2 = Vec3Lit(0.0f, 0.0f, 5.0f);
            Vec3 target2 = Vec3Lit(0.0f, 0.0f, 0.0f);
            Vec3 up2 = Vec3Lit(0.0f, 1.0f, 0.0f);
            Mat4 expected2 = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, -5.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(Mat4LookAt(eye2, target2, up2), expected2),
                    "Mat4LookAt Looking at the origin from (0, 0, 5)");

            // Test Case 3: Tilted Up Vector - Looking at the origin from (5, 5, 5) with a tilted up vector
            Vec3 eye3 = Vec3Lit(5.0f, 5.0f, 5.0f);
            Vec3 target3 = Vec3Lit(0.0f, 0.0f, 0.0f);
            Vec3 up3 = Vec3Lit(0.0f, 1.0f, 0.0f);
            Mat4 expected3 = Mat4Lit(0.707107f, 0.0f, -0.707107f, 0.0f, -0.408248f, 0.816497f, -0.408248f, 0.0f,
                                     0.57735f, 0.57735f, 0.57735f, -8.66025f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(Mat4LookAt(eye3, target3, up3), expected3),
                    "Mat4LookAt Looking at the origin from (5, 5, 5) with a tilted up vector");
        }

        // Mat4Rotate x Mat4Multiply
        {
            f32 target = Deg2Rad(90.0f);
            Vec3 axis = Vec3Lit(0.0f, -1.0f, 0.0f);
            Mat4 orient = Mat4Identity();
            Mat4 expected = Mat4Lit(0.0f, 0.0f, -1.0f, 0.0f,  //
                                    0.0f, 1.0f, 0.0f, 0.0f,   //
                                    1.0f, 0.0f, 0.0f, 0.0f,   //
                                    0.0f, 0.0f, 0.0f, 1.0f);
            f32 step = target / 128.0f;
            f32 rotation = 0.0f;
            for (; rotation + step <= target; rotation += step) {
                Mat4 r = Mat4Rotate(step, axis);
                orient = Mat4Multiply(r, orient);
            }

            f32 rest = target - rotation;
            if (rest > 0.0f) {
                Mat4 r = Mat4Rotate(rest, axis);
                orient = Mat4Multiply(r, orient);
            }
            TAssert(Mat4Eq(orient, expected), "Integration Turn Left");
        }
    }
}

static void TestL() {
    TraceLog(LOG_INFO, "Running Tests");
    TestMath();

    if (TestFail) {
        TraceLog(LOG_ERROR, "Failed tests!");
        exit(1);
    }

    TraceLog(LOG_INFO, "Tests Pass");
}
