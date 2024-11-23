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
#define f32acos acos
#define f32eq(x, y) (f32abs((x) - (y)) < (TOL))

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

typedef struct Vec3 {
    union {
        struct {
            f32 x;
            f32 y;
            f32 z;
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

static Mat4 Mat4Translate(f32 x, f32 y, f32 z) {
    return Mat4Lit(1.0f, 0.0f, 0.0f, x,    //
                   0.0f, 1.0f, 0.0f, y,    //
                   0.0f, 0.0f, 1.0f, z,    //
                   0.0f, 0.0f, 0.0f, 1.0f  //
    );
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
    f32 x;
    f32 y;
    f32 z;
    f32 w;
} Vec4;

#define Vec4Lit(x, y, z, w) ((Vec4){(x), (y), (z), (w)})

static Vec4 Mat4TransformVec4(Mat4 m, f32 x, f32 y, f32 z, f32 w) {
    return Vec4Lit((x * m.mm.m00) + (x * m.mm.m01) + (x + m.mm.m02) + (x * m.mm.m03),  //
                   (y * m.mm.m10) + (y * m.mm.m11) + (y + m.mm.m12) + (y * m.mm.m13),  //
                   (z * m.mm.m20) + (z * m.mm.m21) + (z + m.mm.m22 + (z * m.mm.m23)),  //
                   (w * m.mm.m30) + (w * m.mm.m31) + (w + m.mm.m32 + (w * m.mm.m33))   //

    );
}

static Vec3 Vec3Transform(Vec3 v, Mat4 m) {
    return Vec3Lit((v.x * m.mm.m00) + (v.y * m.mm.m01) + (v.z * m.mm.m02),  //
                   (v.x * m.mm.m10) + (v.y * m.mm.m11) + (v.z * m.mm.m12),  //
                   (v.x * m.mm.m20) + (v.y * m.mm.m21) + (v.z * m.mm.m22));
}

////////////////////////////////////////////////////////////
// S:Collision

// CCW winding.
// If collision, normal will point in direction of separating axis towards triangle ABC.
bool CollideTriangleTriangle(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec2 e, Vec2 f, Vec2* normal, f32* depth) {
    Vec2 edges[6] = {Vec2Sub(b, a), Vec2Sub(c, b), Vec2Sub(a, c), Vec2Sub(e, d), Vec2Sub(f, e), Vec2Sub(d, f)};

    *depth = F32_MAX;

    // SAT.
    for (u8 i = 0; i < 6; ++i) {
        Vec2 edge = edges[i];
        Vec2 n = Vec2Normalize(Vec2Cross(edge));
        f32 proj_a = Vec2Dot(a, n);
        f32 proj_b = Vec2Dot(b, n);
        f32 proj_c = Vec2Dot(c, n);

        f32 proj_d = Vec2Dot(d, n);
        f32 proj_e = Vec2Dot(e, n);
        f32 proj_f = Vec2Dot(f, n);

        f32 max_abc = f32max(proj_a, f32max(proj_b, proj_c));
        f32 min_def = f32min(proj_d, f32min(proj_e, proj_f));

        if (max_abc < min_def) {
            return false;
        }

        f32 min_abc = f32min(proj_a, f32min(proj_b, proj_c));
        f32 max_def = f32max(proj_d, f32max(proj_e, proj_f));

        if (min_abc > max_def) {
            return false;
        }

        f32 d = f32abs((max_abc - min_abc) - (max_def - min_def));
        if (d < *depth) {
            *depth = d;
            *normal = n;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////
// S:Physics

typedef u32 ParticleId;

typedef u32 ConstraintId;

typedef struct Constraint {
    bool active;
    ParticleId pid_a;
    ParticleId pid_b;
    f32 rest_len;
} Constraint;

typedef u32 BodyId;

typedef struct Body {
    bool active;

    // Only support triangles, CCW order.
    ParticleId pids[3];
} Body;

static void BodyInit(Body* body) {
    static Body ZERO = {0};
    *body = ZERO;
}

#define MAX_PARTICLES 10
#define MAX_CONSTRAINTS 10
#define MAX_ITERATIONS 10
#define MAX_BODIES 10

typedef struct PhysicsState {
    f32 fixed_dt;
    Vec2 gravity;

    bool active[MAX_PARTICLES];
    Vec2 pos[MAX_PARTICLES];
    Vec2 pos_old[MAX_PARTICLES];
    Vec2 force[MAX_PARTICLES];
    Constraint constraints[MAX_CONSTRAINTS];
    Body body[MAX_BODIES];

} PhysicsState;

#define ParticleGetPos(pstate, id) ((pstate)->pos[(id)])
#define GetBody(pstate, bid) (&(pstate)->body[(bid)])
#define GetConstraint(pstate, cid) (&(pstate)->constraints[(cid)])

static void PhysicsStateInit(PhysicsState* pstate) {
    memset(pstate, 0, sizeof(*pstate));
    pstate->fixed_dt = 1.0f / 60.0f;
}

static ParticleId PhysicsStateAddParticle(PhysicsState* pstate, Vec2 pos) {
    for (ParticleId i = 0; i < MAX_PARTICLES; ++i) {
        if (!pstate->active[i]) {
            pstate->active[i] = true;
            pstate->pos[i] = pos;
            pstate->pos_old[i] = pos;
            return i;
        }
    }

    Assert(false);
    return 0;
}

static Constraint* PhysicsStateAllocateConstraint(PhysicsState* pstate) {
    for (u32 i = 0; i < MAX_CONSTRAINTS; ++i) {
        if (!pstate->constraints[i].active) {
            Constraint c = {0};
            Constraint* cc = &pstate->constraints[i];
            *cc = c;
            cc->active = true;
            return cc;
        }
    }

    Assert(false);
    return NULL;
}

static void PhysicsStateAccumulateForces(PhysicsState* pstate) {
    for (ParticleId i = 0; i < MAX_PARTICLES; ++i) {
        pstate->force[i] = Vec2Scale(pstate->gravity, pstate->active[i]);
    }
}
static void PhysicsStateIntegrate(PhysicsState* pstate) {
    f32 dt_squared = pstate->fixed_dt * pstate->fixed_dt;
    for (ParticleId i = 0; i < MAX_PARTICLES; ++i) {
        // x_new = 2x - x_old + force * dt^2
        Vec2 cur_x = pstate->pos[i];
        Vec2 dx = Vec2Add(Vec2Sub(cur_x, pstate->pos_old[i]), Vec2Scale(pstate->force[i], dt_squared));
        pstate->pos[i] = Vec2Add(cur_x, Vec2Scale(dx, pstate->active[i]));
        pstate->pos_old[i] = cur_x;
    }
}
static void PhysicsStateSatisfyConstraints(PhysicsState* pstate) {
    f32 d = 100.0f;
    Vec2 minv = Vec2Lit(-d, -d);
    Vec2 maxv = Vec2Lit(d, d);
    for (u32 j = 0; j < MAX_ITERATIONS; ++j) {
        for (ParticleId i = 0; i < MAX_PARTICLES; ++i) {
            // Cube constraint.
            pstate->pos[i] = Vec2Clamp(pstate->pos[i], minv, maxv);
        }

        for (u32 k = 0; k < MAX_CONSTRAINTS; ++k) {
            Constraint c = pstate->constraints[k];
            if (!c.active) {
                continue;
            }

            // Stick constraint.
            Vec2 x0 = pstate->pos[c.pid_a];
            Vec2 x1 = pstate->pos[c.pid_b];
            Vec2 delta = Vec2Sub(x0, x1);
            f32 delta_len = Vec2Len(delta);
            f32 diff = (delta_len - c.rest_len) / delta_len;
            pstate->pos[c.pid_a] = Vec2Sub(x0, Vec2Scale(delta, 0.5f * diff));
            pstate->pos[c.pid_b] = Vec2Add(x1, Vec2Scale(delta, 0.5f * diff));
        }

        Vec2 normal = Vec2Zero();
        f32 depth = 0.0f;
        if (CollideTriangleTriangle(pstate->pos[0], pstate->pos[1], pstate->pos[2], pstate->pos[3], pstate->pos[4],
                                    pstate->pos[5], &normal, &depth)) {
            // pstate->active[0] = false;
            // pstate->active[1] = false;
            // pstate->active[2] = false;
            // pstate->active[3] = false;
            // pstate->active[4] = false;
            // pstate->active[5] = false;

            Vec2 disp = Vec2Scale(normal, 0.5f * depth);
            Vec2 neg_disp = Vec2Scale(disp, -1.0f);
            pstate->pos[0] = Vec2Add(pstate->pos[0], disp);
            pstate->pos[1] = Vec2Add(pstate->pos[1], disp);
            pstate->pos[2] = Vec2Add(pstate->pos[2], disp);

            pstate->pos[3] = Vec2Add(pstate->pos[3], neg_disp);
            pstate->pos[4] = Vec2Add(pstate->pos[4], neg_disp);
            pstate->pos[5] = Vec2Add(pstate->pos[5], neg_disp);
        }
    }
}

static void PhysicsStateTick(PhysicsState* pstate) {
    PhysicsStateAccumulateForces(pstate);
    PhysicsStateIntegrate(pstate);
    PhysicsStateSatisfyConstraints(pstate);
}

static void PhysicsStateDraw(PhysicsState* pstate) {
    for (ConstraintId cid = 0; cid < MAX_CONSTRAINTS; ++cid) {
        Constraint* c = GetConstraint(pstate, cid);
        if (!c->active) {
            continue;
        }
        Vec2 pa = ParticleGetPos(pstate, c->pid_a);
        Vec2 pb = ParticleGetPos(pstate, c->pid_b);
        DrawLineEx(Vec2ToRVector2(pa), Vec2ToRVector2(pb), 2, BLACK);
    }

    for (ParticleId i = 0; i < MAX_PARTICLES; ++i) {
        if (!pstate->active[i]) {
            continue;
        }

        Vec2 p = ParticleGetPos(pstate, i);
        DrawCircle(p.x, p.y, 3, RED);
    }
}

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
        "out vec3 fPosEye;\n"
        "out vec4 fColor;\n"
        "out vec3 fNormal;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);\n"
        "   fPosEye = vec3(vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0));\n"
        "   fColor = vec4(vColor.x, vColor.y, vColor.z, 1.0);\n"
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
        "   FragColor = vec4(fColor.xyz, 1.0);\n"
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
        }
    }

    return SHADER;
}

// Shader that will draw light sources without lighting calculations effecting the color.
static LShader GetUnlitShader() {
    static const char* VSRC =
        "#version 330 core\n"
        "layout (location = 0) in vec3 vPos;\n"
        "\n"
        "uniform mat4 vModel;\n"
        "uniform mat4 vView;\n"
        "uniform mat4 vProj;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vProj * vView * vModel * vec4(vPos, 1.0);\n"
        "}\0";

    static const char* FSRC =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "    FragColor = vec4(0.0, 0.0, 0.5, 1.0);\n"
        "}\0";
    static bool INIT = false;
    static LShader SHADER = {0};

    if (!INIT) {
        INIT = true;
        LShaderInit(&SHADER, VSRC, FSRC);

        // Att lookup.
        {
            SHADER.vpos_loc = glGetAttribLocation(SHADER.pid, "vPos");
            SHADER.vmodel_loc = glGetUniformLocation(SHADER.pid, "vModel");
            SHADER.vview_loc = glGetUniformLocation(SHADER.pid, "vView");
            SHADER.vproj_loc = glGetUniformLocation(SHADER.pid, "vProj");
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
    union {
        f32 f[sizeof(Vec3) * 3];

        struct {
            Vec3 pos;
            Vec3 color;
            Vec3 normal;
        };
    };
} Vertex;

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

typedef enum MID { M_CUBE = 0, M_QUAD, M_ISO_TRIANGLE, M_PYRAMID, M_COUNT } MID;

static LMesh GetMesh(u32 mid) {
    Assert(mid < MAX_MESHES);
    return MESHES[mid];
}

static LMesh AddMesh(Vertex* vs, u32 vsize, f32* ts, u32 tsize) {
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

    // ISO Triangle.
    {
        Vertex vs[] = {
            {{{0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},
            {{{-1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},
            {{{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},
        };
        f32 ts[] = {0, 1, 2};
        AddMesh(vs, ArraySizeOf(vs), ts, ArraySizeOf(ts));
    }

    // Pyramid.
    {
        Vertex vs[] = {
            // Base
            {{{-1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f}}},   //
            {{{-1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f}}},  //
            {{{1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f}}},   //
            {{{1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f}}},    //

            // Top point.
            {{{0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}}},  //
        };
        f32 ts[] = {
            // Base
            0, 1, 2,  //
            2, 3, 0,  //

            // top
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
    FlagOn(entity->flags, EFLAG_DRAW);
    return entity;
}

////////////////////////////////////////////////////////////

typedef enum InputDir {
    DIR_NONE = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,

    DIR_COUNT
} InputDir;

typedef struct GameState {
    /////////////////////////////////////////
    // Core fields
    bool debug;
    bool game_over;

    float timer;

    RenderTexture2D target;

    PhysicsState pstate;

    /////////////////////////////////////////
    // Gameplay fields

    struct {
        Entity* head;
        Entity* tail;
    } monsters;
} GameState;

#define RenderWidth(state) ((state)->target.texture.width)
#define RenderHeight(state) ((state)->target.texture.height)
#define GetPhysicsState(gstate) (&(gstate)->pstate)

static GameState GState;

#define GetGameState() (&GState)

static void SimpleScene(GameState* state) {
    // LoadLevel();

    // Entity 0.
    {
        Entity* entity = AllocateEntity();
        FlagOn(entity->flags, EFLAG_PLAYER_CONTROLLED);
        entity->pos = ORIGIN;
    }
}

static void GameStateInit(GameState* gstate) {
    memset(gstate, 0, sizeof(GameState));

    PhysicsState* pstate = &gstate->pstate;
    PhysicsStateInit(pstate);
    pstate->gravity = Vec2Lit(0.0f, -10.0f);

    InitWindow(RenderWidth, RenderHeight, "Game");
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetConfigFlags(FLAG_VSYNC_HINT);
    SetTargetFPS(60);
    EntitySystemInit();

    gstate->target = LoadRenderTexture(RenderWidth, RenderHeight);

    SimpleScene(gstate);

    { glEnable(GL_DEPTH_TEST); }
}

static void DrawBatch(LShader shader, EntityId* entities, u32 size) {
    for (u32 i = 0; i < size; ++i) {
        Entity* entity = GetEntity(entities[i]);
        Mat4 scale = Mat4Scale(entity->scale);
        Mat4 translate = Mat4TranslateV(entity->pos);
        Mat4 model = Mat4Multiply(translate, Mat4Multiply(entity->orient, scale));
        glUniformMatrix4fv(shader.vmodel_loc, 1, GL_FALSE, (const f32*)&model.m);
        glDrawElements(GL_TRIANGLES, entity->mesh.ntriangles * 3, GL_UNSIGNED_INT,
                       (void*)(entity->mesh.ibufferidx * sizeof(GLuint)));
    }
}

static void GameStateUpdate(GameState* gstate, float dt) {
    gstate->timer += dt;

    // PhysicsStateUpdate(&state->pstate, dt);
    if (!IsKeyDown(KEY_SPACE)) {
        PhysicsStateTick(&gstate->pstate);
    }

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
    static Mat4 vieworient = {0};
    static Mat4 viewbody = {0};

    static bool INIT = false;
    static Vec3 LIGHT_POS = Vec3Lit(10.0f, 10.0f, -10.0f);

    static EntityId LIGHTS[MAX_ENTITY_BATCH] = {0};
    static u32 LIGHTS_SIZE = 0;

    static EntityId SPINNING[MAX_ENTITY_BATCH] = {0};
    static u32 SPINNING_SIZE = 0;

    static EntityId SHADOW_DEBUG[MAX_ENTITY_BATCH] = {0};
    static u32 SHADOW_DEBUG_SIZE = 0;

    static EntityId PHYSICS[MAX_ENTITY_BATCH] = {0};
    static u32 PHYSICS_SIZE = 0;

    static EntityId ISO_TRIANGLES[MAX_ENTITY_BATCH] = {0};
    static u32 ISO_TRIANGLES_SIZE = 0;

    static EntityId PYRAMIDS[MAX_ENTITY_BATCH] = {0};
    static u32 PYRAMIDS_SIZE = 0;

    static EntityId WALLS[MAX_ENTITY_BATCH] = {0};
    static u32 WALLS_SIZE = 0;

    if (!INIT) {
        INIT = true;

        RenderDataInit();
        vieworient = Mat4RotateY(Deg2Rad(90.0f));
        viewbody = vieworient;

        {
            Vec3 positions[] = {LIGHT_POS};

            EntityId* entities = LIGHTS;
            LIGHTS_SIZE = ArraySizeOf(positions);
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_CUBE);
                entities[i] = entity->id;
            }
        }

        {
            Vec3 positions[] = {Vec3Lit(0.0f, 0.0f, 0.0f), Vec3Lit(-5.0f, -3.0f, -3.0f), Vec3Lit(15.0f, 3.0f, -50.0f),
                                Vec3Lit(-15.0f, 0.0f, 0.0f)};

            EntityId* entities = SPINNING;
            SPINNING_SIZE = ArraySizeOf(positions);
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_CUBE);
                entities[i] = entity->id;
            }
        }

        {
            Vec3 positions[] = {Vec3Lit(10.0f, 3.0f, 0.0f), Vec3Lit(0.0f, 3.0f, 5.0f)};
            f32 rotations[] = {Deg2Rad(45.0f), Deg2Rad(0.0f)};

            EntityId* entities = SHADOW_DEBUG;
            SHADOW_DEBUG_SIZE = ArraySizeOf(positions);
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_CUBE);
                entity->orient = Mat4Rotate(rotations[i], Vec3Lit(0.0f, 1.0f, 0.0f));
                entity->scale = 3.0f;
                entities[i] = entity->id;
            }
        }

        {
            static Vec3 positions[] = {Vec3Lit(-20.0f, 3.0f, 0.0f)};
            static Vec3 velocities[] = {Vec3Lit(0.0f, 0.0f, -10.0f)};
            static f32 rotations[] = {Deg2Rad(0.0f)};

            EntityId* entities = PHYSICS;
            PHYSICS_SIZE = ArraySizeOf(positions);
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_CUBE);
                entity->orient = Mat4Rotate(rotations[i], Vec3Lit(0.0f, 1.0f, 0.0f));
                entity->scale = 3.0f;
                entity->v = velocities[i];
                entities[i] = entity->id;
            }
        }

        {
            static Vec3 positions[] = {Vec3Lit(-20.0f, 3.0f, 15.0f)};
            static f32 rotations[] = {Deg2Rad(0.0f)};

            EntityId* entities = ISO_TRIANGLES;
            ISO_TRIANGLES_SIZE = ArraySizeOf(positions);
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_ISO_TRIANGLE);
                entity->orient = Mat4Rotate(rotations[i], Vec3Lit(0.0f, 1.0f, 0.0f));
                entity->scale = 3.0f;
                entities[i] = entity->id;
            }
        }

        // PYRAMIDS
        {
            static Vec3 positions[] = {Vec3Lit(20.0f, 3.0f, 15.0f)};
            static f32 rotations[] = {Deg2Rad(0.0f)};

            EntityId* entities = PYRAMIDS;
            PYRAMIDS_SIZE = ArraySizeOf(positions);
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_PYRAMID);
                entity->orient = Mat4Rotate(rotations[i], Vec3Lit(1.0f, 0.0f, 0.0f));
                entity->scale = 3.0f;
                entities[i] = entity->id;
            }
        }

        {
            static Vec3 positions[] = {Vec3Lit(0.0f, -10.0f, 0.0f)};
            static f32 rotations[] = {Deg2Rad(-90.0f)};
            static Vec3 axis[] = {Vec3Lit(1.0f, 0.0f, 0.0f)};

            EntityId* entities = WALLS;
            WALLS_SIZE = ArraySizeOf(positions);
            for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                Entity* entity = AllocateEntity();
                entity->pos = positions[i];
                entity->mesh = GetMesh(M_QUAD);
                entity->orient = Mat4Rotate(rotations[i], axis[i]);
                entity->scale = 100.0f;
                entities[i] = entity->id;
            }
        }
    }

    // Camera movement.
    {
        static const u8 GAMEPAD_ID = 0;

        // Left stick movement.
        {
            static const f32 VEL = 20.0f;

            f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_X);
            f32 y = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_Y);
            Vec3 dir_model = Vec3Lit(x, 0.0f, y);

            // Determine body forward pos from camera forward.
            Vec3 forward = Vec3Transform(Vec3Lit(0.0f, 0.0f, -1.0f), vieworient);
            Vec3 c = Vec3Normalize(Vec3Lit(-forward.x, 0.0f, -forward.z));
            Vec3 a = Vec3Normalize(Vec3Cross(Vec3Lit(0.0f, 1.0f, 0.0f), c));
            Vec3 b = Vec3Normalize(Vec3Cross(c, a));
            Mat4 rotation = Mat4Identity();
            rotation.v.a = a;
            rotation.v.b = b;
            rotation.v.c = c;

            Vec3 dir_world = Vec3Transform(dir_model, rotation);
            Vec3 mov_world = Vec3Scale(dir_world, VEL * delta);
            viewpos = Vec3Add(viewpos, mov_world);
        }

        // Right stick movement.
        {
            f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_X);
            f32 y = -1.0f * GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_Y);
            if (x != 0.0f || y != 0.0f) {
                static const rad VEL = Deg2Rad(70.0f);

                // rotate orient.
                {
                    // Find rotation axis in model space.
                    Vec3 dir = Vec3Lit(x, y, 0.0f);
                    Vec3 forward = Vec3Lit(0.0f, 0.0f, -1.0f);
                    Vec3 axis = Vec3Cross(forward, dir);

                    f32 len = Vec3Len(dir);

                    f32 angularvel = VEL * len * delta;
                    Mat4 rotation = Mat4Rotate(angularvel, axis);
                    vieworient = Mat4Multiply(vieworient, rotation);
                }

                // rotate body.
                {
                    // Find rotation axis in model space. only consider rotation around y axis
                    // (which comes from moving stick left/right in x dir).
                    Vec3 dir = Vec3Lit(x, 0.0f, 0.0f);
                    Vec3 forward = Vec3Lit(0.0f, 0.0f, -1.0f);
                    Vec3 axis = Vec3Cross(forward, dir);

                    f32 len = Vec3Len(dir);

                    f32 angularvel = VEL * len * delta;
                    Mat4 rotation = Mat4Rotate(angularvel, axis);
                    viewbody = Mat4Multiply(viewbody, rotation);
                }
            }
        }
    }

    // Get camera matrix by finding transformed forward and then getting look at matrix from that dir + worl pos.
    // remember that we can't just use the view orient matrix because the j and k basis vectors will have
    // roll in them. for fps style camera we wan't no roll.
    Vec3 viewforward = Vec3Transform(Vec3Lit(0.0f, 0.0f, -1.0f), vieworient);
    Mat4 view = Mat4LookAt(viewpos, Vec3Add(viewpos, viewforward), Vec3Lit(0.0f, 1.0f, 0.0f));
    Mat4 proj = Mat4Perspective(Deg2Rad(45.0f), (f32)GetScreenWidth() / (f32)GetScreenHeight(), 0.1f, 1000.0f);

    // Drawing.
    {
        // Begin.
        {
            BeginDrawing();
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindVertexArray(VAO);
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

        // Lights
        {
            LShader shader = GetUnlitShader();
            glUseProgram(shader.pid);
            glUniformMatrix4fv(shader.vview_loc, 1, GL_FALSE, (const f32*)&view.m);
            glUniformMatrix4fv(shader.vproj_loc, 1, GL_FALSE, (const f32*)&proj.m);

            // Draw cube lights.
            {
                EntityId* entities = LIGHTS;
                u32 size = LIGHTS_SIZE;
                for (u32 i = 0; i < size; ++i) {
                    Entity* entity = GetEntity(entities[i]);
                    entity->orient = Mat4Rotate(gstate->timer, Vec3Lit(1.0f, 0.3f, 0.5f));
                }

                DrawBatch(shader, entities, size);
            }
        }

        {
            LShader shader = GetShader();
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

            {
                EntityId* entities = SPINNING;
                u32 size = SPINNING_SIZE;
                for (u32 i = 0; i < size; ++i) {
                    Entity* entity = GetEntity(entities[i]);
                    entity->orient = Mat4Rotate(gstate->timer, Vec3Lit(1.0f, 0.3f, 0.5f));
                }
                DrawBatch(shader, entities, size);
            }

            // Draw non rotating cube for easier lighting visual debug.
            {
                EntityId* entities = SHADOW_DEBUG;
                u32 size = SHADOW_DEBUG_SIZE;
                DrawBatch(shader, entities, size);
            }

            // Physics cube.
            {
                EntityId* entities = PHYSICS;
                u32 size = PHYSICS_SIZE;
                for (u32 i = 0; i < size; ++i) {
                    Entity* entity = GetEntity(entities[i]);
                    entity->pos = Vec3Add(entity->pos, Vec3Scale(entity->v, delta));
                }
                DrawBatch(shader, entities, size);
            }

            // ISO_TRIANGLES Draw.
            {
                EntityId* entities = ISO_TRIANGLES;
                u32 size = ISO_TRIANGLES_SIZE;
                DrawBatch(shader, entities, size);
            }

            // PYRAMIDS Draw.
            {
                EntityId* entities = PYRAMIDS;
                u32 size = PYRAMIDS_SIZE;
                DrawBatch(shader, entities, size);
            }

            // Walls.
            {
                EntityId* entities = WALLS;
                u32 size = WALLS_SIZE;
                DrawBatch(shader, entities, size);
            }

            if (gstate->debug) {
                DrawFPS(5, 5);
            }
        }

        EndDrawing();
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
