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
#define f32lerp(s, e, t) ((s) + (((e) - (s)) * (t)))

static f32 f32clamp(f32 v, f32 min, f32 max) { return f32min(f32max(v, min), max); }

typedef struct Vec2 {
    union {
        struct {
            f32 x;
            f32 y;
        };

        f32 f[2];
    };
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
            f32 yaw;    // rotation about up (y) axis.
            f32 pitch;  // rotation about (x) axis
            f32 roll;   // rotation about z axis.
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
// formula v1 - v2
static Vec3 Vec3Sub(Vec3 v1, Vec3 v2) { return Vec3Lit(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z); }
static Vec3 Vec3Scale(Vec3 v, f32 n) { return Vec3Lit(v.x * n, v.y * n, v.z * n); }
// Inverts a vector.
static Vec3 Vec3Negate(Vec3 v) { return Vec3Lit(-1.0f * v.x, -1.0f * v.y, -1.0f * v.z); }
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
static Vec3 Vec3Lerp(Vec3 start, Vec3 end, f32 t) {
    return Vec3Lit(f32lerp(start.x, end.x, t), f32lerp(start.y, end.y, t), f32lerp(start.z, end.z, t));
}

static f32 Vec3Len(Vec3 v) { return f32sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z)); }
static f32 Vec3Dot(Vec3 v1, Vec3 v2) { return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z); }
// Returns the angle between a and b in radians. Assumes a and b are normalized.
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

static Vec3 Vec3Project(Vec3 v, Vec3 axis) { return Vec3Scale(axis, Vec3Dot(v, axis)); }

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

static Mat4 Mat4ScaleV(Vec3 v) {
    return Mat4Lit(v.x, 0.0f, 0.0f, 0.0f,  //
                   0.0f, v.y, 0.0f, 0.0f,  //
                   0.0f, 0.0f, v.z, 0.0f,  //
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

// Contstruct matrix that reflects vectors around the plane defined by n. n doesn't need to be unit.
// useful for 3d reflections. see FOGB 2.3
static Mat4 Mat4Reflect(Vec3 n) {
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

static Mat4 Mat4Perspective(rad fov, f32 aspect, f32 near, f32 far) {
    f32 zoom = 1.0f / (f32)f32tan(fov * 0.5f);
    f32 depth = far - near;
    return Mat4Lit(zoom / aspect, 0.0f, 0.0f, 0.0f,                                  //
                   0.0f, zoom, 0.0f, 0.0f,                                           //
                   0.0f, 0.0f, -(far + near) / depth, (-2.0f * far * near) / depth,  //
                   0.0f, 0.0f, -1.0f, 0.0f);
}

// FoGED 6.3.4
// maps (left, bottom) -> (-1, -1)
static Mat4 Mat4Ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    f32 w_inv = 1.0f / (right - left);
    f32 h_inv = 1.0f / (bottom - top);
    f32 d_inv = 1.0f / (far - near);
    return Mat4Lit(2.0f * w_inv, 0.0f, 0.0f, -(right + left) * w_inv,  //
                   0.0f, 2.0f * h_inv, 0.0f, -(bottom + top) * h_inv,  //
                   0.0f, 0.0f, d_inv, -near * d_inv,                   //
                   0.0f, 0.0f, 0.0f, 1.0f);
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

static Mat4 Mat4SVRT(Vec3 scale, Mat4 rotate, Vec3 pos) {
    return Mat4Multiply(Mat4TranslateV(pos), Mat4Multiply(rotate, Mat4ScaleV(scale)));
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

static const Vec4 LCLEAR = Vec4Lit(1.0f, 1.0f, 1.0f, 1.0f);
static const Vec4 LRED = Vec4Lit(1.0f, 0.0f, 0.0f, 1.0f);
static const Vec4 LGREEN = Vec4Lit(0.0f, 1.0f, 0.0f, 1.0f);
static const Vec4 LBLUE = Vec4Lit(0.0f, 0.0f, 1.0f, 1.0f);
static const Vec4 LBLACK = Vec4Lit(0.0f, 0.0f, 0.0f, 1.0f);
static const Vec4 LPURPLE = Vec4Lit(1.0f, 0.0f, 1.0f, 1.0f);
static const Vec4 LORANGE = Vec4Lit(1.0f, 0.5f, 0.0f, 1.0f);
static const Vec4 LGRAY = Vec4Lit(199.0f / 255.0f, 197.0f / 255.0f, 198.0f / 255.0f, 0.16f);
static const Vec4 LYELLOW = Vec4Lit(1.0f, 1.0f, 0.0f, 1.0f);

#define RGBALit(r, g, b, a) (Vec4Lit((r) / 255.0f, (g) / 255.0f, (b) / 255.0f, (a)))
#define RGBLit(r, g, b) (RGBALit((r), (g), (b), 1.0f))

////////////////////////////////////////////////////////////
// S:Render Data

typedef struct LShader {
    u32 pid;  // program id.
    u32 vid;
    u32 fid;

    // vert shader.
    u32 vpos_loc;
    u32 vnormal_loc;
    u32 vmodel_loc;
    u32 vview_loc;
    u32 vproj_loc;
    u32 vtextcoords_loc;
    u32 vucolor_loc;
    u32 vuvoffset_loc;
    u32 vuvscale_loc;

    // frag shader.
} LShader;

static void LShaderInit(LShader* shader, const char* vsrc, const char* fsrc) {
    char error[1024] = {0};
    u32 size = ArraySizeOf(error);

    // vertex shader.
    {
        i32 success = 0;
        shader->vid = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(shader->vid, 1, &vsrc, NULL);
        glCompileShader(shader->vid);
        glGetShaderiv(shader->vid, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader->vid, size, NULL, error);
            TraceLog(LOG_ERROR, "Error compiling vertex shader: %s", error);
            Assert(false);
        }
    }

    // fragment shader.
    {
        i32 success = 0;
        shader->fid = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(shader->fid, 1, &fsrc, NULL);
        glCompileShader(shader->fid);
        glGetShaderiv(shader->fid, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader->fid, size, NULL, error);
            TraceLog(LOG_ERROR, "Error compiling fragment shader: %s", error);
            Assert(false);
        }
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
        "layout (location = 2) in vec3 vNormal;\n"
        "\n"
        "uniform mat4 vModel;\n"
        "uniform mat4 vView;\n"
        "uniform mat4 vProj;\n"
        "\n"
        "uniform vec4 vUColor;\n"
        "\n"
        "out vec4 fColor;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);\n"
        "   fColor = vUColor;\n"
        // TODO: This is hacky and doesn't work in all cases (e.g. non uniform scale). come up with
        // more robust way to do this. also consider moving this calc to CPU.
        // "   fNormal = mat3(transpose(inverse(vView * vModel))) * vNormal;\n"
        "}\0";

    static const char* FSRC =
        "#version 330 core\n"
        "\n"
        "in vec4 fColor;\n"
        "\n"
        "out vec4 FragColor;\n"
        "\n"
        "void main()\n"
        "{\n"
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

            SHADER.vmodel_loc = glGetUniformLocation(SHADER.pid, "vModel");
            SHADER.vview_loc = glGetUniformLocation(SHADER.pid, "vView");
            SHADER.vproj_loc = glGetUniformLocation(SHADER.pid, "vProj");
            SHADER.vucolor_loc = glGetUniformLocation(SHADER.pid, "vUColor");
        }
    }

    return SHADER;
}

static LShader GetShader2D() {
    static const char* VSRC =
        "#version 330 core\n"
        "layout (location = 0) in vec3 vPos;\n"
        "layout (location = 1) in vec2 vTextCoords;\n"
        "\n"
        "uniform mat4 vModel;\n"
        "uniform mat4 vProj;\n"
        "\n"
        "uniform vec4 vUColor;\n"
        "uniform vec2 vUVOffset;\n"
        "uniform vec2 vUVScale;\n"
        "\n"
        "out vec4 fColor;\n"
        "out vec2 fTextCoords;\n"
        "void main()\n"
        "{\n"
        "   fTextCoords = vUVOffset + (vUVScale * vTextCoords);\n"
        // TODO: Incorporate camera at some point.
        "   gl_Position = vProj * vModel * vec4(vPos.x, vPos.y, 0.0, 1.0);\n"
        "   fColor = vUColor;\n"
        "}\0";

    static const char* FSRC =
        "#version 330 core\n"
        "\n"
        "in vec4 fColor;\n"
        "in vec2 fTextCoords;\n"
        "\n"
        "uniform sampler2D texture0;\n"
        "\n"
        "out vec4 FragColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    FragColor = fColor * texture(texture0, fTextCoords);\n"
        // "   FragColor = fColor;\n"
        // useful for debug.
        // "    FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
        "}\0";
    static bool INIT = false;
    static LShader SHADER = {0};

    if (!INIT) {
        INIT = true;
        LShaderInit(&SHADER, VSRC, FSRC);

        // Att lookup.
        {
            SHADER.vpos_loc = glGetAttribLocation(SHADER.pid, "vPos");
            SHADER.vtextcoords_loc = glGetAttribLocation(SHADER.pid, "vTextCoords");

            SHADER.vmodel_loc = glGetUniformLocation(SHADER.pid, "vModel");
            SHADER.vproj_loc = glGetUniformLocation(SHADER.pid, "vProj");
            SHADER.vucolor_loc = glGetUniformLocation(SHADER.pid, "vUColor");
            SHADER.vuvoffset_loc = glGetUniformLocation(SHADER.pid, "vUVOffset");
            SHADER.vuvscale_loc = glGetUniformLocation(SHADER.pid, "vUVScale");
        }
    }

    return SHADER;
}

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
    Vec3 pos;
    Vec3 normal;
} Vertex;

typedef struct Triangle {
    union {
        u32 vs[3];
        struct {
            u32 va;
            u32 vb;
            u32 vc;
        };
    };
} Triangle;

#define TriLit(n0, n1, n2) ((Triangle){.va = (n0), .vb = (n1), .vc = (n2)})

#define MAX_VERTICES 512

static u32 VAO = 0;
static u32 VBO = 0;
static u32 VIO = 0;  // Index buffer.

static Vertex VBUFFER[MAX_VERTICES] = {};  // Vertex buffer
static u32 VBUFFER_SIZE = 0;

#define MAX_TRIANGLES 2048
static u32 IBUFFER[MAX_TRIANGLES] = {};  // Index buffer
static u32 IBUFFER_SIZE = 0;

typedef struct LMesh {
    u32 ibufferidx;  // Offset into index buffer
    u32 ntriangles;
} LMesh;

#define MAX_MESHES 256
static LMesh MESHES[MAX_MESHES] = {0};
static u32 MESHES_SIZE = 0;

typedef enum MID {
    M_CUBE = 0,
    M_QUAD,
    M_ISO_TRIANGLE,
    M_PYRAMID,
    M_2S_ISO_TRIANGLE,
    M_ICOSAHEDRON,
    M_ICOSPHERE,
    M_COUNT
} MID;

typedef struct LTexture {
    u32 id;
    i32 width;
    i32 height;
} LTexture;

static LMesh GetMesh(u32 mid) {
    Assert(mid < MAX_MESHES);
    return MESHES[mid];
}

static void AddMesh(Vec3* vs, u32 vsize, Triangle* ts, u32 tsize) {
    u32 voffset = VBUFFER_SIZE;
    u32 ioffset = IBUFFER_SIZE;

    Assert(VBUFFER_SIZE + vsize <= MAX_VERTICES);
    Assert(IBUFFER_SIZE + (tsize * 3) <= MAX_TRIANGLES);

    Vec3 ns[MAX_VERTICES] = {0};
    for (u32 i = 0; i < tsize; ++i) {
        Triangle t = ts[i];
        Vec3 a = vs[t.va];
        Vec3 b = vs[t.vb];
        Vec3 c = vs[t.vc];
        Vec3 n = Vec3Normalize(Vec3Cross(Vec3Sub(b, a), Vec3Sub(c, a)));

        ns[t.va] = n;
        ns[t.vb] = n;
        ns[t.vc] = n;

        IBUFFER[IBUFFER_SIZE++] = voffset + t.va;
        IBUFFER[IBUFFER_SIZE++] = voffset + t.vb;
        IBUFFER[IBUFFER_SIZE++] = voffset + t.vc;
    }

    for (u32 i = 0; i < vsize; ++i) {
        VBUFFER[VBUFFER_SIZE++] = (Vertex){.pos = vs[i], .normal = ns[i]};
    }

    LMesh* mesh = &MESHES[MESHES_SIZE];
    ++MESHES_SIZE;
    mesh->ibufferidx = ioffset;
    mesh->ntriangles = tsize;
}

static void RenderDataInit() {
    VAO = 0;
    VBO = 0;
    VIO = 0;
    memset(VBUFFER, 0, sizeof(VBUFFER));
    VBUFFER_SIZE = 0;
    memset(IBUFFER, 0, sizeof(IBUFFER));
    IBUFFER_SIZE = 0;

    // Important!!!! Init the buffer before sending to GPU or else no vertex data!!!!!

    // Cube.
    {
        Vec3 vs[] = {
            // Back.
            Vec3Lit(0.5f, -0.5f, -0.5f),
            Vec3Lit(-0.5f, -0.5f, -0.5f),
            Vec3Lit(-0.5f, 0.5f, -0.5f),
            Vec3Lit(0.5f, 0.5f, -0.5f),

            // Front.
            Vec3Lit(-0.5f, -0.5f, 0.5f),
            Vec3Lit(0.5f, -0.5f, 0.5f),
            Vec3Lit(0.5f, 0.5f, 0.5f),
            Vec3Lit(-0.5f, 0.5f, 0.5f),

            // Left.
            Vec3Lit(-0.5f, 0.5f, 0.5f),
            Vec3Lit(-0.5f, 0.5f, -0.5f),
            Vec3Lit(-0.5f, -0.5f, -0.5f),
            Vec3Lit(-0.5f, -0.5f, 0.5f),

            // Right.
            Vec3Lit(0.5f, 0.5f, -0.5f),
            Vec3Lit(0.5f, 0.5f, 0.5f),
            Vec3Lit(0.5f, -0.5f, 0.5f),
            Vec3Lit(0.5f, -0.5f, -0.5f),

            // Bottom.
            Vec3Lit(-0.5f, -0.5f, -0.5f),
            Vec3Lit(0.5f, -0.5f, -0.5f),
            Vec3Lit(0.5f, -0.5f, 0.5f),
            Vec3Lit(-0.5f, -0.5f, 0.5f),

            // Top.
            Vec3Lit(0.5f, 0.5f, -0.5f),
            Vec3Lit(-0.5f, 0.5f, -0.5f),
            Vec3Lit(-0.5f, 0.5f, 0.5f),
            Vec3Lit(0.5f, 0.5f, 0.5f),
        };
        Triangle ts[] = {
            // Back.
            TriLit(0, 1, 2),  //
            TriLit(2, 3, 0),  //

            // Front.
            TriLit(4, 5, 6),  //
            TriLit(6, 7, 4),  //

            // Left.
            TriLit(8, 9, 10),   //
            TriLit(10, 11, 8),  //

            // Right.
            TriLit(12, 13, 14),  //
            TriLit(14, 15, 12),  //

            // Bottom.
            TriLit(16, 17, 18),  //
            TriLit(18, 19, 16),  //

            // Top.
            TriLit(20, 21, 22),  //
            TriLit(22, 23, 20),  //
        };
        AddMesh(vs, ArraySizeOf(vs), ts, ArraySizeOf(ts));
    }

    // Quad.
    {
        // quad is aligned with the xy plane.
        Vec3 vs[] = {
            Vec3Lit(-0.5f, 0.5f, 0.0f),
            Vec3Lit(-0.5f, -0.5f, 0.0f),
            Vec3Lit(0.5f, -0.5f, 0.0f),
            Vec3Lit(0.5f, 0.5f, 0.0f),
        };
        Triangle ts[] = {
            TriLit(0, 1, 2),  //
            TriLit(2, 3, 0),  //
        };
        AddMesh(vs, ArraySizeOf(vs), ts, ArraySizeOf(ts));
    }

    // ISO TRIANGLE Mesh.
    {
        Vec3 vs[] = {
            Vec3Lit(0.0f, 1.0f, 0.0f),
            Vec3Lit(-1.0f, 0.0f, 0.0f),
            Vec3Lit(1.0f, 0.0f, 0.0f),
        };
        Triangle ts[] = {TriLit(0, 1, 2)};
        AddMesh(vs, ArraySizeOf(vs), ts, ArraySizeOf(ts));
    }

    // PYRAMID Mesh.
    {
        Vec3 vs[] = {
            // Base
            Vec3Lit(-1.0f, 1.0f, 0.0f),   //
            Vec3Lit(-1.0f, -1.0f, 0.0f),  //
            Vec3Lit(1.0f, -1.0f, 0.0f),   //
            Vec3Lit(1.0f, 1.0f, 0.0f),    //

            // Top point.
            Vec3Lit(0.0f, 0.0f, -1.0f),  //
        };
        Triangle ts[] = {
            // Base
            TriLit(0, 1, 2),  //
            TriLit(2, 3, 0),  //
            // Top
            TriLit(0, 3, 4),  //
            // Left
            TriLit(1, 0, 4),  //
            // Right
            TriLit(3, 2, 4),  //
            // Bottom
            TriLit(2, 1, 4),  //
        };
        AddMesh(vs, ArraySizeOf(vs), ts, ArraySizeOf(ts));
    }

    // 2S ISO TRIANGLE Mesh.
    {
        Vec3 vs[] = {
            Vec3Lit(1.0f, 0.0f, 0.0f),      //
            Vec3Lit(-0.5f, 0.866f, 0.0f),   //
            Vec3Lit(-0.5f, -0.866f, 0.0f),  //
            Vec3Lit(1.0f, 0.0f, 0.0f),      //
            Vec3Lit(-0.5f, -0.866f, 0.0f),  //
            Vec3Lit(-0.5f, 0.866f, 0.0f),   //
        };
        Triangle ts[] = {
            TriLit(0, 1, 2),  //
            TriLit(3, 4, 5),  //
        };
        AddMesh(vs, ArraySizeOf(vs), ts, ArraySizeOf(ts));
    }

    // Icosahedron Mesh.
    // https://en.wikipedia.org/wiki/Icosahedron
    {
        // golden ratio.
        f32 t = LPHI;

        Vec3 vs[] = {
            Vec3Normalize(Vec3Lit(-1.0f, t, 0.0f)),  Vec3Normalize(Vec3Lit(1.0f, t, 0.0f)),
            Vec3Normalize(Vec3Lit(-1.0f, -t, 0.0f)), Vec3Normalize(Vec3Lit(1.0f, -t, 0.0f)),
            Vec3Normalize(Vec3Lit(0.0f, -1.0f, t)),  Vec3Normalize(Vec3Lit(0.0f, 1.0f, t)),
            Vec3Normalize(Vec3Lit(0.0f, -1.0f, -t)), Vec3Normalize(Vec3Lit(0.0f, 1.0f, -t)),
            Vec3Normalize(Vec3Lit(t, 0.0f, -1.0f)),  Vec3Normalize(Vec3Lit(t, 0.0f, 1.0f)),
            Vec3Normalize(Vec3Lit(-t, 0.0f, -1.0f)), Vec3Normalize(Vec3Lit(-t, 0.0f, 1.0f)),
        };
        Triangle ts[] = {
            // 5 faces around point 0
            TriLit(0, 11, 5),
            TriLit(0, 5, 1),
            TriLit(0, 1, 7),
            TriLit(0, 7, 10),
            TriLit(0, 10, 11),

            // 5 adjacent faces
            TriLit(1, 5, 9),
            TriLit(5, 11, 4),
            TriLit(11, 10, 2),
            TriLit(10, 7, 6),
            TriLit(7, 1, 8),

            // 5 faces around point 3
            TriLit(3, 9, 4),
            TriLit(3, 4, 2),
            TriLit(3, 2, 6),
            TriLit(3, 6, 8),
            TriLit(3, 8, 9),

            // 5 adjacent faces
            TriLit(4, 9, 5),
            TriLit(2, 4, 11),
            TriLit(6, 2, 10),
            TriLit(8, 6, 7),
            TriLit(9, 8, 1),
        };
        AddMesh(vs, ArraySizeOf(vs), ts, ArraySizeOf(ts));
    }

    // ICOSPHERE Mesh.
    // https://gist.github.com/skeeto/b4aecead36ddec149d99b9f09830a373
    {
        // golden ratio.
        f32 t = LPHI;

        Vec3 vs[MAX_VERTICES] = {0};
        u32 vsize = 0;

        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(-1.0f, t, 0.0f));
        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(1.0f, t, 0.0f));

        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(-1.0f, -t, 0.0f));
        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(1.0f, -t, 0.0f));

        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(0.0f, -1.0f, t));
        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(0.0f, 1.0f, t));

        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(0.0f, -1.0f, -t));
        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(0.0f, 1.0f, -t));

        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(t, 0.0f, -1.0f));
        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(t, 0.0f, 1.0f));

        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(-t, 0.0f, -1.0f));
        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(-t, 0.0f, 1.0f));

        Triangle ts[MAX_TRIANGLES] = {0};
        u32 tsize = 0;

        // 5 faces around point 0
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(0, 11, 5);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(0, 5, 1);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(0, 1, 7);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(0, 7, 10);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(0, 10, 11);

        // 5 adjacent faces
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(1, 5, 9);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(5, 11, 4);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(11, 10, 2);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(10, 7, 6);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(7, 1, 8);

        // 5 faces around point 3
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(3, 9, 4);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(3, 4, 2);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(3, 2, 6);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(3, 6, 8);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(3, 8, 9);

        // 5 adjacent faces
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(4, 9, 5);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(2, 4, 11);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(6, 2, 10);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(8, 6, 7);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(9, 8, 1);

        // TODO: Issue. seems like a seg fault when I bump this too high. probably something
        // to do with max ibuffer or vbuffer size.
        u32 nsubdivides = 1;

        bool found[MAX_VERTICES][MAX_VERTICES] = {0};
        u32 midpoints[MAX_VERTICES][MAX_VERTICES] = {0};
        for (u32 count = 0; count < nsubdivides; ++count) {
            Triangle ts2[MAX_TRIANGLES] = {0};
            u32 ts2size = 0;
            for (u32 ti = 0; ti < tsize; ++ti) {
                Triangle t = ts[ti];
                u32 edges[][2] = {{t.va, t.vb}, {t.vb, t.vc}, {t.vc, t.va}};
                // compute midpoint of each edge.
                for (u32 ei = 0; ei < ArraySizeOf(edges); ++ei) {
                    u32 va = edges[ei][0];
                    u32 vb = edges[ei][1];
                    if (!found[va][vb]) {
                        Vec3 midpoint = Vec3Scale(Vec3Add(vs[va], vs[vb]), 0.5f);
                        u32 idx = vsize;
                        Assert(vsize < ArraySizeOf(vs));
                        vs[vsize++] = Vec3Normalize(midpoint);

                        found[va][vb] = true;
                        found[vb][va] = true;
                        midpoints[va][vb] = idx;
                        midpoints[vb][va] = idx;
                    }
                }

                u32 a = midpoints[t.va][t.vb];
                u32 b = midpoints[t.vb][t.vc];
                u32 c = midpoints[t.vc][t.va];

                Assert(ts2size < ArraySizeOf(ts2));
                ts2[ts2size++] = TriLit(t.va, a, c);
                Assert(ts2size < ArraySizeOf(ts2));
                ts2[ts2size++] = TriLit(t.vb, b, a);
                Assert(ts2size < ArraySizeOf(ts2));
                ts2[ts2size++] = TriLit(t.vc, c, b);
                Assert(ts2size < ArraySizeOf(ts2));
                ts2[ts2size++] = TriLit(a, b, c);
            }

            for (u32 j = 0; j < ts2size; ++j) {
                ts[j] = ts2[j];
            }
            tsize = ts2size;
        }

        AddMesh(vs, vsize, ts, tsize);
    }

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VBUFFER), VBUFFER, GL_STATIC_DRAW);

    glGenBuffers(1, &VIO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VIO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(IBUFFER), IBUFFER, GL_STATIC_DRAW);

    // TODO: Maybe this should go in GetShader?
    LShader shader = GetShader();
    glVertexAttribPointer(shader.vpos_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(shader.vpos_loc);
    glVertexAttribPointer(shader.vnormal_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(shader.vnormal_loc);
}

static Vertex LMeshGetVertex(LMesh mid, u32 vid) { return VBUFFER[IBUFFER[mid.ibufferidx + vid]]; }

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
// S:Geometry

static bool DEBUG_EVER = false;

static const f32 SLOP = 2.0f * F32_EPSILON;

static const Vec3 AXIS[] = {Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f)};

static Vec3 ComputeCentroid(Vec3* vs, u32 vsize) {
    Vec3 center = {0};
    for (u32 i = 0; i < vsize; ++i) {
        center = Vec3Add(center, vs[i]);
    }
    center.x /= vsize;
    center.y /= vsize;
    center.z /= vsize;
    return center;
}

typedef struct Sphere {
    Vec3 c;  // center
    f32 r;   // radius
} Sphere;

// Computes a sphere in the coord space of the passed in verts.
static Sphere BoundingSphere(Vec3* vs, u32 vsize) {
    Vec3 center = ComputeCentroid(vs, vsize);

    f32 radius = 0.0f;
    for (u32 i = 0; i < vsize; ++i) {
        f32 len = Vec3Len(Vec3Sub(vs[i], center));
        if (len > radius) {
            radius = len;
        }
    }

    return (Sphere){.c = center, .r = radius};
}

// Returns normal pointing from b to a if there is a collision.
static bool TestSphereVsSphere(Sphere a, Sphere b, Vec3* normal, f32* depth) {
    bool hit = false;
    Vec3 dir = Vec3Sub(b.c, a.c);
    f32 d = Vec3Dot(dir, dir);
    f32 rr = a.r + b.r;
    if (d <= rr * rr) {
        hit = true;
        d = f32sqrt(d);
        dir = Vec3Normalize(dir);
    }

    *normal = dir;
    *depth = d;
    return hit;
}

typedef struct AABB {
    Vec3 c;  // center.
    Vec3 r;  // half width extents along each axis.
} AABB;

// Computes the axis aligned bounding box in the coordinate space of passed in vertices.
static AABB ComputeAABB(Vec3* vs, u32 size) {
    AABB aabb = {0};
    for (u32 i = 0; i < ArraySizeOf(AXIS); ++i) {
        Vec3 dir = AXIS[i];

        f32 dmin = F32_MAX;
        f32 dmax = -F32_MAX;
        for (u32 j = 0; j < size; ++j) {
            f32 d = Vec3Dot(vs[j], dir);
            if (d < dmin) {
                dmin = d;
            }
            if (d > dmax) {
                dmax = d;
            }
        }

        // compute the half width extent of the dimension of the axis.
        // the center can be found by adding the radius to the min distance.
        // value in the correct coordinate.
        aabb.r.f[i] = 0.5f * (dmax - dmin);
        aabb.c.f[i] = aabb.r.f[i] + dmin;
    }

    return aabb;
}

static bool TestAABBVsAABB(AABB a, AABB b, Vec3* normal, f32* depth) {
    if (DEBUG_EVER) {
        breakpoint();
    }

    f32 dcx = a.c.f[0] - b.c.f[0];
    f32 dx = (a.r.f[0] + b.r.f[0]) - f32abs(dcx);
    if (dx <= 0.0) {
        return false;
    }

    f32 dcy = a.c.f[1] - b.c.f[1];
    f32 dy = (a.r.f[1] + b.r.f[1]) - f32abs(dcy);
    if (dy <= 0.0f) {
        return false;
    }

    f32 dcz = a.c.f[2] - b.c.f[2];
    f32 dz = (a.r.f[2] + b.r.f[2]) - f32abs(dcz);
    if (dz <= 0.0f) {
        return false;
    }

    Vec3 n = Vec3Lit(1.0f * (dcx < 0.0f ? -1.0f : 1.0f), 0.0f, 0.0f);
    f32 d = dx;
    if (dy < d || dz < d) {
        if (dy < dz) {
            n = Vec3Lit(0.0f, 1.0f * (dcy < 0.0f ? -1.0f : 1.0f), 0.0f);
            d = dy;
        } else {
            n = Vec3Lit(0.0f, 0.0f, 1.0f * (dcy < 0.0f ? -1.0f : 1.0f));
            d = dz;
        }
    }

    *normal = n;
    *depth = d;
    return true;
}

// Returns normal pointing from aabb to sphere if there is a collision.
static bool TestSphereVsAABB(Sphere s, AABB aabb, Vec3* normal, f32* depth) {
    Vec3 max = Vec3Add(aabb.c, aabb.r);
    Vec3 min = Vec3Sub(aabb.c, aabb.r);

    // Find closest point from sphere center to the space bounded by aabb (which might be inside of) aabb.
    Vec3 cp = Vec3Zero();
    for (u32 i = 0; i < ArraySizeOf(AXIS); ++i) {
        f32 v = s.c.f[i];
        if (v < min.f[i]) {
            v = min.f[i];
        }
        if (v > max.f[i]) {
            v = max.f[i];
        }
        cp.f[i] = v;
    }

    bool hit = false;
    Vec3 dir = Vec3Sub(cp, s.c);
    f32 d = Vec3Dot(dir, dir);
    if (d <= s.r * s.r) {
        // if (DEBUG_EVER) {
        //     breakpoint();
        // }
        hit = true;
        d = f32sqrt(d);
        // TODO: Add SLOP.
        d = (s.r - d);
        // Want to point normal towards sphere so negate.
        dir = Vec3Normalize(Vec3Negate(dir));
    }

    *normal = dir;
    *depth = d;
    return hit;
}

typedef enum PolyhedronType { PH_NONE = 0, PH_SPHERE, PH_AABB, PH_COUNT } PolyhedronType;

typedef struct Polyhedron {
    PolyhedronType type;

    union {
        Sphere sphere;
        AABB aabb;
    };
} Polyhedron;

static Polyhedron ComputePolyhedron(Vec3* vs, u32 vsize, PolyhedronType type) {
    Polyhedron p = {0};
    p.type = type;
    switch (type) {
        case PH_SPHERE: {
            p.sphere = BoundingSphere(vs, vsize);
            break;
        }
        case PH_AABB: {
            p.aabb = ComputeAABB(vs, vsize);
            break;
        }
        default: {
            TraceLog(LOG_ERROR,
                     "ComputePolyhedron: Unknown polyhedron type=%d. Most likely entity needs bounding volume type set",
                     type);
            exit(1);
        }
    }
    return p;
}

// Returns true if both polyhedrons collide. normal will point from b to a.
static bool TestPolyhedronVsPolyhedron(Polyhedron* a, Polyhedron* b, Vec3* normal, f32* depth) {
    bool collides = false;
    Vec3 n = Vec3Zero();
    f32 d = 0.0f;

    // Collision table. if we don't have an entry in the collision table set up between two
    // polyedron types, then we just say they never collide.
    // IMPORTANT: If you are adding new entries here, you need to handle a vs b and b vs a
    // comparisons.
    // TODO: Can we make this nicer? Could we literally turn this into a table and just make reverse
    // functions? ex. Take TestSphereVsAABB. we could make TestAABBVsSphere and have it call the former function
    // and add negation to the collision normal.
    if (a->type == PH_SPHERE) {
        if (b->type == PH_AABB) {
            collides = TestSphereVsAABB(a->sphere, b->aabb, &n, &d);
        }
    } else if (a->type == PH_AABB) {
        if (b->type == PH_AABB) {
            collides = TestAABBVsAABB(a->aabb, b->aabb, &n, &d);
        } else if (b->type == PH_SPHERE) {
            collides = TestSphereVsAABB(b->sphere, a->aabb, &n, &d);
            // point normal from b to a.
            n = Vec3Negate(n);
        }
    }

    *normal = n;
    *depth = d;
    return collides;
}

////////////////////////////////////////////////////////////

const float RenderWidth = 1280;
const float RenderHeight = 720;

static const Vec3 ORIGIN = Vec3Lit(0.0f, 0.0f, 0.0f);
static const Vec2 ORIGIN2D = Vec2Lit(0.0f, 0.0f);

static const u8 GAMEPAD_ID = 0;

////////////////////////////////////////////////////////////
static const Vec3 FORWARD = Vec3Lit(0.0f, 0.0f, -1.0f);

typedef enum EntityFlag {
    EFLAG_NONE = 0,
    EFLAG_ACTIVE = 1 << 0,
    EFLAG_DRAW = 1 << 1,
    EFLAG_PLAYER_CONTROLLED = 1 << 2,
    EFLAG_PHYSICS_ON = 1 << 3,
    EFLAG_COLLIDER = 1 << 4,
    EFLAG_MOB = 1 << 5,
    EFLAG_PROJECTILE = 1 << 6,
    EFLAG_WALL = 1 << 7,
    EFLAG_BUMPER = 1 << 8,
    EFLAG_BALL = 1 << 9,
    EFLAG_BLOCK = 1 << 10,
} EntityFlag;

typedef u32 EntityId;

typedef struct Entity {
    /////////////////////////////////////////////////////////////////
    // Core field.
    EntityId id;
    // TODO: Convert to u64
    EntityFlag flags;

    // Decomposition of model matrix.
    Vec3 pos;
    Vec3 ppos;  // prev pos.
    Vec3 scale;
    Mat4 orient;

    Vec3 v;
    f32 friction;
    f32 restitution;

    LMesh mesh;
    Vec4 color;

    // TODO: At some point it might make more sense to store a polyhedron struct itself here.
    // Might only make sense if we ever need to incrementally recompute bounding volumes.
    // Right now we always do on the fly.
    PolyhedronType phtype;

    /////////////////////////////////////////////////////////////////
    // Gameplay fields.

    bool hit;
    f32 hit_timer;
    i32 last_hit;  // entity id. signed to set to -1 for sentinel val.

    bool straight_shot;

    u32 health;

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
    entity->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
    entity->orient = Mat4Identity();
    entity->color = Vec4Lit(0.0f, 0.0f, 0.0f, 1.0f);
    FlagOn(entity->flags, EFLAG_DRAW);
    return entity;
}

static Mat4 EntityGetMat(Entity* e) {
    Mat4 scale = Mat4ScaleV(e->scale);
    Mat4 translate = Mat4TranslateV(e->pos);
    Mat4 model = Mat4Multiply(translate, Mat4Multiply(e->orient, scale));
    return model;
}

// Returns the bounding sphere in world space.
static Sphere EntityBoundingSphere(Entity* e) {
    Mat4 m = EntityGetMat(e);
    Vec3 vs[MAX_VERTICES] = {0};
    u32 vsize = e->mesh.ntriangles * 3;
    Assert(vsize < MAX_VERTICES);
    for (u32 i = 0; i < vsize; ++i) {
        Vec3 pos = LMeshGetVertex(e->mesh, i).pos;
        vs[i] = Vec3WTransform(pos, m);
    }

    return BoundingSphere(vs, vsize);
}

// Returns the AABB in world sphere.
static AABB EntityAABB(Entity* e) {
    Mat4 m = EntityGetMat(e);
    Vec3 vs[MAX_VERTICES] = {0};
    u32 vsize = e->mesh.ntriangles * 3;
    Assert(vsize < MAX_VERTICES);
    for (u32 i = 0; i < vsize; ++i) {
        Vec3 pos = LMeshGetVertex(e->mesh, i).pos;
        vs[i] = Vec3WTransform(pos, m);
    }

    return ComputeAABB(vs, vsize);
}

static Polyhedron EntityBoundingVolume(Entity* e) {
    Mat4 m = EntityGetMat(e);
    Vec3 vs[MAX_VERTICES] = {0};
    u32 vsize = e->mesh.ntriangles * 3;
    Assert(vsize < MAX_VERTICES);
    for (u32 i = 0; i < vsize; ++i) {
        Vec3 pos = LMeshGetVertex(e->mesh, i).pos;
        vs[i] = Vec3WTransform(pos, m);
    }

    return ComputePolyhedron(vs, vsize, e->phtype);
}

////////////////////////////////////////////////////
typedef struct CollisionInfo {
    EntityId id0;
    EntityId id1;

    f32 depth;
    Vec3 normal;
} CollisionInfo;

// L Check And Flip
#define LCAF(e0, e1, f0, f1, cinfo)

////////////////////////////////////////////////////

static Mat4 GetCameraMat(Vec3 eye, Vec3 orient) {
    return Mat4Invert(Mat4Multiply(Mat4TranslateV(eye), Mat4RotateEuler(orient)));
}

////////////////////////////////////////////////////

#define MAX_SCENES 10

typedef struct GameState {
    /////////////////////////////////////////
    // Core fields
    bool debug;
    bool update;
    bool game_over;
    bool init;  // False on first frame of scene load. Used to tell update function to perform init.

    bool debug_ever;

    f32 dt;  // frame delta time.
    f32 timer;
    u32 nframe;  // num frame we are on.

    void (*scenefuncs[MAX_SCENES])(struct GameState*);
    u32 scenefuncssize;
    u32 sceneid;

    /////////////////////////////////////////
    // Gameplay fields
} GameState;

static GameState GState;

#define GetGameState() (&GState)

typedef void (*SceneTickFunc)(GameState* gstate);

// Resets global state/system state that contains per scene info. Experimental approach
// for use in scene switching as opposed to putting all this state into GameState object
// and passing that around. The globalness of these systems is kind of convenient at the
// moment.
static void GameStateLoadScene(GameState* gstate, u32 sceneid) {
    Assert(sceneid < MAX_SCENES);

    EntitySystemInit();
    CmdBufferZero();
    RenderDataInit();

    // Signal to scene update func to run init.
    gstate->init = false;
    gstate->update = true;
    gstate->debug = false;
    gstate->debug_ever = false;
    gstate->timer = 0.0f;
    gstate->nframe = 0;

    gstate->sceneid = sceneid;
}

static bool ON = false;

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

            // If intervals do not overlap, then no collision.
            // TODO: floating point robustness.
            if (tamax < tbmin || tamin > tbmax) {
                collides = false;
                dir = axis;  // Useful to keep axis of separationg for debugging.
                goto collideret;
            }

            // Otherwise there is a collision and we need to determine what the displacement is.
            // (dir of displacement is the separating axis).
            f32 imin = f32max(tamin, tbmin);
            f32 imax = f32min(tamax, tbmax);
            f32 d = imax - imin;
            // TODO: floating point robustness.
            Assert(d > 0.0f);

            // If the separation is more minimal, then remember axis + sep distance.
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
    // TODO: floating point robustness.
    f32 flip = Vec3Dot(dir, atob) < 0.0f ? -1.0f : 1.0f;
    dir = Vec3Scale(dir, flip);

    *dirout = dir;
    *dout = dmin;
    return collides;
}

static void DrawEntities(LShader shader, EntityId* entities, u32 size) {
    for (u32 i = 0; i < size; ++i) {
        Entity* entity = GetEntity(entities[i]);
        Mat4 scale = Mat4ScaleV(entity->scale);
        Mat4 translate = Mat4TranslateV(entity->pos);
        Mat4 model = Mat4Multiply(translate, Mat4Multiply(entity->orient, scale));
        AddCmd(entity->mesh, model, entity->color);
    }
}

static void DrawEntitiesWireFrame(LShader shader, EntityId* entities, u32 size) {
    for (u32 i = 0; i < size; ++i) {
        Entity* entity = GetEntity(entities[i]);
        Mat4 scale = Mat4ScaleV(entity->scale);
        Mat4 translate = Mat4TranslateV(entity->pos);
        Mat4 model = Mat4Multiply(translate, Mat4Multiply(entity->orient, scale));
        DrawCmd* cmd = AddCmd(entity->mesh, model, entity->color);
        cmd->wireframe = true;
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

static void DrawSphereWf(Vec3 center, f32 radius, Vec4 color) {
    DrawCmd* cmd = AddCmd(GetMesh(M_ICOSPHERE), Mat4SRT(radius, Mat4Identity(), center), color);
    cmd->wireframe = true;
}

static void DrawAABBWf(Vec3 center, Vec3 radius, Vec4 color) {
    // Reminder that radius are the *half* widths, so need to multiply by 2 to scale correctly
    DrawCmd* cmd = AddCmd(GetMesh(M_CUBE), Mat4SVRT(Vec3Scale(radius, 2.0f), Mat4Identity(), center), color);
    cmd->wireframe = true;
}

#define MAX_ENTITY_BATCH 256

static void Scene1Tick(GameState* gstate) {
    static Vec3 viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);
    static Vec3 vieworient = Vec3Lit(0.0f, 0.0f, 0.0f);

    static Vec3 LIGHT_POS = Vec3Lit(10.0f, 10.0f, -10.0f);

    static EntityId LIGHTS[MAX_ENTITY_BATCH] = {0};
    // static u32 LIGHTS_SIZE = 0;

    static EntityId SPINNING[MAX_ENTITY_BATCH] = {0};
    static u32 SPINNING_SIZE = 0;

    static EntityId MESH_TEST[MAX_ENTITY_BATCH] = {0};
    static u32 MESH_TEST_SIZE = 0;

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

    f32 dt = gstate->dt;

    // Init logic.
    if (!gstate->init) {
        viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);
        vieworient = Vec3Lit(0.0f, 0.0f, 0.0f);

        LIGHT_POS = Vec3Lit(10.0f, 10.0f, -10.0f);

        memset(LIGHTS, 0, sizeof(LIGHTS));
        memset(SPINNING, 0, sizeof(SPINNING));
        SPINNING_SIZE = 0;
        memset(SHADOW_DEBUG, 0, sizeof(SHADOW_DEBUG));
        SHADOW_DEBUG_SIZE = 0;
        memset(PHYSICS, 0, sizeof(PHYSICS));
        PHYSICS_SIZE = 0;
        memset(ISO_TRIANGLES, 0, sizeof(ISO_TRIANGLES));
        ISO_TRIANGLES_SIZE = 0;
        memset(COLLISIONS, 0, sizeof(COLLISIONS));
        COLLISIONS_SIZE = 0;
        memset(PYRAMIDS, 0, sizeof(PYRAMIDS));
        PYRAMIDS_SIZE = 0;
        memset(WALLS, 0, sizeof(WALLS));
        WALLS_SIZE = 0;

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

        // MESH_TEST init
        {
            EntityId* entities = MESH_TEST;
            u32 size = 0;

            const Vec3 origin = Vec3Lit(-40.0f, 0.0f, 0.0f);

            // Colliders
            {
                Vec4 color = LGRAY;
                color.a = 0.5f;

                Vec3 pos = origin;

                for (u32 i = 0; i < M_COUNT; ++i, ++size) {
                    Entity* entity = AllocateEntity();
                    entity->pos = pos;
                    entity->mesh = GetMesh(i);
                    entity->orient = Mat4Identity();
                    entity->color = color;
                    entity->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
                    entities[size] = entity->id;

                    pos = Vec3Add(pos, Vec3Lit(0, 0, 5.0f));
                }
            }

            MESH_TEST_SIZE = size;
            Assert(MESH_TEST_SIZE < MAX_ENTITY_BATCH);
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
                entity->scale = Vec3Lit(3.0f, 3.0f, 3.0f);
                entities[i] = entity->id;
            }
        }

        // PHYSICS init
        {
            EntityId* entities = PHYSICS;
            u32 size = 0;

            const Vec3 origin = Vec3Lit(-20.0f, 30.0f, -50.0f);

            // Colliders
            {
                const Vec3 positions[] = {origin};
                const Vec3 velocities[] = {Vec3Lit(3.0f, 10.0f, -6.0f)};
                const f32 rotations[] = {Deg2Rad(0.0f)};
                Vec4 color = LPURPLE;
                color.a = 0.16f;
                const Vec4 colors[] = {color};

                for (u32 i = 0; i < ArraySizeOf(positions); ++i, ++size) {
                    Entity* entity = AllocateEntity();
                    entity->pos = positions[i];
                    entity->mesh = GetMesh(M_CUBE);
                    entity->orient = Mat4Rotate(rotations[i], Vec3Lit(0.0f, 1.0f, 0.0f));
                    entity->color = colors[i];
                    entity->scale = Vec3Lit(3.0f, 3.0f, 3.0f);
                    entity->v = velocities[i];
                    FlagOn(entity->flags, EFLAG_COLLIDER);
                    entities[size] = entity->id;
                }
            }

            // Walls
            {
                const f32 len = 20.0f;
                const f32 hlen = len / 2.0f;
                const Vec3 positions[] = {
                    Vec3Add(origin, Vec3Lit(-hlen, 0.0f, 0.0f)), Vec3Add(origin, Vec3Lit(hlen, 0.0f, 0.0f)),
                    Vec3Add(origin, Vec3Lit(0.0f, 0.0f, hlen)),  Vec3Add(origin, Vec3Lit(0.0f, 0.0f, -hlen)),
                    Vec3Add(origin, Vec3Lit(0.0f, -hlen, 0.0f)), Vec3Add(origin, Vec3Lit(0.0f, hlen, 0.0f))};
                const Vec3 axis[] = {Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),
                                     Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f)};
                const f32 rotations[] = {Deg2Rad(90.0f),  Deg2Rad(90.0f), Deg2Rad(180.0f),
                                         Deg2Rad(180.0f), Deg2Rad(90.0f), Deg2Rad(90.0f)};
                const Vec4 colors[] = {LGRAY, LGRAY, LGRAY, LGRAY, LGRAY, LGRAY};
                const Vec3 scale = Vec3Lit(len, len, 1.0f);

                for (u32 i = 0; i < ArraySizeOf(positions); ++i, ++size) {
                    Entity* entity = AllocateEntity();
                    entity->pos = positions[i];
                    entity->mesh = GetMesh(M_CUBE);
                    entity->orient = Mat4Rotate(rotations[i], axis[i]);
                    entity->color = colors[i];
                    entity->scale = Vec3Lit(3.0f, 3.0f, 3.0f);
                    entity->scale = scale;
                    FlagOn(entity->flags, EFLAG_COLLIDER);
                    entities[size] = entity->id;
                }
            }

            PHYSICS_SIZE = size;
            Assert(PHYSICS_SIZE < MAX_ENTITY_BATCH);
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
                entity->scale = Vec3Lit(3.0f, 3.0f, 3.0f);
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
                            entity->scale = Vec3Lit(3.0f, 3.0f, 3.0f);  // 1.0f;
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
                            entity->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
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
                            entity->scale = Vec3Lit(3.0f, 3.0f, 3.0f);  // 1.0f;
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
                            entity->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
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
                    // This first case is edge on edge collision.
                    // TODO: The collision detects, but the normal is not right. need to figure out
                    // how to handle case of parallel edges. How to determine what the axis of separation is
                    // if edges are parallel and on a line with eachother?
                    Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
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
                            entity->scale = Vec3Lit(3.0f, 3.0f, 3.0f);  // 1.0f;
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
                            entity->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
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
                entity->scale = Vec3Lit(3.0f, 3.0f, 3.0f);
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
                entity->scale = Vec3Lit(10000.0f, 10000.0f, 10000.0f);
                entities[i] = entity->id;
            }
        }
    }

    // Update logic.
    {
        // Camera movement.
        {
            static const u8 GAMEPAD_ID = 0;
            static bool FREECAM = false;

            if (IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
                FREECAM = !FREECAM;
            }

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
                // If not freecam, then limiting pitch to 0 will do fps style movement along current plane.
                bodyorient.pitch = FREECAM ? bodyorient.pitch : 0.0f;
                Mat4 rotation = Mat4RotateEuler(bodyorient);

                Vec3 dir_world = Vec3Transform(dir_model, rotation);
                Vec3 mov_world = Vec3Scale(dir_world, vel * dt);
                viewpos = Vec3Add(viewpos, mov_world);
            }

            // Right stick movement.
            {
                f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_X);
                f32 y = -1.0f * GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_Y);
                if (x != 0.0f || y != 0.0f) {
                    static const rad VEL = Deg2Rad(90.0f);

                    // rotate orient.
                    vieworient.yaw += -1.0f * x * VEL * dt;
                    vieworient.pitch += y * VEL * dt;
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

            if (IsKeyPressed(KEY_R)) {
                ON = !ON;
                TraceLog(LOG_INFO, "debug %s", ON ? "on" : "off");
            }

            // Physics update.
            {
                static bool OFF = false;

                if (IsKeyPressed(KEY_SPACE)) {
                    OFF = !OFF;
                }

                EntityId* entities = PHYSICS;
                u32 size = PHYSICS_SIZE;

                if (!OFF) {
                    for (u32 i = 0; i < size; ++i) {
                        Entity* entity = GetEntity(entities[i]);
                        entity->pos = Vec3Add(entity->pos, Vec3Scale(entity->v, dt));
                    }
                }

                for (u32 eidx = 1; eidx < size; ++eidx) {
                    Entity* e0 = GetEntity(entities[0]);
                    Entity* e1 = GetEntity(entities[eidx]);

                    if (!OFF) {
                        Vec3 dir = Vec3Zero();
                        f32 dmin = 0.0f;
                        bool collides = CollideEntityEntity(e0, e1, &dir, &dmin);

                        if (collides) {
                            Vec3 normal = Vec3Scale(dir, -1.0f);
                            Mat4 reflect = Mat4Reflect(normal);
                            e0->v = Vec3Transform(e0->v, reflect);

                            // e0->v = Vec3Zero();
                            e1->v = Vec3Zero();
                            {
                                Vec3 arrow = Vec3Scale(dir, -1.0f);
                                e0->pos = Vec3Add(e0->pos, Vec3Scale(arrow, 0.5f * dmin));
                                Mat4 orient = Mat4PointAt(arrow);
                                DrawArrow(arrow, e0->pos, orient, 0.3f, LPURPLE);
                            }
                            {
                                Vec3 arrow = dir;
                                e0->pos = Vec3Add(e0->pos, Vec3Scale(arrow, 0.5f * dmin));
                                Mat4 orient = Mat4PointAt(arrow);
                                DrawArrow(arrow, e1->pos, orient, 0.3f, LBLACK);
                            }
                        } else {
                            {
                                Vec3 arrow = dir;
                                Mat4 orient = Mat4PointAt(arrow);
                                DrawArrow(arrow, e1->pos, orient, 0.3f, LGREEN);
                            }
                        }
                    }
                }
                DrawEntities(shader, entities, size);
                DrawEntitiesWireFrame(shader, &entities[1], size - 1);
            }

            // MESH_TEST update.
            {
                EntityId* entities = MESH_TEST;
                u32 size = MESH_TEST_SIZE;
                for (u32 i = 0; i < size; ++i) {
                    Entity* entity = GetEntity(entities[i]);
                    entity->orient = Mat4Rotate(gstate->timer * 0.5f, Vec3Lit(1.0f, 0.3f, 0.5f));

                    if (gstate->debug) {
                        Sphere sphere = EntityBoundingSphere(entity);
                        DrawSphereWf(sphere.c, sphere.r, LYELLOW);
                        AABB aabb = EntityAABB(entity);
                        DrawAABBWf(aabb.c, aabb.r, LBLUE);
                    }
                }
                DrawEntities(shader, entities, size);
                DrawEntitiesWireFrame(shader, entities, size);
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
                    e->pos = Vec3Add(e->pos, Vec3Scale(e->v, dt));
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

                        //         Vec3 ab = Vec3Sub(t.vb.pos, t.va.pos);
                        //         Vec3 ac = Vec3Sub(t.vc.pos, t.va.pos);
                        //         Vec3 norm = Vec3Cross(ab, ac);

                        //         Vec3 center = Vec3Scale(Vec3Add(t.va.pos, Vec3Add(t.vb.pos, t.vc.pos)), 1.0f / 3.0f);
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
                    Entity* e = GetEntity(entities[i]);
                    e->pos = Vec3Add(e->pos, Vec3Scale(e->v, dt));
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

        Mat4 view = GetCameraMat(viewpos, vieworient);
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
            // {
            //     // Calculate the sun direction in view space (only factoring in rotation).
            //     Vec3 lightdir = Vec3Lit(-0.2f, -1.0f, -0.3f);
            //     // Vec3 lightdir = Vec3Lit(0.0f, 0.0f, -1.0f);
            //     Mat4 norm = Mat4Transpose(Mat4Invert(view));
            //     norm.v.d = Vec3Zero();
            //     Vec3 lightdir_eye = Vec3Transform(lightdir, norm);
            // }
            glUniformMatrix4fv(shader.vview_loc, 1, GL_FALSE, (const f32*)&view.m);
            glUniformMatrix4fv(shader.vproj_loc, 1, GL_FALSE, (const f32*)&proj.m);
        }

        for (u32 i = 0; i < CBUFFER_SIZE; ++i) {
            DrawCmd* cmd = &CBUFFER[i];
            glUniformMatrix4fv(shader.vmodel_loc, 1, GL_FALSE, (const f32*)&cmd->modelm.m);
            glUniform4fv(shader.vucolor_loc, 1, (const f32*)&cmd->color.f);
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

static void Scene2Tick(GameState* gstate) {
    static Vec3 viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);
    static Vec3 vieworient = Vec3Lit(0.0f, 0.0f, 0.0f);

    static EntityId QUAD[MAX_ENTITY_BATCH] = {0};
    static u32 QUAD_SIZE = 0;

    static u32 VAO2 = 0;
    static u32 VBO2 = 0;
    static u32 TID = 0;
    static LTexture T1 = {0};

    f32 dt = gstate->dt;

    // Init logic.
    if (!gstate->init) {
        f32 vertices[] = {
            // pos            // tex
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  //
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  //
            1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  //

            0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  //
            1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  //
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  //
        };

        LShader shader = GetShader2D();

        glGenVertexArrays(1, &VAO2);
        glGenBuffers(1, &VBO2);

        glBindBuffer(GL_ARRAY_BUFFER, VBO2);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindVertexArray(VAO2);
        glBindBuffer(GL_ARRAY_BUFFER, VBO2);
        glEnableVertexAttribArray(shader.vpos_loc);
        glVertexAttribPointer(shader.vpos_loc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(shader.vtextcoords_loc);
        glVertexAttribPointer(shader.vtextcoords_loc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void*)(3 * sizeof(float)));

        {
            i32 x, y, n;
            stbi_set_flip_vertically_on_load(true);
            u8* data = stbi_load("assets/textures/awesomeface.png", &x, &y, &n, 0);
            Assert(data != NULL);

            glGenTextures(1, &TID);
            glBindTexture(GL_TEXTURE_2D, TID);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                            GL_REPEAT);  // set texture wrapping to GL_REPEAT (default wrapping method)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            // set texture filtering parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

            stbi_image_free(data);
        }

        {
            i32 n = 0;
            stbi_set_flip_vertically_on_load(true);
            u8* data = stbi_load("assets/textures/helloatlas.png", &T1.width, &T1.height, &n, 0);
            Assert(data != NULL);

            glGenTextures(1, &T1.id);
            glBindTexture(GL_TEXTURE_2D, T1.id);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                            GL_REPEAT);  // set texture wrapping to GL_REPEAT (default wrapping method)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            // set texture filtering parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, T1.width, T1.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

            stbi_image_free(data);
        }

        glUseProgram(shader.pid);
        glUniform1i(glGetUniformLocation(shader.pid, "texture0"), 0);

        viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);
        vieworient = Vec3Lit(0.0f, 0.0f, 0.0f);

        memset(QUAD, 0, sizeof(QUAD));
        QUAD_SIZE = 0;

        vieworient.yaw = Deg2Rad(90.0f);

        // QUAD init
        {
            EntityId* entities = QUAD;
            u32 size = 0;

            const Vec3 origin = Vec3Lit(-20.0f, 30.0f, -50.0f);

            Entity* e = AllocateEntity();
            e->pos = Vec3Zero();
            e->mesh = GetMesh(M_QUAD);
            e->color = LRED;
            e->scale = Vec3Lit(50.0f, 50.0f, 1.0f);
            entities[size++] = e->id;

            QUAD_SIZE = size;
            Assert(QUAD_SIZE < MAX_ENTITY_BATCH);
        }
    }

    // Update logic.
    {
        {
            LShader shader = GetShader2D();

            // QUAD.
            {
                EntityId* entities = QUAD;
                u32 size = QUAD_SIZE;
                DrawEntities(shader, entities, size);
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
            glBindVertexArray(VAO2);
        }

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

        Mat4 proj = Mat4Ortho(0, 800, 600, 0, -1.0f, 1.0f);

        LShader shader = GetShader2D();

        {
            glUseProgram(shader.pid);
            glUniformMatrix4fv(shader.vproj_loc, 1, GL_FALSE, (const f32*)&proj.m);
        }

        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, TID);

            Mat4 scale = Mat4Scale(50.0f);
            Mat4 orient = Mat4Rotate(gstate->timer, Vec3Lit(0.0f, 0.0f, 1.0f));
            Mat4 translate = Mat4TranslateV(Vec3Lit(300.0f, 300.0f, 0.0f));
            Mat4 model = Mat4Multiply(Mat4Multiply(translate, orient), scale);

            Vec2 uvoffset = Vec2Zero();
            Vec2 uvscale = Vec2Lit(1.0f, 1.0f);

            glUniformMatrix4fv(shader.vmodel_loc, 1, GL_FALSE, (const f32*)&model.m);
            glUniform4fv(shader.vucolor_loc, 1, (const f32*)&LRED.f);
            glUniform2fv(shader.vuvoffset_loc, 1, (const f32*)&uvoffset.f);
            glUniform2fv(shader.vuvscale_loc, 1, (const f32*)&uvscale.f);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, T1.id);

            Vec3 pos = Vec3Lit(300.0f, 200.0f, 0.0f);

            {
                Mat4 scale = Mat4Scale(100.0f);
                Mat4 orient = Mat4Identity();
                Mat4 translate = Mat4TranslateV(pos);
                Mat4 model = Mat4Multiply(Mat4Multiply(translate, orient), scale);

                Vec2 uvoffset = Vec2Zero();
                Vec2 uvscale = Vec2Lit(1.0f, 1.0f);

                glUniformMatrix4fv(shader.vmodel_loc, 1, GL_FALSE, (const f32*)&model.m);
                glUniform4fv(shader.vucolor_loc, 1, (const f32*)&LCLEAR.f);
                glUniform2fv(shader.vuvoffset_loc, 1, (const f32*)&uvoffset.f);
                glUniform2fv(shader.vuvscale_loc, 1, (const f32*)&uvscale.f);

                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            pos = Vec3Add(pos, Vec3Lit(T1.width + 40.0f, 0.0f, 0.0f));

            {
                {
                    LTexture tx = T1;

                    for (u32 i = 0; i < 2; ++i) {
                        for (u32 j = 0; j < 2; ++j) {
                            Vec2 r_src = Vec2Lit(48.0f, 48.0f);
                            Vec2 p_src = Vec2Lit(r_src.x * i, r_src.y * j);

                            Vec2 r_dst = Vec2Lit(100.0f, 100.0f);
                            Vec2 p_dst = Vec2Lit(pos.x + (r_dst.x * i * 1.3f), pos.y + (r_dst.x * j * 1.3f));

                            // TODO: is scale right?
                            Vec2 uvscale = Vec2Lit(r_src.x / tx.width, r_src.y / tx.height);
                            Vec2 uvoffset = Vec2Lit(p_src.x / tx.width, p_src.y / tx.height);

                            Mat4 scale = Mat4ScaleV(Vec3Lit(r_dst.x, r_dst.y, 1.0f));
                            Mat4 orient = Mat4Identity();

                            Mat4 translate = Mat4TranslateV(Vec3Lit(p_dst.x, p_dst.y, 0.0f));
                            Mat4 model = Mat4Multiply(Mat4Multiply(translate, orient), scale);

                            glUniformMatrix4fv(shader.vmodel_loc, 1, GL_FALSE, (const f32*)&model.m);
                            glUniform4fv(shader.vucolor_loc, 1, (const f32*)&LCLEAR.f);
                            glUniform2fv(shader.vuvoffset_loc, 1, (const f32*)&uvoffset.f);
                            glUniform2fv(shader.vuvscale_loc, 1, (const f32*)&uvscale.f);

                            glDrawArrays(GL_TRIANGLES, 0, 6);
                        }
                    }
                }
            }
        }

        EndDrawing();
        CmdBufferZero();  // Maybe consider moving this to be at the top of update func.
    }
}

static void Scene3Tick(GameState* gstate) {
    static Vec3 viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);
    static Vec3 vieworient = Vec3Lit(0.0f, 0.0f, 0.0f);

    static EntityId playerid;

    static EntityId MOBS[MAX_ENTITY_BATCH] = {0};
    static u32 MOBS_SIZE = 0;

    static EntityId TARGETS[MAX_ENTITY_BATCH] = {0};
    static u32 TARGETS_SIZE = 0;

    static EntityId WALLS[MAX_ENTITY_BATCH] = {0};
    static u32 WALLS_SIZE = 0;

    f32 dt = gstate->dt;

    // Init logic.
    if (!gstate->init) {
        viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);

        memset(MOBS, 0, sizeof(MOBS));
        MOBS_SIZE = 0;
        memset(WALLS, 0, sizeof(WALLS));
        WALLS_SIZE = 0;
        memset(TARGETS, 0, sizeof(TARGETS));
        TARGETS_SIZE = 0;

        vieworient.yaw = Deg2Rad(90.0f);

        // Player init.
        {
            Entity* entity = AllocateEntity();
            entity->pos = Vec3Lit(0.0f, 0.0f, 0.0f);
            entity->orient = Mat4RotateY(Deg2Rad(90.0f));
            FlagOn(entity->flags, EFLAG_PLAYER_CONTROLLED);

            playerid = entity->id;
        }

        // MOBS init
        {
            EntityId* entities = MOBS;
            u32 size = 0;

            const Vec3 origin = Vec3Lit(-50.0f, 0.0f, 0.0f);

            // Colliders
            {
                const Vec3 positions[] = {origin};
                const Vec3 velocities[] = {Vec3Lit(3.0f, 10.0f, -6.0f)};
                const f32 rotations[] = {Deg2Rad(0.0f)};
                Vec4 color = LPURPLE;
                color.a = 0.16f;
                const Vec4 colors[] = {color};

                for (u32 i = 0; i < ArraySizeOf(positions); ++i, ++size) {
                    Entity* entity = AllocateEntity();
                    entity->pos = positions[i];
                    entity->mesh = GetMesh(M_CUBE);
                    entity->orient = Mat4Rotate(rotations[i], Vec3Lit(0.0f, 1.0f, 0.0f));
                    entity->color = colors[i];
                    entity->scale = Vec3Lit(5.0f, 5.0f, 5.0f);
                    entity->v = velocities[i];
                    FlagOn(entity->flags, EFLAG_COLLIDER);
                    FlagOn(entity->flags, EFLAG_MOB);
                    entities[size] = entity->id;
                }
            }

            MOBS_SIZE = size;
            Assert(MOBS_SIZE < MAX_ENTITY_BATCH);
        }

        // TARGETS init
        {
            EntityId* entities = TARGETS;
            u32 size = 0;

            const Vec3 origin = Vec3Lit(-50.0f, 0.0f, 30.0f);

            {
                const Vec3 positions[] = {origin};
                const Vec3 velocities[] = {Vec3Lit(3.0f, 10.0f, -6.0f)};
                const f32 rotations[] = {Deg2Rad(0.0f)};
                Vec4 color = LRED;
                color.a = 0.16f;
                const Vec4 colors[] = {color};

                for (u32 i = 0; i < ArraySizeOf(positions); ++i, ++size) {
                    Entity* entity = AllocateEntity();
                    entity->pos = positions[i];
                    entity->mesh = GetMesh(M_ICOSAHEDRON);
                    entity->orient = Mat4Rotate(rotations[i], Vec3Lit(0.0f, 1.0f, 0.0f));
                    entity->color = colors[i];
                    entity->scale = Vec3Lit(5.0f, 5.0f, 5.0f);
                    entity->v = velocities[i];
                    FlagOn(entity->flags, EFLAG_COLLIDER);
                    entities[++size] = entity->id;
                    Assert(size < MAX_ENTITY_BATCH);
                }
            }

            TARGETS_SIZE = size;
            Assert(TARGETS_SIZE < ArraySizeOf(TARGETS));
        }

        // walls init.
        {
            static Vec3 positions[] = {Vec3Lit(0.0f, -10.0f, 0.0f)};
            static f32 rotations[] = {Deg2Rad(-90.0f)};
            static Vec3 axis[] = {Vec3Lit(1.0f, 0.0f, 0.0f)};
            Vec4 colors[] = {LORANGE};

            EntityId* entities = WALLS;
            u32* size = &WALLS_SIZE;
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_QUAD);
                entity->orient = Mat4Rotate(rotations[i], axis[i]);
                entity->color = colors[i];
                entity->scale = Vec3Lit(10000.0f, 10000.0f, 10000.0f);
                entities[((*size)++) + i] = entity->id;
            }
        }
    }

    // Update logic.
    {
        Entity* player = GetEntity(playerid);

        // Camera movement.
        {
            static const u8 GAMEPAD_ID = 0;
            static bool FREECAM = false;

            if (IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
                FREECAM = !FREECAM;
            }

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
                // If not freecam, then limiting pitch to 0 will do fps style movement along current plane.
                bodyorient.pitch = FREECAM ? bodyorient.pitch : 0.0f;
                Mat4 rotation = Mat4RotateEuler(bodyorient);

                Vec3 dir_world = Vec3Transform(dir_model, rotation);
                Vec3 mov_world = Vec3Scale(dir_world, vel * dt);
                viewpos = Vec3Add(viewpos, mov_world);
                player->pos = viewpos;
            }

            // Right stick movement.
            {
                f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_X);
                f32 y = -1.0f * GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_Y);
                if (x != 0.0f || y != 0.0f) {
                    static const rad VEL = Deg2Rad(90.0f);

                    // rotate orient.
                    vieworient.yaw += -1.0f * x * VEL * dt;
                    vieworient.pitch += y * VEL * dt;
                    // Clamp pitch to avoid looking too high/low which would sign flip things.
                    vieworient.pitch = f32clamp(vieworient.pitch, Deg2Rad(-89.0f), Deg2Rad(89.0f));
                    player->orient = Mat4RotateEuler(vieworient);
                }
            }
        }

        // Scene3 update.
        {
            {
                EntityManager* em = GetEntityManager();
                u32 esize = ArraySizeOf(em->entities);
                for (u32 i = 0; i < esize; ++i) {
                    Entity* e = GetEntity(i);
                    if (!TestFlag(e->flags, EFLAG_ACTIVE)) {
                        continue;
                    }

                    if (TestFlag(e->flags, EFLAG_MOB)) {
                        if (e->hit) {
                            e->hit_timer += dt;
                            if (e->hit_timer >= 1.2f) {
                                e->hit = false;
                                e->hit_timer = 0.0f;
                            }
                        }

                        // Mob Movement.
                        // TODO: You could imagine something much more intelligent like some
                        // path finding/planner code to drive better interactions.
                        {
                            static const f32 SPEED = 10.0f;
                            static const f32 ANGULAR_SPEED = Deg2Rad(15.0f);

                            Vec3 wishdir = Vec3Normalize(Vec3Sub(player->pos, e->pos));
                            Vec3 dir = Vec3Normalize(Vec3Transform(Vec3Lit(0.0f, 0.0f, -1.0f), e->orient));
                            f32 diff = Vec3Angle(dir, wishdir);
                            if (diff != 0.0f) {
                                Vec3 axis = Vec3Cross(dir, wishdir);

                                i32 flip = diff > 0.0f ? 1.0f : -1.0f;
                                diff = f32abs(diff);
                                f32 to_rotate = f32min(diff, ANGULAR_SPEED * dt);

                                Mat4 rot = Mat4Rotate(flip * to_rotate, axis);
                                dir = Vec3Normalize(Vec3Transform(dir, rot));
                                e->orient = Mat4PointAt(dir);
                            }

                            bool should_move = !e->hit && (diff <= ANGULAR_SPEED * 1.4f);
                            e->v = Vec3Scale(Vec3Transform(Vec3Lit(0.0f, 0.0f, -1.0f), e->orient), SPEED * should_move);
                            e->pos = Vec3Add(e->pos, Vec3Scale(e->v, dt));
                        }
                    }

                    if (TestFlag(e->flags, EFLAG_PROJECTILE)) {
                        e->pos = Vec3Add(e->pos, Vec3Scale(e->v, dt));
                    }

                    if (TestFlag(e->flags, EFLAG_PLAYER_CONTROLLED)) {
                        if (IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) {
                            Entity* projectile = AllocateEntity();
                            projectile->pos = player->pos;
                            projectile->mesh = GetMesh(M_ICOSPHERE);
                            projectile->color = LPURPLE;
                            projectile->scale = Vec3Lit(0.3f, 0.3f, 0.3f);
                            FlagOn(projectile->flags, EFLAG_PROJECTILE);
                            FlagOn(projectile->flags, EFLAG_COLLIDER);
                            projectile->v =
                                Vec3Scale(Vec3Transform(Vec3Lit(0.0f, 0.0f, -1.0f), player->orient), 200.0f);
                        }
                    }
                }

                // Draw.
                for (u32 i = 0; i < ArraySizeOf(em->entities); ++i) {
                    Entity* e = GetEntity(i);
                    if (!TestFlag(e->flags, EFLAG_ACTIVE) || !TestFlag(e->flags, EFLAG_DRAW)) {
                        continue;
                    }

                    Vec4 color = e->color;
                    if (TestFlag(e->flags, EFLAG_MOB) && e->hit) {
                        static const f32 FREQ = 2.5f;
                        f32 t = 0.5f * (1.0f + f32sin(2.0f * LPI * FREQ * e->hit_timer));
                        color.r = f32lerp(color.r, LRED.r, t);
                        color.g = f32lerp(color.g, LRED.g, t);
                        color.b = f32lerp(color.b, LRED.b, t);
                        color.a = f32lerp(color.a, LRED.a, t);
                    }

                    Mat4 scale = Mat4ScaleV(e->scale);
                    Mat4 translate = Mat4TranslateV(e->pos);
                    Mat4 model = Mat4Multiply(translate, Mat4Multiply(e->orient, scale));
                    AddCmd(e->mesh, model, color);
                    DrawCmd* cmd2 = AddCmd(e->mesh, model, color);
                    cmd2->wireframe = true;

                    if (gstate->debug && TestFlag(e->flags, EFLAG_COLLIDER)) {
                        Sphere sphere = EntityBoundingSphere(e);
                        DrawSphereWf(sphere.c, sphere.r, LYELLOW);
                    }
                }
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

        Mat4 view = GetCameraMat(viewpos, vieworient);
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
            glUniformMatrix4fv(shader.vview_loc, 1, GL_FALSE, (const f32*)&view.m);
            glUniformMatrix4fv(shader.vproj_loc, 1, GL_FALSE, (const f32*)&proj.m);
        }

        for (u32 i = 0; i < CBUFFER_SIZE; ++i) {
            DrawCmd* cmd = &CBUFFER[i];
            glUniformMatrix4fv(shader.vmodel_loc, 1, GL_FALSE, (const f32*)&cmd->modelm.m);
            glUniform4fv(shader.vucolor_loc, 1, (const f32*)&cmd->color.f);
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

static void Scene4Tick(GameState* gstate) {
    static Vec3 viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);
    static Vec3 vieworient = Vec3Lit(0.0f, 0.0f, 0.0f);

    static Vec3 GRAVITY = Vec3Lit(0.0f, -9.81f, 0.0f);

    static EntityId playerid;

    static EntityId BALLS[MAX_ENTITY_BATCH] = {0};
    static u32 BALLS_SIZE = 0;

    static EntityId WALLS[MAX_ENTITY_BATCH] = {0};
    static u32 WALLS_SIZE = 0;

    f32 dt = gstate->dt;

    // Init logic.
    if (!gstate->init) {
        viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);
        vieworient = Vec3Lit(0.0f, 0.0f, 0.0f);

        memset(WALLS, 0, sizeof(WALLS));
        WALLS_SIZE = 0;
        memset(BALLS, 0, sizeof(BALLS));
        BALLS_SIZE = 0;

        vieworient.yaw = Deg2Rad(90.0f);

        // Player init.
        {
            Entity* entity = AllocateEntity();
            entity->pos = Vec3Lit(0.0f, 10.0f, 0.0f);
            entity->orient = Mat4RotateY(Deg2Rad(90.0f));
            entity->scale = Vec3Lit(15.0f, 15.0f, 15.0f);
            // entity->mesh = GetMesh(M_ICOSPHERE);
            // entity->color = LCLEAR;
            FlagOn(entity->flags, EFLAG_PLAYER_CONTROLLED);
            // FlagOn(entity->flags, EFLAG_COLLIDER);

            playerid = entity->id;
        }

        // BALLS init
        {
            EntityId* entities = BALLS;
            u32 size = 0;

            const Vec3 origin = Vec3Lit(-50.0f, 10.0f, 0.0f);

            {
                const Vec3 positions[] = {origin};
                const Vec3 velocities[] = {Vec3Lit(3.0f, 10.0f, -6.0f)};
                const f32 rotations[] = {Deg2Rad(0.0f)};
                Vec4 color = LRED;
                color.a = 0.16f;
                const Vec4 colors[] = {color};

                for (u32 i = 0; i < ArraySizeOf(positions); ++i, ++size) {
                    Entity* entity = AllocateEntity();
                    entity->pos = positions[i];
                    entity->mesh = GetMesh(M_ICOSPHERE);
                    entity->orient = Mat4Rotate(rotations[i], Vec3Lit(0.0f, 1.0f, 0.0f));
                    entity->color = colors[i];
                    entity->scale = Vec3Lit(3.0f, 3.0f, 3.0f);
                    entity->v = velocities[i];
                    entity->phtype = PH_SPHERE;
                    entity->last_hit = -1;
                    FlagOn(entity->flags, EFLAG_BALL);
                    FlagOn(entity->flags, EFLAG_COLLIDER);
                    entities[++size] = entity->id;
                    Assert(size < MAX_ENTITY_BATCH);
                }
            }

            BALLS_SIZE = size;
            Assert(BALLS_SIZE < ArraySizeOf(BALLS));
        }

        // walls init.
        {
            u32 width = 20.0f;
            u32 height = 30.0f;
            Vec3 positions[] = {Vec3Lit(0.0f, 0.0f, 0.0f), Vec3Lit(0.0f, height, 0.0f),
                                Vec3Lit(0.0f, 0.0f, -0.5f * width), Vec3Lit(0.0f, 0.0f, 0.5f * width),
                                Vec3Lit(-50.0f, 0.0f, 0.0f)};
            f32 rotations[] = {Deg2Rad(-90.0f), Deg2Rad(90.0f), Deg2Rad(0.0f), Deg2Rad(180.0f), Deg2Rad(90.0f)};
            Vec3 axis[] = {Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),
                           Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f)};
            Vec4 colors[] = {LORANGE, LBLUE, LGRAY, LGRAY, LGRAY};

            EntityId* entities = WALLS;
            u32 size = WALLS_SIZE;
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_QUAD);
                entity->orient = Mat4Rotate(rotations[i], axis[i]);
                entity->color = colors[i];
                entity->scale = Vec3Lit(10000.0f, 10000.0f, 0.0f);
                entity->phtype = PH_AABB;
                entity->friction = 0.7f;
                entity->restitution = 0.8f;
                FlagOn(entity->flags, EFLAG_COLLIDER);
                FlagOn(entity->flags, EFLAG_WALL);
                entities[++size] = entity->id;
                Assert(size < MAX_ENTITY_BATCH);
            }

            WALLS_SIZE = size;
            Assert(WALLS_SIZE < ArraySizeOf(WALLS));
        }
    }

    // Update logic.
    {
        Entity* player = GetEntity(playerid);

        // Camera movement.
        {
            static const u8 GAMEPAD_ID = 0;
            static bool FREECAM = false;

            if (IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
                FREECAM = !FREECAM;
            }

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
                // If not freecam, then limiting pitch to 0 will do fps style movement along current plane.
                bodyorient.pitch = FREECAM ? bodyorient.pitch : 0.0f;
                Mat4 rotation = Mat4RotateEuler(bodyorient);

                Vec3 dir_world = Vec3Transform(dir_model, rotation);
                Vec3 mov_world = Vec3Scale(dir_world, vel * dt);
                player->pos = Vec3Add(player->pos, mov_world);
            }

            // Right stick movement.
            {
                f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_X);
                f32 y = -1.0f * GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_Y);
                if (x != 0.0f || y != 0.0f) {
                    static const rad VEL = Deg2Rad(90.0f);

                    // rotate orient.
                    vieworient.yaw += -1.0f * x * VEL * dt;
                    vieworient.pitch += y * VEL * dt;
                    // Clamp pitch to avoid looking too high/low which would sign flip things.
                    vieworient.pitch = f32clamp(vieworient.pitch, Deg2Rad(-89.0f), Deg2Rad(89.0f));
                    player->orient = Mat4RotateEuler(vieworient);
                }
            }
        }

        // Scene4 update.
        {
            {
                EntityManager* em = GetEntityManager();
                u32 esize = ArraySizeOf(em->entities);
                for (u32 i = 0; i < esize; ++i) {
                    Entity* e = GetEntity(i);
                    if (!TestFlag(e->flags, EFLAG_ACTIVE)) {
                        continue;
                    }

                    if (TestFlag(e->flags, EFLAG_PHYSICS_ON)) {
                        e->v = Vec3Add(e->v, Vec3Scale(GRAVITY, dt));
                        e->pos = Vec3Add(e->pos, Vec3Scale(e->v, dt));
                    }

                    if (TestFlag(e->flags, EFLAG_BALL)) {
                        e->v = Vec3Add(e->v, Vec3Scale(GRAVITY, dt));
                        e->pos = Vec3Add(e->pos, Vec3Scale(e->v, dt));
                    }

                    if (TestFlag(e->flags, EFLAG_PLAYER_CONTROLLED)) {
                        {
                            static Entity* BUMPER = NULL;
                            if (IsGamepadButtonDown(GAMEPAD_ID, GAMEPAD_BUTTON_LEFT_TRIGGER_2)) {
                                if (BUMPER == NULL) {
                                    Entity* enew = AllocateEntity();
                                    enew->mesh = GetMesh(M_CUBE);
                                    enew->scale = Vec3Lit(1000.0f, 2.0f, 1000.0f);
                                    enew->pos = ORIGIN;
                                    enew->phtype = PH_AABB;
                                    enew->friction = 1.0f;
                                    enew->restitution = 1.2f;
                                    FlagOn(enew->flags, EFLAG_COLLIDER);
                                    FlagOn(enew->flags, EFLAG_BUMPER);
                                    BUMPER = enew;
                                }
                            } else if (BUMPER != NULL) {
                                FlagOff(BUMPER->flags, EFLAG_ACTIVE);
                                BUMPER = NULL;
                            }
                        }

                        {
                            static Entity* BUMPER = NULL;
                            if (IsGamepadButtonDown(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) {
                                if (BUMPER == NULL) {
                                    Entity* enew = AllocateEntity();
                                    enew->mesh = GetMesh(M_CUBE);
                                    enew->scale = Vec3Lit(10.0f, 20.0f, 10.0f);
                                    enew->phtype = PH_AABB;
                                    enew->friction = 1.0f;
                                    enew->restitution = 1.2f;
                                    enew->color = LRED;
                                    FlagOn(enew->flags, EFLAG_COLLIDER);
                                    FlagOn(enew->flags, EFLAG_BUMPER);
                                    FlagOff(enew->flags, EFLAG_DRAW);
                                    BUMPER = enew;
                                }

                                Vec3 bodyorient = vieworient;
                                bodyorient.pitch = 0.0f;
                                Mat4 m = Mat4RotateEuler(bodyorient);
                                m = Mat4Multiply(Mat4TranslateV(e->pos), m);
                                BUMPER->pos = Vec3WTransform(Vec3Scale(FORWARD, 10.0f), m);
                            } else if (BUMPER != NULL) {
                                FlagOff(BUMPER->flags, EFLAG_ACTIVE);
                                BUMPER = NULL;
                            }
                        }
                    }
                }

                // Collisions.
                {
                    CollisionInfo collisions[10] = {0};
                    u32 csize = 0;

                    // Detection.
                    u32 size = ArraySizeOf(em->entities);
                    for (u32 i = 0; i < size; ++i) {
                        Entity* e0 = GetEntity(i);
                        if (!TestFlag(e0->flags, EFLAG_ACTIVE) || !TestFlag(e0->flags, EFLAG_COLLIDER)) {
                            continue;
                        }

                        Polyhedron p0 = EntityBoundingVolume(e0);
                        for (u32 j = i + 1; j < size; ++j) {
                            Entity* e1 = GetEntity(j);
                            if (!TestFlag(e1->flags, EFLAG_ACTIVE) || !TestFlag(e1->flags, EFLAG_COLLIDER)) {
                                continue;
                            }

                            Polyhedron p1 = EntityBoundingVolume(e1);

                            CollisionInfo collision = {0};
                            bool collides = TestPolyhedronVsPolyhedron(&p0, &p1, &collision.normal, &collision.depth);
                            if (collides) {
                                collision.id0 = e0->id;
                                collision.id1 = e1->id;
                                collisions[csize++] = collision;
                                Assert(csize < 10);
                            }
                        }
                    }

                    // Response.
                    for (u32 i = 0; i < csize; ++i) {
                        CollisionInfo* collision = &collisions[i];
                        Entity* e0 = GetEntity(collision->id0);
                        Entity* e1 = GetEntity(collision->id1);
                        {
                            bool e0ball = TestFlag(e0->flags, EFLAG_BALL);
                            bool e0wall = TestFlag(e0->flags, EFLAG_WALL);
                            bool e1ball = TestFlag(e1->flags, EFLAG_BALL);
                            bool e1wall = TestFlag(e1->flags, EFLAG_WALL);
                            if ((e0ball && e1wall) || (e0wall && e1ball)) {
                                Vec3 normal = collision->normal;
                                f32 depth = collision->depth;
                                Entity* eball = e0;
                                Entity* ewall = e1;
                                if (e0wall && e1ball) {
                                    eball = e1;
                                    ewall = e0;
                                    normal = Vec3Negate(normal);
                                }

                                eball->pos = Vec3Add(eball->pos, Vec3Scale(normal, -depth));
                                // Reflect velocity about the contact normal.
                                // Remember that to reflect a vec about a normal you add the portion fo the vec in the
                                // per to the negative of the portion in the parallel.
                                // NOTE: That parallel and perp are in relation to the colission *normal* rather than
                                // the surface. some texts treat it the other way around.
                                Vec3 v_parallel = Vec3Scale(normal, Vec3Dot(eball->v, normal));
                                Vec3 v_perp = Vec3Sub(eball->v, v_parallel);
                                eball->v = Vec3Add(Vec3Scale(v_perp, 1.0f), Vec3Scale(v_parallel, -1.0f));

                                eball->last_hit = ewall->id;
                            }
                        }

                        {
                            bool e0ball = TestFlag(e0->flags, EFLAG_BALL);
                            bool e0bumper = TestFlag(e0->flags, EFLAG_BUMPER);
                            bool e1ball = TestFlag(e1->flags, EFLAG_BALL);
                            bool e1bumper = TestFlag(e1->flags, EFLAG_BUMPER);
                            if ((e0ball && e1bumper) || (e0bumper && e1ball)) {
                                static const f32 RETURN_SPEED = 30.0f;
                                static const f32 RETURN_ANGLE_RAD = Deg2Rad(10.0f);

                                Vec3 normal = collision->normal;
                                Entity* eball = e0;
                                Entity* ebumper = e1;
                                if (e0bumper && e1ball) {
                                    eball = e1;
                                    ebumper = e0;
                                    normal = Vec3Negate(normal);
                                }

                                if (eball->last_hit != ebumper->id) {
                                    Vec3 bodyorient = vieworient;
                                    bodyorient.pitch = RETURN_ANGLE_RAD;
                                    Mat4 rotate = Mat4RotateEuler(bodyorient);
                                    normal = Vec3Normalize(Vec3Transform(FORWARD, rotate));
                                    eball->v = Vec3Scale(normal, RETURN_SPEED);
                                }

                                eball->last_hit = ebumper->id;
                            }
                        }

                        {
                            bool e0physics = TestFlag(e0->flags, EFLAG_PHYSICS_ON);
                            bool e0wall = TestFlag(e0->flags, EFLAG_WALL);
                            bool e1physics = TestFlag(e1->flags, EFLAG_PHYSICS_ON);
                            bool e1wall = TestFlag(e1->flags, EFLAG_WALL);
                            if ((e0physics && e1wall) || (e0wall && e1physics)) {
                                Vec3 normal = collision->normal;
                                f32 depth = collision->depth;
                                Entity* ephysics = e0;
                                Entity* ewall = e1;
                                if (e0wall && e1physics) {
                                    ephysics = e1;
                                    ewall = e0;
                                    normal = Vec3Negate(normal);
                                }

                                ephysics->pos = Vec3Add(ephysics->pos, Vec3Scale(normal, -depth));
                                // Reflect velocity about the contact normal.
                                // Remember that to reflect a vec about a normal you add the portion fo the vec in the
                                // per to the negative of the portion in the parallel.
                                // NOTE: That parallel and perp are in relation to the colission *normal* rather than
                                // the surface. some texts treat it the other way around.
                                Vec3 v_parallel = Vec3Scale(normal, Vec3Dot(ephysics->v, normal));
                                Vec3 v_perp = Vec3Sub(ephysics->v, v_parallel);
                                ephysics->v = Vec3Add(Vec3Scale(v_perp, ewall->friction),
                                                      Vec3Scale(v_parallel, -1.0f * ewall->restitution));
                            }
                        }
                    }
                }

                // Draw.
                for (u32 i = 0; i < ArraySizeOf(em->entities); ++i) {
                    Entity* e = GetEntity(i);
                    if (!TestFlag(e->flags, EFLAG_ACTIVE)) {
                        continue;
                    }

                    if (TestFlag(e->flags, EFLAG_DRAW)) {
                        Vec4 color = e->color;
                        if (TestFlag(e->flags, EFLAG_MOB) && e->hit) {
                            static const f32 FREQ = 2.5f;
                            f32 t = 0.5f * (1.0f + f32sin(2.0f * LPI * FREQ * e->hit_timer));
                            color.r = f32lerp(color.r, LRED.r, t);
                            color.g = f32lerp(color.g, LRED.g, t);
                            color.b = f32lerp(color.b, LRED.b, t);
                            color.a = f32lerp(color.a, LRED.a, t);
                        }

                        Mat4 scale = Mat4ScaleV(e->scale);
                        Mat4 translate = Mat4TranslateV(e->pos);
                        Mat4 model = Mat4Multiply(translate, Mat4Multiply(e->orient, scale));
                        AddCmd(e->mesh, model, color);
                        DrawCmd* cmd2 = AddCmd(e->mesh, model, color);
                        cmd2->wireframe = true;
                    }

                    if (gstate->debug && TestFlag(e->flags, EFLAG_COLLIDER)) {
                        Sphere sphere = EntityBoundingSphere(e);
                        DrawSphereWf(sphere.c, sphere.r, LYELLOW);

                        AABB aabb = EntityAABB(e);
                        DrawAABBWf(aabb.c, aabb.r, LBLUE);
                    }
                }
            }
        }

        // Update camera pos at the end of all this to make sure player pos is final position for frame.
        viewpos = player->pos;
        if (gstate->debug) {
            // TraceLog(LOG_INFO, "pos: %f %f %f", player->pos.x, player->pos.y, player->pos.z);
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

        Mat4 view = GetCameraMat(viewpos, vieworient);
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
            glUniformMatrix4fv(shader.vview_loc, 1, GL_FALSE, (const f32*)&view.m);
            glUniformMatrix4fv(shader.vproj_loc, 1, GL_FALSE, (const f32*)&proj.m);
        }

        for (u32 i = 0; i < CBUFFER_SIZE; ++i) {
            DrawCmd* cmd = &CBUFFER[i];
            glUniformMatrix4fv(shader.vmodel_loc, 1, GL_FALSE, (const f32*)&cmd->modelm.m);
            glUniform4fv(shader.vucolor_loc, 1, (const f32*)&cmd->color.f);
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

static void Scene5Tick(GameState* gstate) {
    static Vec3 viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);
    static Vec3 vieworient = Vec3Lit(0.0f, 0.0f, 0.0f);

    static const Vec4 COLOR0 = RGBLit(181.0f, 141.0f, 182.0f);
    static const Vec4 COLOR1 = RGBLit(93.0f, 46.0f, 70.0f);
    static const Vec4 COLOR2 = RGBLit(173.0f, 106.0f, 108.0f);
    static const Vec4 COLOR3 = RGBLit(208.0f, 173.0f, 167.0f);
    static const Vec4 COLOR4 = RGBLit(232.0f, 214.0f, 203.0f);

    static EntityId playerid;

    f32 dt = gstate->dt;

    // Init logic.
    if (!gstate->init) {
        viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);
        vieworient = Vec3Lit(0.0f, 0.0f, 0.0f);

        // Player init.
        {
            Entity* entity = AllocateEntity();
            entity->pos = Vec3Lit(0.0f, -17.5f, 0.0f);
            entity->scale = Vec3Lit(15.0f, 15.0f, 15.0f);
            entity->mesh = GetMesh(M_CUBE);
            entity->color = COLOR3;
            entity->scale = Vec3Lit(15.0f, 0.5f, 1.0f);
            entity->phtype = PH_AABB;
            FlagOn(entity->flags, EFLAG_PLAYER_CONTROLLED);
            FlagOn(entity->flags, EFLAG_COLLIDER);

            playerid = entity->id;
        }

        // BALLS init
        {
            Vec3 origin = Vec3Lit(0.0f, -5.0f, 0.0f);

            {
                const Vec3 positions[] = {origin};

                Entity* player = GetEntity(playerid);
                Vec3 v = Vec3Scale(Vec3Normalize(Vec3Sub(player->pos, origin)), 30.0f);
                const Vec3 velocities[] = {v};

                for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                    Entity* entity = AllocateEntity();
                    entity->pos = positions[i];
                    entity->mesh = GetMesh(M_ICOSPHERE);
                    entity->color = COLOR4;
                    entity->scale = Vec3Lit(0.5f, 0.5f, 0.5f);
                    entity->v = velocities[i];
                    entity->phtype = PH_SPHERE;
                    entity->last_hit = -1;
                    FlagOn(entity->flags, EFLAG_BALL);
                    FlagOn(entity->flags, EFLAG_COLLIDER);
                }
            }
        }

        // walls init.
        {
            i32 xoffset = 35.0f;
            i32 yoffset = 20.0f;
            u32 width = 1.0f;
            u32 height = 200.0f;
            u32 depth = 10.0f;

            Vec3 positions[] = {Vec3Lit(xoffset, 0.0f, 0.0f), Vec3Lit(-xoffset, 0.0f, 0.0f),
                                Vec3Lit(1.0f, yoffset, 0.0f), Vec3Lit(1.0f, -yoffset, 0.0f)};
            Vec3 scales[] = {Vec3Lit(width, height, depth), Vec3Lit(width, height, depth),
                             Vec3Lit(height, width, depth), Vec3Lit(height, width, depth)};

            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_CUBE);
                entity->color = COLOR2;
                entity->scale = scales[i];
                entity->phtype = PH_AABB;
                entity->friction = 0.7f;
                entity->restitution = 0.8f;
                entity->phtype = PH_AABB;
                FlagOn(entity->flags, EFLAG_WALL);
                FlagOn(entity->flags, EFLAG_COLLIDER);
            }
        }

        // blocks init.
        {
            Vec3 scale = Vec3Lit(2.0f, 2.0f, 2.0f);
            Mat4 m = Mat4ScaleV(scale);

            Vec3 vs[MAX_VERTICES] = {0};
            LMesh mesh = GetMesh(M_CUBE);
            u32 vsize = mesh.ntriangles * 3;
            Assert(vsize < MAX_VERTICES);
            for (u32 i = 0; i < vsize; ++i) {
                Vec3 pos = LMeshGetVertex(mesh, i).pos;
                vs[i] = Vec3Transform(pos, m);
            }
            AABB aabb = ComputeAABB(vs, vsize);

            Vec3 xoffset = Vec3Lit(2.0f * aabb.r.f[0], 0.0f, 0.0f);
            Vec3 yoffset = Vec3Lit(0.0f, 2.0f * aabb.r.f[1], 0.0f);

            u32 rsize = 20;
            Vec3 rowstart = Vec3Lit(-(rsize * aabb.r.f[0]), 0.0f, 0.0f);

            for (u32 row = 0; row < 5; ++row) {
                Vec3 pos = rowstart;
                for (u32 i = 0; i < rsize; ++i) {
                    Entity* entity = AllocateEntity();
                    entity->pos = pos;
                    entity->mesh = GetMesh(M_CUBE);
                    entity->color = i % 2 == 0 ? COLOR0 : COLOR2;
                    entity->scale = scale;
                    entity->phtype = PH_AABB;
                    entity->friction = 0.7f;
                    entity->restitution = 0.8f;
                    entity->phtype = PH_AABB;
                    FlagOn(entity->flags, EFLAG_BLOCK);
                    FlagOn(entity->flags, EFLAG_COLLIDER);
                    entity->health = 1;

                    pos = Vec3Add(pos, xoffset);
                }
                rowstart = Vec3Add(rowstart, yoffset);
            }
        }
    }

    // Scene5 update.
    {
        if (gstate->debug) {
            static const u8 GAMEPAD_ID = 0;
            static bool FREECAM = true;

            if (IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
                FREECAM = !FREECAM;
            }

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
                // If not freecam, then limiting pitch to 0 will do fps style movement along current plane.
                bodyorient.pitch = FREECAM ? bodyorient.pitch : 0.0f;
                Mat4 rotation = Mat4RotateEuler(bodyorient);

                Vec3 dir_world = Vec3Transform(dir_model, rotation);
                Vec3 mov_world = Vec3Scale(dir_world, vel * dt);
                viewpos = Vec3Add(viewpos, mov_world);
            }

            // Right stick movement.
            {
                f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_X);
                f32 y = -1.0f * GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_Y);
                if (x != 0.0f || y != 0.0f) {
                    static const rad VEL = Deg2Rad(90.0f);

                    // rotate orient.
                    vieworient.yaw += -1.0f * x * VEL * dt;
                    vieworient.pitch += y * VEL * dt;
                    // Clamp pitch to avoid looking too high/low which would sign flip things.
                    vieworient.pitch = f32clamp(vieworient.pitch, Deg2Rad(-89.0f), Deg2Rad(89.0f));
                }
            }
        } else {
            viewpos = Vec3Lit(0.0f, 0.0f, 50.0f);
            vieworient = Vec3Zero();
        }

        {
            EntityManager* em = GetEntityManager();
            u32 esize = ArraySizeOf(em->entities);

            if (gstate->update) {
                for (u32 i = 0; i < esize; ++i) {
                    Entity* e = GetEntity(i);
                    if (!TestFlag(e->flags, EFLAG_ACTIVE)) {
                        continue;
                    }

                    e->ppos = e->pos;

                    if (TestFlag(e->flags, EFLAG_BLOCK)) {
                        if (e->health == 0) {
                            // TODO: particles!
                            FlagOff(e->flags, EFLAG_ACTIVE);
                        }
                    }

                    if (TestFlag(e->flags, EFLAG_BALL)) {
                        Vec3 dx = Vec3Scale(e->v, dt);
                        e->pos = Vec3Add(e->pos, dx);
                        // TraceLog(LOG_INFO, "%f %f %f", e->pos.x, e->pos.y, e->pos.z);
                        // TraceLog(LOG_INFO, "v: %f %f %f", e->v.x, e->v.y, e->v.z);
                    }

                    if (TestFlag(e->flags, EFLAG_PLAYER_CONTROLLED)) {
                        static const f32 MOVE_THRESHOLD = 0.05f;
                        static const f32 MAX_SPEED = 40.0f;
                        f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_X);
                        if (f32abs(x) > MOVE_THRESHOLD) {
                            Vec3 dx = Vec3Scale(Vec3Lit(1.0f, 0.0f, 0.0f), x * MAX_SPEED * dt);
                            e->pos = Vec3Add(e->pos, dx);
                        }

                        e->straight_shot = IsGamepadButtonDown(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
                    }
                }

                // Collisions.
                {
                    CollisionInfo collisions[10] = {0};
                    u32 csize = 0;

                    // Detection.
                    u32 size = ArraySizeOf(em->entities);
                    for (u32 i = 0; i < size; ++i) {
                        Entity* e0 = GetEntity(i);
                        if (!TestFlag(e0->flags, EFLAG_ACTIVE) || !TestFlag(e0->flags, EFLAG_COLLIDER)) {
                            continue;
                        }

                        Polyhedron p0 = EntityBoundingVolume(e0);
                        for (u32 j = i + 1; j < size; ++j) {
                            Entity* e1 = GetEntity(j);
                            if (!TestFlag(e1->flags, EFLAG_ACTIVE) || !TestFlag(e1->flags, EFLAG_COLLIDER)) {
                                continue;
                            }

                            Polyhedron p1 = EntityBoundingVolume(e1);

                            CollisionInfo collision = {0};
                            bool collides = TestPolyhedronVsPolyhedron(&p0, &p1, &collision.normal, &collision.depth);
                            if (collides) {
                                collision.id0 = e0->id;
                                collision.id1 = e1->id;

                                // // Time of impact calculation. will back up entities to earliest detected collision
                                // // point.
                                // static const u32 ITERATIONS = 3;
                                // f32 startdt = 0.0f;
                                // f32 enddt = 1.0f;
                                // for (u32 itoi = 0; itoi < ITERATIONS; ++itoi) {
                                //     f32 toi = (startdt + enddt) / 2.0f;
                                //     e0->pos = Vec3Lerp(e0->ppos, e0->pos, toi);
                                //     e1->pos = Vec3Lerp(e1->ppos, e1->pos, toi);

                                //     Polyhedron pa = EntityBoundingVolume(e0);
                                //     Polyhedron pb = EntityBoundingVolume(e1);
                                //     CollisionInfo ci = {0};
                                //     bool c = TestPolyhedronVsPolyhedron(&pa, &pb, &ci.normal, &ci.depth);
                                //     if (c) {
                                //         enddt = toi;
                                //         collision = ci;  // Keep track of earliest detected collision.
                                //     } else {
                                //         startdt = toi;
                                //     }
                                // }

                                if (DEBUG_EVER) {
                                    breakpoint();
                                }

                                collisions[csize++] = collision;
                                Assert(csize < 10);
                            }
                        }
                    }

                    // Response.
                    for (u32 i = 0; i < csize; ++i) {
                        CollisionInfo* collision = &collisions[i];
                        Entity* e0 = GetEntity(collision->id0);
                        Entity* e1 = GetEntity(collision->id1);

                        bool e0ball = TestFlag(e0->flags, EFLAG_BALL);
                        bool e0player = TestFlag(e0->flags, EFLAG_PLAYER_CONTROLLED);
                        bool e0wall = TestFlag(e0->flags, EFLAG_WALL);
                        bool e0block = TestFlag(e0->flags, EFLAG_BLOCK);

                        bool e1ball = TestFlag(e1->flags, EFLAG_BALL);
                        bool e1player = TestFlag(e1->flags, EFLAG_PLAYER_CONTROLLED);
                        bool e1wall = TestFlag(e1->flags, EFLAG_WALL);
                        bool e1block = TestFlag(e1->flags, EFLAG_BLOCK);

                        {
                            bool flip = e0player && e1ball;
                            if ((e0ball && e1player) || flip) {
                                // static const f32 RETURN_SPEED = 30.0f;
                                // static const f32 RETURN_ANGLE_RAD = Deg2Rad(10.0f);

                                Vec3 normal = collision->normal;
                                Entity* eball = e0;
                                Entity* eplayer = e1;
                                if (flip) {
                                    eball = e1;
                                    eplayer = e0;
                                    normal = Vec3Negate(normal);
                                }

                                eball->pos = Vec3Add(eball->pos, Vec3Scale(normal, collision->depth));

                                {
                                    Vec3 vnew = eball->v;

                                    if (eplayer->straight_shot) {
                                        f32 ballspeed = Vec3Len(eball->v);
                                        vnew = Vec3Scale(normal, ballspeed);
                                    } else {
                                        Vec3 vdir = Vec3Normalize(eball->v);
                                        f32 parallel = Vec3Dot(normal, vdir);
                                        if (!f32eq(f32abs(parallel), 1.0f)) {
                                            // Reflect velocity about the contact normal.
                                            // Remember that to reflect a vec about a normal you add the portion fo the
                                            // vec in the per to the negative of the portion in the parallel. NOTE: That
                                            // parallel and perp are in relation to the colission *normal* rather than
                                            // the surface. some texts treat it the other way around.
                                            Vec3 v_parallel = Vec3Scale(normal, Vec3Dot(eball->v, normal));
                                            Vec3 v_perp = Vec3Sub(eball->v, v_parallel);
                                            vnew = Vec3Add(Vec3Scale(v_perp, 1.0f), Vec3Scale(v_parallel, -1.0f));

                                            // Sphere s = EntityBoundingVolume(eball).sphere;
                                            // Vec3 normball = Vec3Negate(normal);
                                            // Vec3 cpball = Vec3Add(s.c, Vec3Scale(normball, s.r));

                                            // AABB aabb = EntityBoundingVolume(eplayer).aabb;
                                            // Vec3 center = aabb.c;
                                            // center.x += aabb.r.x * normal.x;
                                            // center.y += aabb.r.y * normal.y;
                                            // center.z += aabb.r.z * normal.z;

                                            // f32 dist = Vec3Len(Vec3Sub(cpball, center));
                                            // breakpoint();

                                            // f32 angle = 0.0f;
                                            // if (dist > 0.5f) {
                                            // }

                                            // Vec3 dir =
                                            //     Vec3RotateAxis(normal, Vec3Lit(0.0f, 0.0f, 1.0f), Deg2Rad(angle));
                                            // vnew = Vec3Scale(dir, Vec3Len(eball->v));

                                        } else {
                                            // special case dead on collision for now.
                                            // since we are 2d game logic, just pick the zaxis as the rotation axis.
                                            Vec3 dir =
                                                Vec3RotateAxis(normal, Vec3Lit(0.0f, 0.0f, 1.0f), Deg2Rad(-45.0f));
                                            vnew = Vec3Scale(dir, Vec3Len(eball->v));
                                        }
                                    }

                                    eball->v = vnew;
                                }

                                eball->last_hit = eplayer->id;
                            }
                        }

                        {
                            bool flip = e0block && e1ball;
                            if ((e0ball && e1block) || flip) {
                                Vec3 normal = collision->normal;
                                Entity* eball = e0;
                                Entity* eblock = e1;
                                if (flip) {
                                    eball = e1;
                                    eblock = e0;
                                    normal = Vec3Negate(normal);
                                }

                                eball->pos = Vec3Add(eball->pos, Vec3Scale(normal, collision->depth));

                                {
                                    Vec3 vnew = eball->v;

                                    Vec3 vdir = Vec3Normalize(eball->v);
                                    f32 parallel = Vec3Dot(normal, vdir);
                                    if (!f32eq(f32abs(parallel), 1.0f)) {
                                        // Reflect velocity about the contact normal.
                                        // Remember that to reflect a vec about a normal you add the portion fo the vec
                                        // in the per to the negative of the portion in the parallel. NOTE: That
                                        // parallel and perp are in relation to the colission *normal* rather than the
                                        // surface. some texts treat it the other way around.
                                        Vec3 v_parallel = Vec3Scale(normal, Vec3Dot(eball->v, normal));
                                        Vec3 v_perp = Vec3Sub(eball->v, v_parallel);

                                        vnew = Vec3Add(Vec3Scale(v_perp, 1.0f), Vec3Scale(v_parallel, -1.0f));
                                    } else {
                                        // special case dead on collision for now.
                                        // since we are 2d game logic, just pick the zaxis as the rotation axis.
                                        Vec3 dir = Vec3RotateAxis(normal, Vec3Lit(0.0f, 0.0f, 1.0f), Deg2Rad(-45.0f));
                                        vnew = Vec3Scale(dir, Vec3Len(eball->v));
                                    }

                                    eball->v = vnew;
                                }

                                eball->last_hit = eblock->id;
                                if (eblock->health != 0) {
                                    --eblock->health;
                                }
                            }
                        }

                        {
                            bool flip = e0wall && e1ball;
                            if ((e0ball && e1wall) || flip) {
                                // static const f32 RETURN_SPEED = 30.0f;
                                // static const f32 RETURN_ANGLE_RAD = Deg2Rad(10.0f);

                                Vec3 normal = collision->normal;
                                Entity* eball = e0;
                                Entity* ewall = e1;
                                if (flip) {
                                    eball = e1;
                                    ewall = e0;
                                    normal = Vec3Negate(normal);
                                }

                                eball->pos = Vec3Add(eball->pos, Vec3Scale(normal, collision->depth));

                                // Reflect velocity about the contact normal.
                                // Remember that to reflect a vec about a normal you add the portion fo the vec in the
                                // per to the negative of the portion in the parallel.
                                // NOTE: That parallel and perp are in relation to the colission *normal* rather than
                                // the surface. some texts treat it the other way around.
                                Vec3 v_parallel = Vec3Scale(normal, Vec3Dot(eball->v, normal));
                                Vec3 v_perp = Vec3Sub(eball->v, v_parallel);
                                eball->v = Vec3Add(Vec3Scale(v_perp, 1.0f), Vec3Scale(v_parallel, -1.0f));

                                eball->last_hit = ewall->id;

                                // Polyhedron pa = EntityBoundingVolume(eball);
                                // Polyhedron pb = EntityBoundingVolume(ewall);
                                // CollisionInfo c = {0};
                                // breakpoint();
                                // if (TestPolyhedronVsPolyhedron(&pa, &pb, &c.normal, &c.depth)) {
                                //     breakpoint();
                                // }
                            }
                        }

                        {
                            bool flip = e0wall && e1player;
                            if ((e0player && e1wall) || flip) {
                                Vec3 normal = collision->normal;
                                Entity* eplayer = e0;
                                Entity* ewall = e1;
                                if (flip) {
                                    eplayer = e1;
                                    ewall = e0;
                                    normal = Vec3Negate(normal);
                                }

                                eplayer->pos = Vec3Add(eplayer->pos, Vec3Scale(normal, collision->depth));
                            }
                        }
                    }
                }
            }

            // Draw background
            {
                AddCmd(GetMesh(M_QUAD), Mat4ScaleV(Vec3Lit(10000.0f, 10000.0f, 1.0f)), COLOR1);
            }

            // Draw.
            for (u32 i = 0; i < ArraySizeOf(em->entities); ++i) {
                Entity* e = GetEntity(i);
                if (!TestFlag(e->flags, EFLAG_ACTIVE)) {
                    continue;
                }

                if (TestFlag(e->flags, EFLAG_DRAW)) {
                    Vec4 color = e->color;
                    if (TestFlag(e->flags, EFLAG_MOB) && e->hit) {
                        static const f32 FREQ = 2.5f;
                        f32 t = 0.5f * (1.0f + f32sin(2.0f * LPI * FREQ * e->hit_timer));
                        color.r = f32lerp(color.r, LRED.r, t);
                        color.g = f32lerp(color.g, LRED.g, t);
                        color.b = f32lerp(color.b, LRED.b, t);
                        color.a = f32lerp(color.a, LRED.a, t);
                    }

                    Mat4 scale = Mat4ScaleV(e->scale);
                    Mat4 translate = Mat4TranslateV(e->pos);
                    Mat4 model = Mat4Multiply(translate, Mat4Multiply(e->orient, scale));
                    AddCmd(e->mesh, model, color);
                    DrawCmd* cmd2 = AddCmd(e->mesh, model, color);
                    cmd2->wireframe = true;
                }

                if (gstate->debug && TestFlag(e->flags, EFLAG_COLLIDER)) {
                    Sphere sphere = EntityBoundingSphere(e);
                    DrawSphereWf(sphere.c, sphere.r, LYELLOW);

                    AABB aabb = EntityAABB(e);
                    DrawAABBWf(aabb.c, aabb.r, LBLUE);
                }
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

        Mat4 view = GetCameraMat(viewpos, vieworient);
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
            glUniformMatrix4fv(shader.vview_loc, 1, GL_FALSE, (const f32*)&view.m);
            glUniformMatrix4fv(shader.vproj_loc, 1, GL_FALSE, (const f32*)&proj.m);
        }

        for (u32 i = 0; i < CBUFFER_SIZE; ++i) {
            DrawCmd* cmd = &CBUFFER[i];
            glUniformMatrix4fv(shader.vmodel_loc, 1, GL_FALSE, (const f32*)&cmd->modelm.m);
            glUniform4fv(shader.vucolor_loc, 1, (const f32*)&cmd->color.f);
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

static void GameStateInit(GameState* gstate) {
    memset(gstate, 0, sizeof(GameState));

    InitWindow(RenderWidth, RenderHeight, "Game");
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetConfigFlags(FLAG_VSYNC_HINT);
    SetTargetFPS(60);
    DisableCursor();

    {
        glEnable(GL_DEPTH_TEST);
    }

    // Register scenes.
    {
        u32 idx = 0;
        gstate->scenefuncs[idx++] = Scene1Tick;
        gstate->scenefuncs[idx++] = Scene2Tick;
        gstate->scenefuncs[idx++] = Scene3Tick;
        gstate->scenefuncs[idx++] = Scene4Tick;
        gstate->scenefuncs[idx++] = Scene5Tick;
        Assert(idx < MAX_SCENES);
        gstate->scenefuncssize = idx;
    }
    GameStateLoadScene(gstate, 0);
}

static void GameStateTick(GameState* gstate) {
    gstate->dt = GetFrameTime();
    f32 dt = gstate->dt;

    // Check if scene needs to be changed.
    {
        bool right = IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
        bool left = IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_LEFT_TRIGGER_1);

        if ((right || left) && !(right && left)) {
            // Need signed id for ring buffer wraparound on left.
            i32 sceneid = gstate->sceneid;
            if (right) {
                sceneid = (sceneid + 1) % gstate->scenefuncssize;
            } else {
                sceneid = ((sceneid + gstate->scenefuncssize) - 1) % gstate->scenefuncssize;
            }

            GameStateLoadScene(gstate, sceneid);
        }
    }

    gstate->timer += dt;

    {
        static bool ENABLE_DEBUG_KEYMAP = false;

        if (IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
            ENABLE_DEBUG_KEYMAP = !ENABLE_DEBUG_KEYMAP;
        }

        if (ENABLE_DEBUG_KEYMAP) {
            bool debugkey = IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_ONE);
            bool debuggp = IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_LEFT_TRIGGER_2);
            if (debugkey || debuggp) {
                gstate->debug = !gstate->debug;
                gstate->update = !gstate->debug;
                gstate->debug_ever = true;
                DEBUG_EVER = true;
            }
            if (gstate->debug) {
                bool updatekey = IsKeyPressed(KEY_U);
                bool updategp = IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
                if (updatekey || updategp) {
                    gstate->update = !gstate->update;
                }
            }
        }
    }

    // Tick scene.
    gstate->scenefuncs[gstate->sceneid](gstate);

    gstate->init = true;
    ++gstate->nframe;
}

static void TestL();

int main(void) {
    TestL();

    GameStateInit(&GState);

    while (!WindowShouldClose()) {
        GameStateTick(&GState);
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

            // Test case 4: Perspective matrix with small near and large far planes (30-degree FOV, 4:3 aspect
            // ratio)
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
