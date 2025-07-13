#ifndef UNITY_BUILD
#include <audio.h>
#include <shared.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

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
#define f32sin sin
#define f32cos cos
#define f32tan tan
#define f32asin asin
#define f32acos acos
#define f32atan atan
#define f32floor floorf
#define f32ceil ceilf
#define f32eq(x, y) (f32abs((x) - (y)) < (TOL))
// Prefer this lerp forumulation as at t=0 we will have exactly start and at t=1 we will have exactly end.
// The other formulation is subject to floating point precision issues where we might not get exactly
// end at t=1.
#define f32lerp(s, e, t) (((1.0f - (t)) * (s)) + ((t) * (e)))
// #define f32lerp(s, e, t) ((s) + (((e) - (s)) * (t)))
#define f32rand01() ((f32)rand() / (f32)RAND_MAX)
#define f32randab(a, b) (f32lerp((a), (b), (f32rand01())))

static f32 f32clamp(f32 v, f32 min, f32 max) { return f32min(f32max(v, min), max); }

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
LCORE f32 Vec3Angle(Vec3 a, Vec3 b) { return f32acos(Vec3Dot(a, b) / (Vec3Len(a) * Vec3Len(b))); }
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

#define Mat4ToRMatrix(m)                                                                                      \
    ((Matrix){(m).mm.m00, (m).mm.m10, (m).mm.m20, (m).mm.m30, (m).mm.m01, (m).mm.m11, (m).mm.m21, (m).mm.m31, \
              (m).mm.m02, (m).mm.m12, (m).mm.m22, (m).mm.m32, (m).mm.m03, (m).mm.m13, (m).mm.m23, (m).mm.m33})

LCORE Mat4 MatrixToMat4(Matrix mx) {
    return Mat4Lit(mx.m0, mx.m1, mx.m2, mx.m3,    //
                   mx.m4, mx.m5, mx.m6, mx.m7,    //
                   mx.m8, mx.m9, mx.m10, mx.m11,  //
                   mx.m12, mx.m13, mx.m14, mx.m15);
}

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

////////////////////////////////////////////////////////////
// S:Memory

typedef struct {
    // TODO: I might want to include a stable pointer to the start if I ever want to free() arenas.
    // TODO IMPORTANT: Convert these to some typedef'd version of char* so we don't violate strict aliasing rules.
    u8* beg;
    u8* end;
} Arena;

#define New(a, count, type) (ArenaAlloc((a), (count), sizeof(type), _Alignof(type)))
LCORE void* ArenaAlloc(Arena* a, isize count, isize size, isize alignment) {
    isize padding = -(uptr)a->beg & (alignment - 1);
    isize nbytes = count * size;
    // I think <= is appropriate here. if remaining = 1 and we want to allocate 1, then we should be able to do so
    Assert(count <= (a->end - a->beg - padding) / size);
    void* ptr = a->beg + padding;
    a->beg += padding + nbytes;
    return memset(ptr, 0, nbytes);
};

LCORE Arena NewArena(isize cap) {
    Arena a = {0};
    a.beg = malloc(cap);
    Assert(a.beg != NULL);
    a.end = a.beg + cap;
    return a;
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
static const Vec4 LGRAY = Vec4Lit(199.0f / 255.0f, 197.0f / 255.0f, 198.0f / 255.0f, 1.0f);
static const Vec4 LYELLOW = Vec4Lit(1.0f, 1.0f, 0.0f, 1.0f);

#define RGBALit(r, g, b, a) (Vec4Lit((r) / 255.0f, (g) / 255.0f, (b) / 255.0f, (a)))
#define RGBLit(r, g, b) (RGBALit((r), (g), (b), 1.0f))

////////////////////////////////////////////////////////////
// S:Render Data

typedef struct {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
} Vertex;

typedef struct {
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

typedef struct {
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

typedef u32 TextureId;

typedef struct {
    TextureId id;
    i32 width;
    i32 height;
} LTexture;

static LTexture UploadTexture(const char* filename) {
    LTexture tx = {0};
    i32 n = 0;
    stbi_set_flip_vertically_on_load(true);
    u8* data = stbi_load(filename, &tx.width, &tx.height, &n, 0);
    Assert(data != NULL);

    GLenum format;
    Assert(n == 3 || n == 4);
    if (n == 3) {
        format = GL_RGB;
    } else if (n == 4) {
        format = GL_RGBA;
    } else {
        Assert(false);
    }

    glGenTextures(1, &tx.id);
    glBindTexture(GL_TEXTURE_2D, tx.id);

    glTexImage2D(GL_TEXTURE_2D, 0, format, tx.width, tx.height, 0, format, GL_UNSIGNED_BYTE, data);
    // I don't think I want to mipmap since that can apparently cause texture bleeding when atlasing in 2d.
    // glGenerateMipmap(GL_TEXTURE_2D);

    // TODO: Should I always be using GL_CLAMP_TO_EDGE?
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    format == GL_RGBA
                        ? GL_CLAMP_TO_EDGE
                        : GL_REPEAT);  // set texture wrapping to GL_CLAMP_TO_EDGE (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(data);
    return tx;
}

#define MAX_TEXTURES 10
typedef struct {
    u32 txlen;
    u32 txcap;
    LTexture textures[MAX_TEXTURES];
} TextureCache;

static TextureCache TCACHE;

typedef enum TextureLabel {
    TX_DEFAULT = 0,
    TX_LFONT,
    TX_LFONT2,
    TX_WOODMAN_STAGE,
    TX_LADDER_TEST_STAGE,
    TX_ROOM_TEST_STAGE,
    TX_MEGAMAN,
    TX_MEGAMAN2,
    TX_COUNT
} TextureLabel;

static LTexture GetTexture(TextureLabel label) {
    TextureCache* tcache = &TCACHE;
    return tcache->textures[label];
}

#define TextureBottomLeft(tx) (Vec3Lit(-0.5f * (tx).width, -0.5f * (tx).height, 0.0f))

// Address center, and all 4 corners of an aseprite pixel. We consider the aseprite coord
// given in the UI to be the center of the pixel.
#define AspritePxCtr(px) (px)
#define AsepritePxBL(px) (Vec2Lit((px).x - 0.5f, (px).y + 0.5f))
#define AsepritePxBR(px) (Vec2Lit((px).x + 0.5f, (px).y + 0.5f))
#define AsepritePxTL(px) (Vec2Lit((px).x - 0.5f, (px).y - 0.5f))
#define AsepritePxTR(px) (Vec2Lit((px).x + 0.5f, (px).y - 0.5f))

// This essentially moves the origin from aseprite top left corner to bottom right corner. Why do we do this?
// Its because when we import the texture/png we flip the Y axis there. subtract 1 from height so that the
// max height aseprite pixel coords get mapped to 0
#define AsepritePosToFlipped(tx, apos) (Vec2Lit((apos).x, (tx).height - 1.0f - (apos).y))

// Conver the position in aseprite space to world space. aseprite space is the coordinate given by
// aseprite UI.
static Vec2 AsepritePosToWorldPos(LTexture tx, Vec2 pos_aseprite) {
    pos_aseprite = AsepritePosToFlipped(tx, pos_aseprite);

    // The origin in world space of the texture is at its center. We want to find the offset from the origin in
    // aseprite space (which is the center of the bottom left pixel since we flip the Y coords) to the center of
    // the texture itself. Subtract 1 from width/height because we want to find the midpoint between the bottem left
    // pixel's center and the top right pixel's center.
    Vec2 origin_offset = Vec2Lit(-0.5f * (tx.width - 1.0f), -0.5f * (tx.height - 1.0f));
    Vec2 pos_world = Vec2Add(pos_aseprite, origin_offset);
    return pos_world;
}

static void TexturesInit() {
    TextureCache* tcache = &TCACHE;
    tcache->txlen = 0;
    tcache->txcap = MAX_TEXTURES;

    // Default black texture.
    {
        Assert(tcache->txlen < tcache->txcap);

        LTexture tx = {0};
        tx.width = 1;
        tx.height = 1;
        glGenTextures(1, &tx.id);
        glBindTexture(GL_TEXTURE_2D, tx.id);

        // The texture will be all white. Important to do this so that we can properly blend
        // texture colors and uniform frag color.
        GLubyte transparentPixel[] = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, transparentPixel);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        tcache->textures[TX_DEFAULT] = tx;
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_LFONT] = UploadTexture("assets/textures/LFont.png");
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_LFONT2] = UploadTexture("assets/textures/lfont2.png");
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_WOODMAN_STAGE] = UploadTexture("assets/textures/woodman-stage.png");
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_LADDER_TEST_STAGE] = UploadTexture("assets/textures/stages/ladder-test.png");
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_ROOM_TEST_STAGE] = UploadTexture("assets/textures/stages/room-test.png");
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_MEGAMAN] = UploadTexture("assets/textures/megaman.png");
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_MEGAMAN2] = UploadTexture("assets/textures/megaman2.png");
        ++tcache->txlen;
    }
}

typedef struct {
    u32 fbid;        // framebuffer id
    TextureId txid;  // texture id
    u32 width;
    u32 height;
} RenderTarget;

static RenderTarget CreateRenderTarget(u32 width, u32 height) {
    u32 fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    u32 txid;
    glGenTextures(1, &txid);
    glBindTexture(GL_TEXTURE_2D, txid);

    // Init blank texture and add as the color attachment to the framebuffer
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, txid, 0);

    // Create a render buffer for our framebuffer for stencil/depth testing.
    u32 rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    // Bind default framebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Note: This texture is not in the texture cache.
    return (RenderTarget){.fbid = fbo, .txid = txid, .width = width, .height = height};
}

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
        VBUFFER[VBUFFER_SIZE++] = (Vertex){.pos = vs[i], .normal = ns[i], .uv = Vec2Zero()};
    }

    LMesh* mesh = &MESHES[MESHES_SIZE];
    ++MESHES_SIZE;
    mesh->ibufferidx = ioffset;
    mesh->ntriangles = tsize;
}

static void AddMesh2(Vec3* vs, u32 vsize, Vec2* uvs, u32 uvsize, Triangle* ts, u32 tsize) {
    Assert(vsize == uvsize);

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
        VBUFFER[VBUFFER_SIZE++] = (Vertex){.pos = vs[i], .normal = ns[i], .uv = uvs[i]};
    }

    LMesh* mesh = &MESHES[MESHES_SIZE];
    ++MESHES_SIZE;
    mesh->ibufferidx = ioffset;
    mesh->ntriangles = tsize;
}

typedef u32 ShaderId;

typedef struct {
    ShaderId pid;  // program id.
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
        "layout (location = 2) in vec2 vTextCoords;\n"
        "\n"
        "uniform mat4 vModel;\n"
        "uniform mat4 vView;\n"
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
        "   gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);\n"
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
        "   vec4 txColor = texture(texture0, fTextCoords);\n"
        // Crucial for having depth buffer + blending enabled. when I write quads with textures with some tansparent
        // parts. We want to discard those transparent parts else those values will get written to the depth buffer.
        "   if (txColor.a < 0.01) {\n"
        "       discard;\n"
        "   }\n"
        // For blending to work here properly, its important that either texture is specified or texture
        // is default (all black) and a uniform color is specified. also 2d needs to be drawn after 3d.
        "    FragColor = txColor * fColor;\n"
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
            SHADER.vview_loc = glGetUniformLocation(SHADER.pid, "vView");
            SHADER.vproj_loc = glGetUniformLocation(SHADER.pid, "vProj");
            SHADER.vucolor_loc = glGetUniformLocation(SHADER.pid, "vUColor");
            SHADER.vuvoffset_loc = glGetUniformLocation(SHADER.pid, "vUVOffset");
            SHADER.vuvscale_loc = glGetUniformLocation(SHADER.pid, "vUVScale");

            glUseProgram(SHADER.pid);
            glUniform1i(glGetUniformLocation(SHADER.pid, "texture0"), 0);
        }
    }

    return SHADER;
}

static LShader GetEffectsShader() {
    static const char* VSRC =
        "#version 330 core\n"
        "layout (location = 0) in vec3 vPos;\n"
        "layout (location = 2) in vec2 vTextCoords;\n"
        "\n"
        "uniform mat4 vModel;\n"
        "uniform mat4 vView;\n"
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
        "   gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);\n"
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
        "vec2 uvremap(vec2 uv)\n"
        "{\n"
        "    // as we near the edge of our screen apply greater distortion using a cubic function\n"
        "    vec2 curvature = vec2(7.0, 7.0);\n"
        "    uv = uv * 2.0 - 1.0;\n"
        "    vec2 offset = abs(uv.yx) / vec2(curvature.x, curvature.y);\n"
        "    uv = uv + uv * offset * offset;\n"
        "    uv = uv * 0.5 + 0.5;\n"
        "    return uv;\n"
        "}\n"
        "\n"
        "vec4 scanLineIntensity(float uv, float resolution, float opacity)\n"
        "{\n"
        "    float PI = 3.1415926535;\n"
        "    float intensity = sin(uv * resolution * PI * 2.0);\n"
        "    intensity = ((0.5 * intensity) + 0.5) * 0.9 + 0.1;\n"
        "    return vec4(vec3(pow(intensity, opacity)), 1.0);\n"
        "}\n"
        "\n"
        "void main()\n"
        "{\n"
        "   vec2 uv = fTextCoords;\n"
        "   vec4 txcolor = texture(texture0, uv);\n"
        "   FragColor = txcolor;\n"
        // "   vec2 uv = uvremap(fTextCoords);\n"
        // // "   vec2 uv = fTextCoords;\n"
        // "   vec4 txcolor = texture(texture0, uv);\n"
        // "   txcolor *= scanLineIntensity(uv.x, 256.0 * 3.0, 0.2);\n"
        // "   txcolor *= scanLineIntensity(uv.y, 224.0 * 3.0, 0.2);\n"
        // " if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {\n"
        // "   txcolor = vec4(0.0, 0.0, 0.0, 1.0);\n"
        // " }\n"
        // "   FragColor = txcolor;\n"
        //"   float grayscale = max(txcolor.r, max(txcolor.g, txcolor.b));\n"
        // "   FragColor = vec4(grayscale, grayscale, grayscale, 1.0);\n"
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
            SHADER.vview_loc = glGetUniformLocation(SHADER.pid, "vView");
            SHADER.vproj_loc = glGetUniformLocation(SHADER.pid, "vProj");
            SHADER.vucolor_loc = glGetUniformLocation(SHADER.pid, "vUColor");
            SHADER.vuvoffset_loc = glGetUniformLocation(SHADER.pid, "vUVOffset");
            SHADER.vuvscale_loc = glGetUniformLocation(SHADER.pid, "vUVScale");

            glUseProgram(SHADER.pid);
            glUniform1i(glGetUniformLocation(SHADER.pid, "texture0"), 0);
        }
    }

    return SHADER;
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
        // quad is aligned with the xy plane. the full width of the quad is 1.0f so scaling by 2 will
        // make the width 2
        Vec3 vs[] = {
            Vec3Lit(-0.5f, 0.5f, 0.0f),
            Vec3Lit(-0.5f, -0.5f, 0.0f),
            Vec3Lit(0.5f, -0.5f, 0.0f),
            Vec3Lit(0.5f, 0.5f, 0.0f),
        };
        Vec2 uvs[] = {
            Vec2Lit(0.0f, 1.0f),
            Vec2Lit(0.0f, 0.0f),
            Vec2Lit(1.0f, 0.0f),
            Vec2Lit(1.0f, 1.0f),
        };
        Triangle ts[] = {
            TriLit(0, 1, 2),  //
            TriLit(2, 3, 0),  //
        };
        AddMesh2(vs, ArraySizeOf(vs), uvs, ArraySizeOf(uvs), ts, ArraySizeOf(ts));
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

    // NOTE: This is very tricky to get right. Don't dynamically set these based on shader attribute
    // locations. just hardcode and make sure that the shader code specifies the corAserect layout.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    TexturesInit();
}

typedef enum {
    CMD_NONE = 0,
    CMD_DRAW_MESH,
    CMD_CLEAR,
    CMD_COUNT,
} DrawCmdType;

typedef struct {
    DrawCmdType type;

    union {
        // Draws a mesh.
        struct {
            LMesh meshid;
            Mat4 modelm;  // converts from model space to world space.
            Vec4 color;
            Vec2 uvoffset;   // offset of the bottom left corner/origin in texture space (0, 0)
            Vec2 uvscale;    // width/height in texture space of the Aserect to sample inside the texture [0, 1]
            TextureId txid;  // Texture id.
        } meshcmd;

        // Clears buffer with color.
        struct {
            Vec4 color;
        } clearcmd;
    };
} DrawCmd;

// Assumes the struct has been memset already.
LCORE void DrawMeshCmdInit(DrawCmd* cmd) {
    cmd->type = CMD_DRAW_MESH;
    cmd->meshcmd.color = LCLEAR;
    cmd->meshcmd.modelm = Mat4Identity();
    cmd->meshcmd.uvoffset = Vec2Lit(0.0f, 0.0f);
    cmd->meshcmd.uvscale = Vec2Lit(1.0f, 1.0f);
    cmd->meshcmd.txid = GetTexture(TX_DEFAULT).id;
}

#define MAX_CMDS 4092

typedef struct {
    ShaderId sid;

    ///////////////////////
    // Uniform data that should be set once for a pass here.
    Mat4 view;
    Mat4 proj;
    ///////////////////////

    u32 clen;
    u32 ccap;
    DrawCmd cmds[MAX_CMDS];
} ShaderPass;

static void ShaderPassInit(ShaderPass* pass, ShaderId sid) {
    memset(pass, 0, sizeof(ShaderPass));
    pass->sid = sid;
    pass->view = Mat4Identity();
    pass->proj = Mat4Identity();

    pass->clen = 0;
    pass->ccap = MAX_CMDS;
}

static DrawCmd* ShaderPassAddCmd(ShaderPass* pass) {
    Assert(pass->clen < pass->ccap);
    DrawCmd* cmd = &pass->cmds[pass->clen++];
    memset(cmd, 0, sizeof(DrawCmd));
    return cmd;
}

static void ShaderPassExecute(ShaderPass* pass) {
    LShader shader2d = GetShader();
    LShader effects_shader = GetEffectsShader();

    if (pass->sid == shader2d.pid || pass->sid == effects_shader.pid) {
        // Need a better way to map shaders.
        LShader shader = pass->sid == shader2d.pid ? shader2d : effects_shader;

        glUseProgram(shader.pid);

        glUniformMatrix4fv(shader.vview_loc, 1, GL_FALSE, (const f32*)&pass->view.m);
        glUniformMatrix4fv(shader.vproj_loc, 1, GL_FALSE, (const f32*)&pass->proj.m);

        for (u32 i = 0; i < pass->clen; ++i) {
            DrawCmd* cmd = &pass->cmds[i];
            switch (cmd->type) {
                case CMD_DRAW_MESH: {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, cmd->meshcmd.txid);

                    glUniformMatrix4fv(shader.vmodel_loc, 1, GL_FALSE, (const f32*)&cmd->meshcmd.modelm.m);
                    glUniform4fv(shader.vucolor_loc, 1, (const f32*)&cmd->meshcmd.color.f);
                    glUniform2fv(shader.vuvoffset_loc, 1, (const f32*)&cmd->meshcmd.uvoffset.f);
                    glUniform2fv(shader.vuvscale_loc, 1, (const f32*)&cmd->meshcmd.uvscale.f);

                    glDrawElements(GL_TRIANGLES, cmd->meshcmd.meshid.ntriangles * 3, GL_UNSIGNED_INT,
                                   (void*)(cmd->meshcmd.meshid.ibufferidx * sizeof(GLuint)));
                    break;
                };

                case CMD_CLEAR: {
                    Vec4 color = cmd->clearcmd.color;
                    glClearColor(color.r, color.g, color.b, color.a);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    break;
                }

                default: {
                    Assert(false);
                    break;
                };
            }
        }
    } else {
        Assert(false);
    }
}

#define MAX_SHADER_PASSES 10
static const u32 PASSNULL = MAX_SHADER_PASSES;

typedef struct {
    u32 passidx;
    u32 passlen;
    u32 passcap;
    ShaderPass passes[MAX_SHADER_PASSES];

    RenderTarget target;
} RenderState;

static RenderState RSTATE;

#define CurShaderPass() (&RSTATE.passes[RSTATE.passidx])

static void RenderStateInit(u32 width, u32 height) {
    RenderState* rstate = &RSTATE;

    memset(rstate, 0, sizeof(RenderTarget));
    rstate->target = CreateRenderTarget(width, height);
}

// Reset render state and allow accumulation of shader passes.
static void BeginRender() {
    RenderState* rstate = &RSTATE;
    rstate->passidx = PASSNULL;
    rstate->passlen = 0;
    rstate->passcap = MAX_SHADER_PASSES - 1;  // last entry reserved for null idx.
}

static ShaderPass* BeginShaderPass(LShader shader) {
    RenderState* rstate = &RSTATE;
    Assert(rstate->passlen < rstate->passcap);
    Assert(rstate->passidx == PASSNULL);

    rstate->passidx = rstate->passlen++;
    ShaderPass* pass = &rstate->passes[rstate->passidx];
    ShaderPassInit(pass, shader.pid);
    return pass;
}

static void EndShaderPass() {
    RenderState* rstate = &RSTATE;
    Assert(rstate->passidx != PASSNULL);
    rstate->passidx = PASSNULL;
}

// Execute shader passes and produce render to screen.
static void EndRender() {
    RenderState* rstate = &RSTATE;

    Assert(rstate->passidx == PASSNULL);

    // Begin.
    {
        BeginDrawing();

        // Render draw commands to render target.
        glBindFramebuffer(GL_FRAMEBUFFER, rstate->target.fbid);
        // IMPORTANT.
        glViewport(0, 0, rstate->target.width, rstate->target.height);
        glEnable(GL_DEPTH_TEST);

        // TODO: turn this into a draw command using unions.
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // TODO: move this to shader pass data.
        glBindVertexArray(VAO);
    }

    // // Enable/Disable culling.
    // // TODO: Make this nicer at some point. Useful for debugging.
    // {
    //     if (gstate->debug) {
    //         glDisable(GL_CULL_FACE);
    //     } else {
    //         glEnable(GL_CULL_FACE);
    //         glCullFace(GL_BACK);
    //         glFrontFace(GL_CCW);
    //     }
    // }

    for (u32 i = 0; i < rstate->passlen; ++i) {
        ShaderPassExecute(&rstate->passes[i]);
    }

    // Draw the render target texture to screen as a quad.
    // TODO: This is very messy. I should be able to do this using draw commands to change framebuffers.
    // This would require an inversion of how framebuffers work though. They would need to also contain shader
    // pass commands. Or maybe I should just have every shader pass store what framebuffer they are drawing to
    // But this would require some duplication in code. If I have two shaders that process draw commands
    // I'd need to duplicate ClearBuffer() draw command logic.
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // IMPORTANT.
        glViewport(0, 0, GetScreenWidth(), GetScreenHeight());
        glDisable(GL_DEPTH_TEST);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ShaderPass pass;
        ShaderPassInit(&pass, GetEffectsShader().pid);
        DrawCmd* cmd = ShaderPassAddCmd(&pass);
        DrawMeshCmdInit(cmd);
        cmd->meshcmd.meshid = GetMesh(M_QUAD);
        // Since we only set a scale matrix, essentially we are specifying this quad in clip space. since the quad
        // is from -0.5, 0.5, we mult by 2 to get -1, 1 so we cover the entire screen.
        cmd->meshcmd.modelm = Mat4ScaleV(Vec3Lit(2.0f, 2.0f, 1.0f));
        // cmd->meshcmd.color = LRED;  // For debug.
        cmd->meshcmd.txid = rstate->target.txid;
        ShaderPassExecute(&pass);
    }

    EndDrawing();
}

static DrawCmd* AddCmd() {
    RenderState* rstate = &RSTATE;
    Assert(rstate->passlen != 0);
    DrawCmd* cmd = ShaderPassAddCmd(CurShaderPass());
    return cmd;
}

static DrawCmd* AddDrawMeshCmd() {
    DrawCmd* cmd = AddCmd();
    DrawMeshCmdInit(cmd);
    return cmd;
}

static DrawCmd* ClearBuffer(Vec4 color) {
    DrawCmd* cmd = AddCmd();
    cmd->type = CMD_CLEAR;
    cmd->clearcmd.color = color;
    return cmd;
}

LCORE void DrawLSphere(Vec3 center, f32 radius, Vec4 color) {
    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = GetMesh(M_ICOSPHERE);
    cmd->meshcmd.modelm = Mat4SRT(radius, Mat4Identity(), center);
    cmd->meshcmd.color = color;
}

LCORE void DrawLCircle(Vec3 center, f32 radius, Vec4 color) {
    DrawCmd* cmd = AddDrawMeshCmd();
    // We cheat and just use an icosphere. assumes ortho projection which should chop it into a circle.
    cmd->meshcmd.meshid = GetMesh(M_ICOSPHERE);
    cmd->meshcmd.modelm = Mat4SRT(radius, Mat4Identity(), center);
    cmd->meshcmd.color = color;
}

LCORE void DrawLCube(Vec3 center, Vec3 scale, Vec4 color) {
    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = GetMesh(M_CUBE);
    cmd->meshcmd.modelm = Mat4SVRT(scale, Mat4Identity(), center);
    cmd->meshcmd.color = color;
}

LCORE DrawCmd* DrawQuad(Vec3 center, f32 width, f32 height, Mat4 orient, Vec4 color) {
    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = GetMesh(M_QUAD);
    cmd->meshcmd.modelm = Mat4SVRT(Vec3Lit(width, height, 1.0f), orient, center);
    cmd->meshcmd.color = color;
    return cmd;
}

LCORE void DrawAABB(Vec3 center, Vec3 radius, Vec4 color) {
    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = GetMesh(M_CUBE);
    cmd->meshcmd.modelm = Mat4SVRT(Vec3Scale(radius, 2.0f), Mat4Identity(), center);
    cmd->meshcmd.color = color;
}

// Remember
// src == where to draw in texture space
// dest == where to draw in world space.
// (0, 0)
// * maps to bottom left corner in src texture space (this is not uv space, but width/height space)
// * maps to origin in world space.
// radius
// * need to be full width/height of quad in texture space
// * need to be half width/height in world space. Is this right? it looks like it might need to be full width/height
LCORE void DrawLTexturePro(LTexture tx, Vec2 c_src, Vec2 r_src, Vec2 c_dst, Vec2 r_dst, f32 rotation, Vec4 color) {
    // TODO: is scale right?
    Vec2 uvscale = Vec2Lit(r_src.x / tx.width, r_src.y / tx.height);
    Vec2 uvoffset = Vec2Lit(c_src.x / tx.width, c_src.y / tx.height);

    Mat4 scale = Mat4ScaleV(Vec3Lit(r_dst.x, r_dst.y, 1.0f));
    Mat4 orient = Mat4Rotate(rotation, Vec3Lit(0.0f, 0.0f, 1.0f));
    Mat4 translate = Mat4TranslateV(Vec3Lit(c_dst.x, c_dst.y, 0.0f));
    Mat4 model = Mat4Multiply(Mat4Multiply(translate, orient), scale);

    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.modelm = model;
    cmd->meshcmd.color = color;
    cmd->meshcmd.uvoffset = uvoffset;
    cmd->meshcmd.uvscale = uvscale;
    cmd->meshcmd.txid = tx.id;
    cmd->meshcmd.meshid = GetMesh(M_QUAD);
}

// Remember
// src == where to draw in texture space
// dest == where to draw in world space.
// (0, 0)
// * maps to bottom left corner in src texture space (this is not uv space, but width/height space)
// * maps to origin in world space.
// :c_src - source origin (lower left corner) of where to read from texture in uv space
// :r_src - full width/height of sub rect to read from texture in uv space
// :c_dst - destination origin (center of quad) of where to draw in world space
// :r_dst - full width + height of quad
LCORE DrawCmd* DrawLTexturePro2(TextureId txid, Vec2 c_src, Vec2 r_src, Vec2 c_dst, Vec2 r_dst, f32 rotation,
                                Vec4 color) {
    Mat4 scale = Mat4ScaleV(Vec3Lit(r_dst.x, r_dst.y, 1.0f));
    Mat4 orient = Mat4Rotate(rotation, Vec3Lit(0.0f, 0.0f, 1.0f));
    Mat4 translate = Mat4TranslateV(Vec3Lit(c_dst.x, c_dst.y, 0.0f));
    Mat4 model = Mat4Multiply(Mat4Multiply(translate, orient), scale);

    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.modelm = model;
    cmd->meshcmd.color = color;
    cmd->meshcmd.uvoffset = c_src;
    cmd->meshcmd.uvscale = r_src;
    cmd->meshcmd.txid = txid;
    cmd->meshcmd.meshid = GetMesh(M_QUAD);
    return cmd;
}

LCORE void DrawLTextureEx(LTexture tx, Vec2 pos, f32 scale, f32 rotation, Vec4 color) {
    DrawLTexturePro(tx, Vec2Lit(0.0f, 0.0f), Vec2Lit(tx.width, tx.height), pos, Vec2Lit(scale, scale), rotation, color);
}

// Remember that pos will be the center of the quad.
// Scale will control the multiple of original size of the texture.
LCORE void DrawLTexture(LTexture tx, Vec2 pos, f32 scale) {
    DrawLTexturePro(tx, Vec2Lit(0.0f, 0.0f), Vec2Lit(tx.width, tx.height), pos,
                    Vec2Lit(tx.width * scale, tx.height * scale), 0.0f, LCLEAR);
}

////////////////////////////////////////////////////////////
// S:Geometry

static const f32 SLOP = 2.0f * F32_EPSILON;

static const Vec3 AXIS[] = {Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f)};

LCORE Vec3 ComputeCentroid(Vec3* vs, u32 vsize) {
    Vec3 center = {0};
    for (u32 i = 0; i < vsize; ++i) {
        center = Vec3Add(center, vs[i]);
    }
    center.x /= vsize;
    center.y /= vsize;
    center.z /= vsize;
    return center;
}

typedef struct {
    Vec2 c;
    f32 r;
} Circle;

// Returns normal pointing from b to a if there is a collision. Assumes both
// circles are in same coordinate space.
static bool TestCircleCircle(Circle a, Circle b, Vec2* normal, f32* depth) {
    Vec2 ba = Vec2Sub(b.c, a.c);
    f32 dsquared = (ba.x * ba.x) + (ba.y * ba.y);
    f32 r = a.r + b.r;
    // if (squared) distance between a and b is greater than the (squared) distance of the
    // combined radii, then no collision.
    if (r * r < dsquared) {
        return false;
    }

    f32 d = f32sqrt(dsquared);
    *normal = Vec2Scale(ba, 1.0f / d);
    *depth = d - r;
    return true;
}

typedef struct {
    Vec3 c;  // center
    f32 r;   // radius
} Sphere;

typedef struct {
    Vec2 c;  // center.
    Vec2 r;  // half width extents along each axis.
} AABB2D;

// Returns a aabb2d from a bottom left and top right
LCORE AABB2D AABB2DFromMinMax(Vec2 pmin, Vec2 pmax) {
    Vec2 r = Vec2Sub(pmax, pmin);
    r.x *= 0.5f;
    r.y *= 0.5f;

    AABB2D box = {0};
    box.r = r;
    box.c = Vec2Add(pmin, r);
    return box;
}

// Convert an aseprite bb on a marked up png to a world position bb.
// The aseprite bb min will be the bottom left corner of the box THAT I SEE ON THE SCREEN. Not the bottom
// left according to the pixel coords in the UI. The Top right will be similar.
LCORE AABB2D AABB2DFromAsepriteBB(LTexture tx, Vec2 min_aseprite, Vec2 max_aseprite) {
    Vec2 min_world = AsepritePosToWorldPos(tx, min_aseprite);
    Vec2 max_world = AsepritePosToWorldPos(tx, max_aseprite);
    AABB2D bb = AABB2DFromMinMax(min_world, max_world);
    return bb;
}

// Normal from b -> a if there is a collision.
LCORE bool TestAABB2DVsAABB2D(AABB2D a, AABB2D b, Vec2* normal, f32* depth) {
    Vec2 ba = Vec2Sub(a.c, b.c);

    *normal = Vec2Zero();
    *depth = F32_MAX;

    // Remember that r is half extents, so we don't need to divide by 2 to find minimum distance allowed
    // between centers along a given axis. Take abs because we just want magnitude.
    f32 depthx = (a.r.x + b.r.x) - f32abs(ba.x);
    if (depthx < 0.0f) {
        return false;
    }

    f32 depthy = (a.r.y + b.r.y) - f32abs(ba.y);
    if (depthy < 0.0f) {
        return false;
    }

    if (depthx <= depthy) {
        *normal = Vec2Lit(1.0f * (ba.x < 0 ? -1.0f : 1.0f), 0.0f);
        *depth = depthx;
    } else {
        *normal = Vec2Lit(0.0f, 1.0f * (ba.y < 0 ? -1.0f : 1.0f));
        *depth = depthy;
    }

    return true;
}

// Normal will point from aabb to circle.
LCORE bool TestCircleVsAABB2D(Circle circle, AABB2D aabb, Vec2* normal, f32* depth) {
    Vec2 min = Vec2Sub(aabb.c, aabb.r);
    Vec2 max = Vec2Add(aabb.c, aabb.r);

    // Find closest point from circle center to aabb (which might be inside the aabb).
    Vec2 p = circle.c;
    for (u32 i = 0; i < 2; ++i) {
        p.f[i] = f32clamp(p.f[i], min.f[i], max.f[i]);
    }

    // If squared distance from closest point to circle on aabb to circle center is greater
    // than the squared radius of the circle, then we don't have a collision.
    Vec2 disp = Vec2Sub(circle.c, p);
    f32 d = (disp.x * disp.x) + (disp.y * disp.y);
    if (circle.r * circle.r < d) {
        return false;
    }

    // If the center is inside of the aabb, then the normal and depth will not be corAserect, since the
    // closest point will be the center itself. But at least we will report the collision happened.
    *depth = f32sqrt(d) - circle.r;
    *normal = Vec2Normalize(disp);
    return true;
}

typedef struct {
    Vec3 c;  // center.
    Vec3 r;  // half width extents along each axis.
} AABB;

typedef enum PolyhedronType { PH_NONE = 0, PH_CIRCLE, PH_SPHERE, PH_AABB2D, PH_AABB, PH_COUNT } PolyhedronType;

typedef struct {
    PolyhedronType type;

    union {
        Circle circle;
        AABB2D aabb2d;
        Sphere sphere;
        AABB aabb;
    };
} Polyhedron;

LCORE void PolyhedronInit(Polyhedron* p) {
    memset(p, 0, sizeof(Polyhedron));
    p->type = PH_NONE;
}

// lc: bottom left corner
// dims: full width/height of bbox
// grid_dims: width x height of sprite
// Useful for setting collision info based on visually drawn box in programs like aseprite.
// We assume that (0, 0) is the bottom left *corner* (not the center of the bottom left pixel).
// To translate from aseprite, count the number of pixels (where bottom left pixel counts as 1) until the first
// pixel of the collider, then subtract (-1, -1) to get the lc.
LCORE void PolyhedronSetAABB2DFromGridAseRect(Polyhedron* shape, Vec2 lc, Vec2 dims, Vec2 grid_dims) {
    shape->type = PH_AABB2D;

    // The input params assume that the min and max are given as if the origin is the bottom left
    // corner of the sprite, but our collision system assumes that the center of the bounding box is relative
    // to the center of the sprite/entity so we need to offset.
    Vec2 offset = Vec2Scale(grid_dims, -0.5f);

    // Compute half width/height.
    Vec2 r = Vec2Scale(dims, 0.5f);
    Vec2 c = Vec2Add(Vec2Add(lc, r), offset);

    shape->aabb2d.r = r;
    shape->aabb2d.c = c;
}

// Set AABB2D From center and full width and height.
LCORE void PolyHedronSetAABB2DFromWH(Polyhedron* shape, Vec2 c, Vec2 dims) {
    shape->type = PH_AABB2D;
    shape->aabb2d.c = c;
    shape->aabb2d.r = Vec2Lit(0.5f * dims.x, 0.5f * dims.y);
}

LCORE void PolyhedronSetCircle(Polyhedron* shape, Vec2 c, f32 r) {
    shape->type = PH_CIRCLE;
    shape->circle.c = c;
    shape->circle.r = r;
}

// Important to pass decomposed transformation matrix (scale, rot, translate) because some polyhedron
// like world axis-aligned shapes do not rotate for instance.
// TODO: Consider making an interface where I get the corAserect polyhedron transformation matrix as a separate
// function (maybe take an entity as an arg) and then I can just take a single matrix here
LCORE Polyhedron PolyhedronTransform(Polyhedron shape, Vec3 s, Mat4 r, Vec3 t) {
    switch (shape.type) {
        case PH_NONE: {
            break;
        }
        case PH_CIRCLE: {
            Mat4 m = Mat4Multiply(Mat4TranslateV(t), Mat4ScaleV(s));

            // This might be a bit jank. but I transform the center with translation and then
            // transform the radius vector (no translation) and take the length.
            Vec3 c = Vec2ToVec3(shape.circle.c);
            c = Vec3WTransform(c, m);
            // TODO: I might actually want to project this vector onto the xy plane.
            shape.circle.c = Vec2Lit(c.x, c.y);
            shape.circle.r = Vec3Len(Vec3Transform(Vec3Lit(shape.circle.r, 0.0f, 0.0f), m));
            break;
        }
        case PH_AABB2D: {
            Mat4 m = Mat4Multiply(Mat4TranslateV(t), Mat4ScaleV(s));

            // This might be a bit jank. but I transform the center with translation and then
            // transform the radius vector (no translation) and take the length.
            Vec3 c = Vec2ToVec3(shape.aabb2d.c);
            c = Vec3WTransform(c, m);
            // TODO: I might actually want to project this vector onto the xy plane.
            shape.aabb2d.c = Vec2Lit(c.x, c.y);
            Vec3 r = Vec3Transform(Vec2ToVec3(shape.aabb2d.r), m);
            shape.aabb2d.r = Vec2Lit(r.x, r.y);
            break;
        }
        case PH_AABB: {
            Mat4 m = Mat4Multiply(Mat4TranslateV(t), Mat4ScaleV(s));

            shape.aabb.c = Vec3WTransform(shape.aabb.c, m);
            shape.aabb.r = Vec3Transform(shape.aabb.r, m);
            break;
        }
        default: {
            Assert(false);
        }
    }
    return shape;
}

// static Polyhedron ComputePolyhedron(Vec3* vs, u32 vsize, PolyhedronType type) {
//     Polyhedron p = {0};
//     p.type = type;
//     switch (type) {
//         case PH_SPHERE: {
//             p.sphere = BoundingSphere(vs, vsize);
//             break;
//         }
//         case PH_AABB: {
//             p.aabb = ComputeAABB(vs, vsize);
//             break;
//         }
//         default: {
//             TraceLog(LOG_ERROR,
//                      "ComputePolyhedron: Unknown polyhedron type=%d. Most likely entity needs bounding volume type
//                      set", type);
//             Assert(false);
//         }
//     }
//     return p;
// }

// Returns true if both polyhedrons collide. normal will point from b to a.
// Assumes polyhedrons are both in the same coordinate space.
LCORE bool TestPolyhedronVsPolyhedron(Polyhedron* a, Polyhedron* b, Vec3* normal, f32* depth) {
    bool collides = false;
    Vec3 n = Vec3Zero();
    Vec2 n2d = Vec2Zero();
    f32 d = 0.0f;

    // Collision shape table.
    switch (a->type) {
        case PH_CIRCLE: {
            switch (b->type) {
                case PH_CIRCLE: {
                    collides = TestCircleCircle(a->circle, b->circle, &n2d, &d);
                    n = Vec3Lit(n2d.x, n2d.y, 0.0f);
                    break;
                }
                case PH_AABB2D: {
                    collides = TestCircleVsAABB2D(a->circle, b->aabb2d, &n2d, &d);
                    n = Vec3Lit(n2d.x, n2d.y, 0.0f);
                }
                default: {
                    // Collision not supported.
                    break;
                }
            }
            break;
        }
        case PH_AABB2D: {
            switch (b->type) {
                case PH_CIRCLE: {
                    collides = TestCircleVsAABB2D(b->circle, a->aabb2d, &n2d, &d);
                    n = Vec3Negate(Vec3Lit(n2d.x, n2d.y, 0.0f));
                    break;
                }
                case PH_AABB2D: {
                    collides = TestAABB2DVsAABB2D(a->aabb2d, b->aabb2d, &n2d, &d);
                    n = Vec3Lit(n2d.x, n2d.y, 0.0f);
                    break;
                }
                default: {
                    // Collision not supported.
                    break;
                }
            }
            break;
        }
        default: {
            // Collision not supported.
            break;
        }
    }

    *normal = n;
    *depth = d;
    return collides;
}

LCORE void DrawPolyhedron(Polyhedron* shape, Vec4 color) {
    switch (shape->type) {
        case PH_NONE: {
            break;
        }
        case PH_CIRCLE: {
            DrawLCircle(Vec3Lit(shape->circle.c.x, shape->circle.c.y, 0.0f), shape->circle.r, color);
            break;
        }
        case PH_AABB2D: {
            // Because we store half extents, scale them up by 2x to get the proper visual scaling factor.
            DrawQuad(Vec3Lit(shape->aabb2d.c.x, shape->aabb2d.c.y, 0.0f), 2.0f * shape->aabb2d.r.x,
                     2.0f * shape->aabb2d.r.y, Mat4Identity(), color);
            break;
        }
        case PH_AABB: {
            // Because we store half extents, scale them up by 2x to get the proper visual scaling factor.
            DrawLCube(shape->aabb.c, Vec3Scale(shape->aabb.r, 2.0f), color);
            break;
        }
        default: {
            Assert(false);
        }
    }
}

////////////////////////////////////////////////////////////
// S:Font

// Loads a true type file at :filename and bakes it into a 512x512 texture containing
// characters at :fontsize size.
static LTexture BakeTrueTypeFontBitmap(char* filename, f32 fontsize, stbtt_bakedchar* cdata) {
    byte ttf_buffer[1 << 20];
    byte bitmap_alpha[512 * 512];
    fread(ttf_buffer, 1, 1 << 20, fopen(filename, "rb"));
    stbtt_BakeFontBitmap(ttf_buffer, 0, fontsize, bitmap_alpha, 512, 512, 32, 96, cdata);

    // Output alpha bitmap to pgm file (greyscal file). viewable in GIMP for debugging.
    if (false) {
        FILE* out = fopen("C:/Users/rvbea/scratch/font_dump.pgm", "wb");
        if (out) {
            fprintf(out, "P5\n%d %d\n255\n", 512, 512);
            fwrite(bitmap_alpha, 1, 512 * 512, out);
            fclose(out);
        } else {
            perror("Failed to open font_dump.pgm for writing");
        }
    }

    // The baked bitmap assumes origin in top left with +y pointing down. Flip the engine expects
    // origin in bottom left with +y going up.
    for (i32 i = 0; i < (512 * 512) / 2; ++i) {
        i32 col = i % 512;
        i32 top_row = i / 512;
        i32 bottom_row = 512 - top_row - 1;
        i32 top_idx = (top_row * 512) + col;
        i32 bottom_idx = (bottom_row * 512) + col;

        byte temp = bitmap_alpha[top_idx];
        bitmap_alpha[top_idx] = bitmap_alpha[bottom_idx];
        bitmap_alpha[bottom_idx] = temp;
    }

    if (false) {
        FILE* out = fopen("C:/Users/rvbea/scratch/font_dump_flipped.pgm", "wb");
        if (out) {
            fprintf(out, "P5\n%d %d\n255\n", 512, 512);
            fwrite(bitmap_alpha, 1, 512 * 512, out);
            fclose(out);
        } else {
            perror("Failed to open font_dump.pgm for writing");
        }
    }

    // The baked bitmap contains only alpha channel information. Convert to rgba so that we can draw
    // this with our normal shader. yes this is a bit wasteful.
    byte* bitmap = calloc(512 * 512 * 4, sizeof(byte));
    for (i32 i = 0; i < (512 * 512); ++i) {
        bitmap[(i * 4) + 0] = 255;
        bitmap[(i * 4) + 1] = 255;
        bitmap[(i * 4) + 2] = 255;
        bitmap[(i * 4) + 3] = bitmap_alpha[i];
    }

    LTexture tx = {0};
    tx.width = 512;
    tx.height = 512;
    glGenTextures(1, &tx.id);
    glBindTexture(GL_TEXTURE_2D, tx.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tx.width, tx.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    free(bitmap);
    return tx;
}

typedef struct {
    LTexture tx;
    f32 fontsize;
    stbtt_bakedchar cdata[96];  // ASCII 32..126 is 95 glyphs
} LFont;

typedef enum {
    LFONT_DEFAULT = 0,
    LFONT_SMALL,
    LFONT_COUNT,
} FontLabel;

static LFont FONTS[LFONT_COUNT] = {0};

LCORE void InitFonts() {
    memset(FONTS, 0, sizeof(LFont) * LFONT_COUNT);

    {
        LFont font = {0};
        font.fontsize = 32.0f;
        font.tx = BakeTrueTypeFontBitmap("c:/windows/fonts/times.ttf", font.fontsize, font.cdata);
        FONTS[LFONT_DEFAULT] = font;
    }

    {
        LFont font = {0};
        font.fontsize = 16.0f;
        font.tx = BakeTrueTypeFontBitmap("c:/windows/fonts/times.ttf", font.fontsize, font.cdata);
        FONTS[LFONT_SMALL] = font;
    }
}

static LFont GetLFont(FontLabel label) {
    Assert(label < LFONT_COUNT);
    return FONTS[label];
}

LCORE Vec2 DrawLTextEx(LFont font, char* text, i32 len, Vec2 baseline_pos, Vec4 color) {
    for (i32 i = 0; i < len; ++i) {
        stbtt_aligned_quad quad = {0};
        // The y position passed in here is the baseline of the glpyhs (i.e. the common line they will sit on)
        stbtt_GetBakedQuad(font.cdata, font.tx.width, font.tx.height, text[i] - 32, &baseline_pos.x, &baseline_pos.y,
                           &quad, true);

        /*
            Remember. the key to understanding all of these transformations is to visualize them from the perspective
            of the originating coordinate system (stb_truetype's coordinate system). If you try to visualize
            them from the perspective of the final coordinate system, it will be hard to do.
        */

        Vec2 qdims = Vec2Sub(Vec2Lit(quad.x1, quad.y1), Vec2Lit(quad.x0, quad.y0));
        // Note we don't need to flip anything about the quads world space positions even though stb_truetype assumes
        // origin top left and +Y is down. The rendering position data will work.
        Vec2 qorigin = {0};
        qorigin.x = quad.x0 + (0.5f * qdims.x);
        // We need to flip the y position coords since stb_truetype assumes that positioning is relative to a
        // baseline where the origin is top left and +y is down. Our pos.y is our baseline, so to reflect
        // we take the height relative to the baseline and subtract that from the baseline. Think of this as if
        // we were in the coordinate space of stb_truetype. we have a baseline pos.y and some position quad.y1
        // which is the highest y position of the glyph. We need to subtract the height delta from the baseline
        // to reflect in stb_truetype coordinate system which will result in upright positions in lcore engine.
        f32 height_relative_to_baseline = quad.y1 - baseline_pos.y;
        qorigin.y = (baseline_pos.y - height_relative_to_baseline) + (0.5f * qdims.y);

        // Don't need to modify anything to calculate the dims since dimensions are not in reference of a particular
        // coordinate system.
        Vec2 quvdims = Vec2Sub(Vec2Lit(quad.s1, quad.t1), Vec2Lit(quad.s0, quad.t0));
        Vec2 quvoffset = {0};
        quvoffset.x = quad.s0;
        // We do need to flip the y text coords because we have flipped the texture after baking.
        // 1 - t0 will map the top left of the sub rect to the new top left of the flipped texture.
        // By subtracting the height we then get the bottom left corner which is what the engine expects.
        quvoffset.y = 1.0f - quad.t0 - quvdims.y;

        // if (gstate->debug) {
        //     DrawQuad(Vec2ToVec3(qorigin), qdims.x, qdims.y, Mat4Identity(), LRED);
        // }
        DrawLTexturePro2(font.tx.id, quvoffset, quvdims, qorigin, qdims, 0.0f, color);
    }
    return baseline_pos;
}

// This will return the dimensions of the quad that would be necessary to span the entire text (i.e. if somehow
// the text was baked into a texture, what quad dims would be needed to draw the texture).
LCORE Vec2 LFontMeasureText(LFont font, char* text, i32 len) {
    Vec2 tl = Vec2Zero();
    Vec2 br = Vec2Zero();

    Vec2 baseline_pos = Vec2Zero();

    for (i32 i = 0; i < len; ++i) {
        stbtt_aligned_quad quad = {0};
        // The y position passed in here is the baseline of the glpyhs (i.e. the common line they will sit on)
        stbtt_GetBakedQuad(font.cdata, font.tx.width, font.tx.height, text[i] - 32, &baseline_pos.x, &baseline_pos.y,
                           &quad, true);

        tl.x = f32min(tl.x, quad.x0);
        tl.y = f32min(tl.y, quad.y0);

        br.x = f32max(br.x, quad.x1);
        br.y = f32max(br.y, quad.y1);
    }

    Vec2 dims = Vec2Sub(br, tl);
    return dims;
}

// Assumes the font texture contains the ascii printable characters on a single line.
// Characters (note the space beginning the sequence is included):
// !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
LCORE void DrawLText(char* text, u32 len, Vec2 pos, Vec4 color) {
    DrawLTextEx(GetLFont(LFONT_DEFAULT), text, len, Vec2Lit(pos.x, pos.y), color);
}

LCORE void DrawLTextF(Vec2 pos, u32 fontsize, Vec4 color, const char* format, ...) {
    char buffer[1024];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    DrawLText(buffer, strlen(buffer), pos, color);
}

////////////////////////////////////////////////////////////
// S:Sound

typedef enum {
    SOUND_NONE = 0,
    SOUND_BEEP,
    SOUND_JUMP,
    SOUND_HIT_GOAL,
    SOUND_START_LEVEL,
    SOUND_WATER_DEATH,
    SOUND_CAR_DEATH,
    SOUND_GAME_OVER,
    SOUND_COUNT,
} SoundTag;

static SoundId SOUNDS[SOUND_COUNT] = {0};

#define GetSound(sid) (SOUNDS[(sid)])

static void LoadSounds() {
    ClearLSounds();
    SOUNDS[SOUND_NONE] = 0;
    SOUNDS[SOUND_BEEP] = LoadLSound("assets/sounds/beep.wav");
    SOUNDS[SOUND_JUMP] = LoadLSound("assets/sounds/jump.wav");
    SOUNDS[SOUND_HIT_GOAL] = LoadLSound("assets/sounds/hit_goal.wav");
    SOUNDS[SOUND_START_LEVEL] = LoadLSound("assets/sounds/start_level.wav");
    SOUNDS[SOUND_WATER_DEATH] = LoadLSound("assets/sounds/water_death.wav");
    SOUNDS[SOUND_CAR_DEATH] = LoadLSound("assets/sounds/car_death.wav");
    SOUNDS[SOUND_GAME_OVER] = LoadLSound("assets/sounds/game_over.wav");
}

////////////////////////////////////////////////////////////
typedef struct {
    Vec3 p;
    Vec3 v;
    Vec3 scale;
    Vec4 color;
    Mat4 orient;

    f32 lifetime;
    f32 lifedt;
} Particle;

#define MAX_PARTICLES 100

typedef struct {
    Particle particles[MAX_PARTICLES];
    u32 plen;
    u32 pcap;
} ParticleSystem;

static ParticleSystem PARTICLES;

#define GetParticleSystem() (&PARTICLES)

static void ParticleSystemInit() {
    ParticleSystem* particles = &PARTICLES;
    memset(particles, 0, sizeof(*particles));
    particles->plen = 0;
    particles->pcap = MAX_PARTICLES;
}

LCORE Particle* AllocateParticle() {
    ParticleSystem* particles = GetParticleSystem();
    Assert(particles->plen < particles->pcap);
    Particle* particle = &particles->particles[particles->plen++];
    memset(particle, 0, sizeof(Particle));
    particle->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
    particle->orient = Mat4Identity();
    return particle;
}

LCORE void ParticleSystemTick(f32 dt) {
    u32 to_delete[MAX_PARTICLES] = {0};
    u32 dlen = 0;

    ParticleSystem* particles = &PARTICLES;
    for (u32 i = 0; i < particles->plen; ++i) {
        Particle* particle = &particles->particles[i];
        particle->lifedt += dt;
        if (particle->lifedt >= particle->lifetime) {
            to_delete[dlen++] = i;
            continue;
        }

        particle->p = Vec3Add(particle->p, Vec3Scale(particle->v, dt));
        // particle->color.a = f32lerp(1.0f, 0.1f, particle->lifedt / particle->lifetime);

        DrawCmd* cmd = AddDrawMeshCmd();
        cmd->meshcmd.modelm = Mat4SVRT(particle->scale, particle->orient, particle->p);
        cmd->meshcmd.meshid = GetMesh(M_QUAD);
        cmd->meshcmd.color = particle->color;
    }

    // Remove particles by swapping with end. NOTE this means we shouldn't be keeping persistent pointers
    // to these particles around.
    for (i32 i = dlen - 1; i >= 0; --i) {
        Particle temp = particles->particles[particles->plen - 1];
        particles->particles[particles->plen - 1] = particles->particles[to_delete[i]];
        particles->particles[to_delete[i]] = temp;
        --particles->plen;
    }
}

////////////////////////////////////////////////////////////

// Rect in aseprite pos space.
typedef struct {
    Vec2 lc;  // bottom left corner.
    Vec2 r;   // width + height.
} AseRect;

typedef struct {
    AseRect start;  // Start Aserect of the animation.
    Vec2 step;      // step to move each frame in aseprite coord space.
    u32 nframes;
} AnimationAseRect;

typedef enum {
    TI_NONE = 0,
    TI_SPRITE,
    TI_ANIMATION,
    TI_COUNT,
} TextureInfoType;

// Abstraction for texture data for a mesh. Essentially you can think of this at the moment as the information
// required to subselect a rect inside of a 2d texture (animations are a subselected rect that are stepped
// along through the texture at some time interval).
typedef struct {
    TextureId txid;

    TextureInfoType type;
    union {
        struct {
            // Start min and dims of the window in uv space.
            Vec2 minuv;
            Vec2 dimsuv;
        } sprite;

        struct {
            // Start min and dims of the first frame in uv space.
            Vec2 minuv;
            Vec2 dimsuv;

            // How much to step each frame in uv.
            Vec2 stepuv;

            // n frames in animation.
            u32 nframes;
            // Index of what frame we are on in the animation.
            u32 frame;
            // Time spent in current frame.
            f32 frame_dt;

            // // Delta time we've currently spent playing animation.
            // f32 frames_dt;
            // // Maximum time duration of the animation.
            // f32 frames_time;
        } animation;
    };

    Vec4 tint;  // Color.
} TextureInfo;

static void TextureInfoDefault(TextureInfo* txinfo) {
    TextureInfo res = {0};
    res.txid = GetTexture(TX_DEFAULT).id;
    res.type = TI_SPRITE;
    res.sprite.minuv = Vec2Zero();
    res.sprite.dimsuv = Vec2Lit(1.0f, 1.0f);
    res.tint = LCLEAR;

    *txinfo = res;
}

typedef struct {
    Vec3 pos;
    Vec3 scale;
    Mat4 orient;

    Vec3 posa;
    Vec3 posb;
    f32 posdt;

    //////////////////////////////////////
    // Final drawing related fields. For now think of this as the destination rectangle + texture coords
    LMesh mid;
    // Min (generally lower left hand corner) of the texture selection rect in uv space.
    Vec2 uvoffset;
    // Dimensions of the texture selection rect in uv space.
    Vec2 uvscale;
    //////////////////////////////////////

    //////////////////////////////////////
    // Add static offset to the visual position of the graphics. Useful for example when you want to
    // offset a sprite so that it lines up with a different sprite of a different bounding box shape.
    Vec3 pos_offset;
    //////////////////////////////////////

    TextureInfo txinfo;
} GraphicsInfo;

static void GraphicsInfoMemset(GraphicsInfo* graphics) {
    memset(graphics, 0, sizeof(GraphicsInfo));
    graphics->orient = Mat4Identity();
    graphics->pos = Vec3Zero();
    graphics->scale = Vec3Lit(1.0f, 1.0f, 1.0f);

    graphics->uvscale = Vec2Lit(1.0f, 1.0f);
    graphics->uvoffset = Vec2Lit(0.0f, 0.0f);

    graphics->pos_offset = Vec3Zero();

    TextureInfoDefault(&graphics->txinfo);
}

// min - bottom left hand corner of sprite (in png space, not uvspace)
// dims - full width/height of sprite in png space not uvspace.
LCORE void GraphicsInfoSetSprite(GraphicsInfo* graphics, LTexture tx, Vec2 min, Vec2 dims, f32 orient, f32 scale) {
    graphics->scale = Vec3Lit(dims.x * scale, dims.y * scale, 1.0f);
    graphics->orient = Mat4Rotate(orient, Vec3Lit(0.0f, 0.0f, 1.0f));

    graphics->mid = GetMesh(M_QUAD);

    TextureInfo res = {0};
    res.txid = tx.id;
    res.type = TI_SPRITE;
    res.sprite.minuv = Vec2Lit(min.x / tx.width, min.y / tx.height);
    res.sprite.dimsuv = Vec2Lit(dims.x / tx.width, dims.y / tx.height);
    res.tint = LCLEAR;
    graphics->txinfo = res;
}

// Remember rect coords are in aseprite space.
// TODO: This feels inefficient. Feels like what I really want is to be able to save visual info loaded from aseprite
// once as a constant and set an entities' visuals to the constant. problem is that I can't save entire graphics info
// or else when I set entity graphics, I overwrite position. So I need some sort of notion for the "static" entity
// data.
// NOTE: We don't overwrite all graphics info data because we don't set GraphicsInfoMemset, so data like position
// is preserved.
LCORE void GraphicsInfoSetSpriteFromAseprite(GraphicsInfo* graphics, LTexture tx, AseRect src, f32 orient, f32 scale) {
    graphics->scale = Vec3Lit(src.r.x * scale, src.r.y * scale, 1.0f);
    graphics->orient = Mat4Rotate(orient, Vec3Lit(0.0f, 0.0f, 1.0f));

    graphics->mid = GetMesh(M_QUAD);

    src.lc = AsepritePosToFlipped(tx, src.lc);

    TextureInfo res = {0};
    res.txid = tx.id;
    res.type = TI_SPRITE;
    res.sprite.minuv = Vec2Lit(src.lc.x / tx.width, src.lc.y / tx.height);
    res.sprite.dimsuv = Vec2Lit(src.r.x / tx.width, src.r.y / tx.height);
    res.tint = LCLEAR;
    graphics->txinfo = res;
}

LCORE void GraphicsInfoSetAnimationFromAseprite(GraphicsInfo* graphics, LTexture tx, AnimationAseRect animation,
                                                f32 orient, f32 scale) {
    graphics->scale = Vec3Lit(animation.start.r.x * scale, animation.start.r.y * scale, 1.0f);
    graphics->orient = Mat4Rotate(orient, Vec3Lit(0.0f, 0.0f, 1.0f));

    graphics->mid = GetMesh(M_QUAD);

    animation.start.lc = AsepritePosToFlipped(tx, animation.start.lc);

    f32 width_inv = 1.0f / tx.width;
    f32 height_inv = 1.0f / tx.height;
    TextureInfo txinfo = {0};
    txinfo.txid = tx.id;
    txinfo.type = TI_ANIMATION;
    txinfo.animation.minuv = Vec2Lit(animation.start.lc.x * width_inv, animation.start.lc.y * height_inv);
    txinfo.animation.dimsuv = Vec2Lit(animation.start.r.x * width_inv, animation.start.r.y * height_inv);
    txinfo.animation.stepuv = Vec2Lit(animation.step.x * width_inv, animation.step.y * height_inv);
    txinfo.animation.nframes = animation.nframes;
    txinfo.animation.frame = 0;
    txinfo.animation.frame_dt = 0.0f;
    txinfo.tint = LCLEAR;

    graphics->txinfo = txinfo;
}

LCORE void GraphicsInfoSetTexture(GraphicsInfo* graphics, LTexture tx, f32 orient, f32 scale) {
    GraphicsInfoSetSprite(graphics, tx, Vec2Zero(), Vec2Lit(tx.width, tx.height), orient, scale);
}

LCORE void GraphicsInfoSetShape(GraphicsInfo* graphics, LMesh mesh, Vec3 scale, Vec4 color) {
    graphics->mid = mesh;
    graphics->scale = scale;
    graphics->txinfo.tint = color;
}

////////////////////////////////////////////////////////////

typedef enum {
    MT_NONE = 0,
    MT_RABBIT,
    MT_BAT,
    MT_DOG,
    MT_BIRD,
    MT_EGG,
    MT_CHICK,
    MT_COUNT,
} MobType;

////////////////////////////////////////////////////////////

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
    EFLAG_COLLIDER = 1 << 3,
    EFLAG_PROJECTILE = 1 << 4,
    EFLAG_WALL = 1 << 5,
    EFLAG_MOB = 1 << 6,
    EFLAG_CLIMABLE = 1 << 7,
    EFLAG_EXIT = 1 << 8,
    EFLAG_IGNORE_WALL_ON_HIT = 1 << 9,
    EFLAG_SPAWNER = 1 << 10,
} EntityFlag;

typedef u32 EntityId;
static const EntityId NULLID = 0;

typedef struct {
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
    Vec3 forces;
    f32 friction;
    f32 restitution;

    // TODO: Deprecate
    LMesh mesh;

    Vec4 color;

    Polyhedron shape;

    GraphicsInfo graphics;

    f32 active_dt;

    /////////////////////////////////////////////////////////////////
    // Gameplay fields.
    f32 jumping_dt;

    i32 health;

    u32 room_link;  // Used for room exit triggers.
    bool facing_left;

    EntityId attacker_id;
    Vec3 attack_dir;

    MobType mobtype;
    f32 movement_cooldown_dt;
    f32 wakeup_dt;
    bool awake;

    f32 idle_dt;
    f32 shooting_dt;
    i32 nshots;

    bool pos_parametric;
    f32 parametric_dt;
    Vec3 pos_start;
    f32 lifetime;

    EntityId spawnedid;

    EntityId birdid;
    EntityId eggid;

    bool broken;
    f32 broken_dt;
} Entity;

#define ENTITY_CAP 1024
#define ENTITY_INACTIVE_TO_BE_CAP 20

typedef struct {
    Entity entities[ENTITY_CAP];
    u32 cap;

    EntityId inactive_to_be[ENTITY_INACTIVE_TO_BE_CAP];
    u32 inactive_cap;
    u32 inactive_len;
} EntityManager;

static EntityManager EntityM = {0};

#define GetEntityManager() &EntityM

#define GetNumEntities() (EntityM.cap)

static void EntitySystemInit() {
    memset(&EntityM, 0, sizeof(EntityM));
    EntityM.cap = ENTITY_CAP;
    EntityM.inactive_cap = ENTITY_INACTIVE_TO_BE_CAP;
}

static Entity* GetEntity(EntityId id) {
    EntityManager* em = GetEntityManager();
    Entity* entity = &em->entities[id];
    return entity;
}

static EntityId NextEntityId() {
    EntityManager* em = GetEntityManager();
    // Always have a null entity at position 0
    for (uint32_t i = NULLID + 1; i < ArraySizeOf(em->entities); ++i) {
        Entity* entity = &em->entities[i];
        if (!TestFlag(entity->flags, EFLAG_ACTIVE)) {
            return i;
        }
    }
    Assert(false);
    return 0;
}

LCORE Entity* AllocateEntity() {
    EntityId id = NextEntityId();
    Entity* entity = GetEntity(id);
    Assert(entity != NULL);
    memset(entity, 0, sizeof(*entity));
    entity->id = id;
    entity->flags = EFLAG_ACTIVE;
    entity->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
    entity->orient = Mat4Identity();
    entity->color = Vec4Lit(0.0f, 0.0f, 0.0f, 1.0f);
    PolyhedronInit(&entity->shape);
    GraphicsInfoMemset(&entity->graphics);
    FlagOn(entity->flags, EFLAG_DRAW);

    entity->attacker_id = NULLID;
    entity->attack_dir = Vec3Zero();
    return entity;
}

LCORE void SoftDeactivateEntity(EntityId id) {
    EntityManager* em = GetEntityManager();
    Assert(em->inactive_len < em->inactive_cap);
    em->inactive_to_be[em->inactive_len++] = id;
}

LCORE void DeactivateEntities() {
    EntityManager* em = GetEntityManager();
    for (u32 i = 0; i < em->inactive_len; ++i) {
        Entity* e = &em->entities[em->inactive_to_be[i]];
        FlagOff(e->flags, EFLAG_ACTIVE);
    }
    em->inactive_len = 0;
}

////////////////////////////////////////////////////
typedef struct {
    EntityId id0;
    EntityId id1;

    f32 depth;
    Vec3 normal;
} CollisionInfo;

////////////////////////////////////////////////////

LCORE Mat4 GetCameraMat(Vec3 eye, Vec3 orient) {
    return Mat4Invert(Mat4Multiply(Mat4TranslateV(eye), Mat4RotateEuler(orient)));
}

////////////////////////////////////////////////////

typedef struct {
    Vec3 eye;     // world space position of the camera.
    Vec3 orient;  // vector containing euler angle rotation vector. for identity rotation, set to Vec3Zero
} LCamera;

// static Mat4 LCameraGetView(LCamera c) {
//     return Mat4Invert(Mat4Multiply(Mat4TranslateV(c.eye), Mat4RotateEuler(c.orient)));
// }

#define Grid2WorldEx(pos, w, h, d) (Vec3Lit((pos).x * (w), (pos).y * (h), (pos).z * (d)))

static const f32 GRID_WIDTH = 16.0f;
static const f32 GRID_HEIGHT = 16.0f;
static const f32 GRID_DEPTH = 1.0f;
#define Grid2World(pos) (Grid2WorldEx(pos, GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH))

#define World2GridEx(pos, w, h, d) (Vec3Lit((u32)((pos).x / (w)), (u32)((pos).y / (h)), (u32)((pos).z) / (d)))
#define World2Grid(pos) (World2GridEx(pos, GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH))

////////////////////////////////////////////////////

// A blueprint containing all info necessary to create an entity.
typedef struct {
    Vec3 pos;
    Polyhedron shape;

    u32 room_idx_to_be;

    MobType mobtype;
} EntitySpec;

typedef struct {
    // How to find. make a 128 x 120 selection in aseprite. the pixel value that is 1, 1 off from the
    // corner you finish selecting is the start.
    // This will end up being the origin in worldspace.
    Vec3 camera_start;
    AABB2D room_bounds;  // The bounding box of the room. Only used for non static camera rooms.
    bool static_camera;

    Vec3 player_start;

    EntitySpec enemies[10];
    u32 elen;

    EntitySpec spawners[10];
    u32 spawnerslen;

    EntitySpec next_room_exit_spec;
    EntitySpec prev_room_exit_spec;
} Room;

typedef struct {
    LTexture background;

    EntitySpec walls[64];
    u32 wlen;

    EntitySpec ladders[10];
    u32 llen;

    Room rooms[10];
    u32 rlen;
} Stage;

typedef enum {
    STAGE_NONE = 0,
    STAGE_WOODMAN,
    STAGE_LADDER_TEST,
    STAGE_ROOM_TEST,
    STAGE_COUNT,
} StageId;

static Stage* GetStage(StageId sid) {
    static Stage STAGES[STAGE_COUNT] = {0};
    static bool INIT = false;

    if (!INIT) {
        INIT = true;

        {
            LTexture tx = GetTexture(TX_WOODMAN_STAGE);
            Stage stage = {0};
            stage.background = tx;

            // Walls
            {
                EntitySpec* specs = stage.walls;
                u32* len = &stage.wlen;
                u32 cap = ArraySizeOf(stage.walls);
                Vec2 positions[][2] = {
                    {Vec2Lit(1.0f, 256.0f), Vec2Lit(256.0f, 193.0f)},
                    {Vec2Lit(257.0f, 256.0f), Vec2Lit(320.0f, 177.0f)},
                    {Vec2Lit(320.0f, 256.0f), Vec2Lit(421.0f, 193.0f)},
                    {Vec2Lit(401.0f, 256.0f), Vec2Lit(489.0f, 177.0f)},
                    {Vec2Lit(481.0f, 256.0f), Vec2Lit(704.0f, 161.0f)},
                    {Vec2Lit(699.0f, 256.0f), Vec2Lit(832.0f, 177.0f)},
                    {Vec2Lit(828.0f, 346.0f), Vec2Lit(1184.0f, 193.0f)},
                    {Vec2Lit(0.0f, 256.0f), Vec2Lit(0.0f, 0.0f)},
                    {Vec2Lit(1201.0f, 302.0f), Vec2Lit(1280.0f, 1.0f)},

                    // Room 1
                    {Vec2Lit(1169.0f, 374.0f), Vec2Lit(1216.0f, 337.0f)},
                    {Vec2Lit(1249.0f, 480.0f), Vec2Lit(1280.0f, 291.0f)},
                    {Vec2Lit(1089.0f, 479.0f), Vec2Lit(1279.0f, 449.0f)},
                    {Vec2Lit(1185.0f, 477.0f), Vec2Lit(1216.0f, 433.0f)},
                    {Vec2Lit(1121.0f, 477.0f), Vec2Lit(1152.0f, 433.0f)},
                    {Vec2Lit(1105.0f, 385.0f), Vec2Lit(1136.0f, 334.0f)},
                    {Vec2Lit(1026.0f, 368.0f), Vec2Lit(1104.0f, 335.0f)},
                    {Vec2Lit(1026.0f, 634.0f), Vec2Lit(1056.0f, 363.0f)},

                    // Room 2
                    {Vec2Lit(1029.0f, 686.0f), Vec2Lit(1088.0f, 609.0f)},
                    {Vec2Lit(1037.0f, 720.0f), Vec2Lit(1104.0f, 673.0f)},
                    {Vec2Lit(1079.0f, 725.0f), Vec2Lit(1190.0f, 705.0f)},
                    {Vec2Lit(1185.0f, 717.0f), Vec2Lit(1453.0f, 673.0f)},

                    // Room 3
                    {Vec2Lit(1441.0f, 738.0f), Vec2Lit(1504.0f, 641.0f)},
                    {Vec2Lit(1502.0f, 734.0f), Vec2Lit(1572.0f, 705.0f)},

                    // Room 4
                    {Vec2Lit(1569.0f, 720.0f), Vec2Lit(1626.0f, 673.0f)},
                    {Vec2Lit(1601.0f, 681.0f), Vec2Lit(1735.0f, 641.0f)},
                    {Vec2Lit(1729.0f, 666.0f), Vec2Lit(2017.0f, 609.0f)},

                    // Room 5
                    {Vec2Lit(2001.0f, 628.0f), Vec2Lit(2048.0f, 377.0f)},
                    {Vec2Lit(2001.0f, 631.0f), Vec2Lit(2042.0f, 331.0f)},

                    // Room 6
                    {Vec2Lit(1867.0f, 528.0f), Vec2Lit(1984.0f, 449.0f)},
                    {Vec2Lit(1832.0f, 462.0f), Vec2Lit(1920.0f, 433.0f)},
                    {Vec2Lit(1798.0f, 445.0f), Vec2Lit(1840.0f, 82.0f)},
                    {Vec2Lit(1857.0f, 326.0f), Vec2Lit(1927.0f, 193.0f)},

                    // Room 7
                    {Vec2Lit(1921.0f, 212.0f), Vec2Lit(1994.0f, 161.0f)},
                    {Vec2Lit(1985.0f, 176.0f), Vec2Lit(2048.0f, 129.0f)},
                    {Vec2Lit(2045.0f, 134.0f), Vec2Lit(2176.0f, 129.0f)},
                    {Vec2Lit(2209.0f, 102.0f), Vec2Lit(2368.0f, 97.0f)},
                    {Vec2Lit(2369.0f, 166.0f), Vec2Lit(2560.0f, 161.0f)},
                    {Vec2Lit(2593.0f, 134.0f), Vec2Lit(2688.0f, 129.0f)},
                    {Vec2Lit(2689.0f, 86.0f), Vec2Lit(2752.0f, 81.0f)},
                    {Vec2Lit(2753.0f, 92.0f), Vec2Lit(3024.0f, 65.0f)},
                    {Vec2Lit(3041.0f, 473.0f), Vec2Lit(3072.0f, 27.0f)},

                };
                for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                    EntitySpec spec = {0};
                    spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                    spec.shape.type = PH_AABB2D;
                    spec.shape.aabb2d.c = Vec2Zero();
                    spec.shape.aabb2d.r = bb.r;
                    Assert(*len < cap);
                    specs[(*len)++] = spec;
                }
            }

            // Ladders
            {
                EntitySpec* specs = stage.ladders;
                u32* len = &stage.llen;
                u32 cap = ArraySizeOf(stage.ladders);
                Vec2 positions[][2] = {
                    {Vec2Lit(1185.0f, 336.0f), Vec2Lit(1200.0f, 187.0f)},

                    // Room 1
                    {Vec2Lit(1073.0f, 560.0f), Vec2Lit(1088.0f, 449.0f)},

                    // Room 5
                    {Vec2Lit(1985.0f, 608.0f), Vec2Lit(2000.0f, 444.0f)},

                    // Room 6
                    {Vec2Lit(1841.0f, 384.0f), Vec2Lit(1856.0f, 190.0f)},

                    // Room 7
                    {Vec2Lit(3025.0f, 352.0f), Vec2Lit(3040.0f, 61.0f)},

                };
                for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                    EntitySpec spec = {0};
                    spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                    spec.shape.type = PH_AABB2D;
                    spec.shape.aabb2d.c = Vec2Zero();
                    spec.shape.aabb2d.r = bb.r;
                    Assert(*len < cap);
                    specs[(*len)++] = spec;
                }
            }

            // Room 0
            {
                Room room = {0};
                // Camera info
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1.0f, 1.0f), Vec2Lit(256.0f, 240.0f));
                        room.camera_start = Vec3Lit(bb.c.x, bb.c.y, 1.0f);
                    }
                    room.static_camera = false;
                    room.room_bounds = AABB2DFromAsepriteBB(tx, Vec2Lit(1.0f, 240.0f), Vec2Lit(1280.0f, 1.0f));
                }
                {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(118.0f, 192.0f), Vec2Lit(141.0f, 169.0f));
                    room.player_start = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                }

                // Enemies
                {
                    EntitySpec* specs = room.enemies;
                    u32* len = &room.elen;
                    u32 cap = ArraySizeOf(room.enemies);
                    Vec2 positions[][2] = {
                        {Vec2Lit(441.0f, 176.0f), Vec2Lit(472.0f, 145.0f)},
                        {Vec2Lit(757.0f, 176.0f), Vec2Lit(788.0f, 145.0f)},
                    };
                    MobType mobtypes[] = {
                        MT_RABBIT,
                        MT_RABBIT,
                    };
                    for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                        AABB2D bb =
                            AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.mobtype = mobtypes[i];
                        Assert(*len < cap);
                        specs[(*len)++] = spec;
                    }
                }

                // Exits.
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(Vec2Lit(1185.0f, 240.0f)),
                                                         AsepritePxTR(Vec2Lit(1201.0f, 233.0f)));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.room_idx_to_be = 1;

                        room.next_room_exit_spec = spec;
                    }
                }

                stage.rooms[stage.rlen++] = room;
            }

            // Room 1
            {
                Room room = {0};
                // Camera info
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1025.0f, 480.0f), Vec2Lit(1280.0f, 241.0f));
                        room.camera_start = Vec3Lit(bb.c.x, bb.c.y, 1.0f);
                    }
                    room.static_camera = true;
                }
                {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1185.0f, 336.0f), Vec2Lit(1208.0f, 313.0f));
                    room.player_start = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                }

                // Enemies
                {
                    EntitySpec* specs = room.enemies;
                    u32* len = &room.elen;
                    u32 cap = ArraySizeOf(room.enemies);
                    Vec2 positions[][2] = {
                        {Vec2Lit(1068.0f, 384.0f), Vec2Lit(1083.0f, 369.0f)},
                        {Vec2Lit(1114.0f, 401.0f), Vec2Lit(1129.0f, 386.0f)},
                        {Vec2Lit(1147.0f, 369.0f), Vec2Lit(1162.0f, 354.0f)},
                    };
                    MobType mobtypes[] = {
                        MT_BAT,
                        MT_BAT,
                        MT_BAT,
                    };
                    for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                        AABB2D bb =
                            AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.mobtype = mobtypes[i];
                        Assert(*len < cap);
                        specs[(*len)++] = spec;
                    }
                }

                // Exits.
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(Vec2Lit(1073.0f, 483.0f)),
                                                         AsepritePxTR(Vec2Lit(1088.0f, 488.0f)));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.room_idx_to_be = 2;

                        room.next_room_exit_spec = spec;
                    }
                }

                stage.rooms[stage.rlen++] = room;
            }

            // Room 2
            {
                Room room = {0};
                // Camera info
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1025.0f, 752.0f), Vec2Lit(1280.0f, 513.0f));
                        room.camera_start = Vec3Lit(bb.c.x, bb.c.y, 1.0f);
                    }
                    room.static_camera = true;
                }
                {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1059.0f, 608.0f), Vec2Lit(1082.0f, 585.0f));
                    room.player_start = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                }

                // Enemies
                {
                    EntitySpec* specs = room.enemies;
                    u32* len = &room.elen;
                    u32 cap = ArraySizeOf(room.enemies);
                    Vec2 positions[][2] = {
                        {Vec2Lit(1199.0f, 672.0f), Vec2Lit(1230.0f, 641.0f)},
                    };
                    MobType mobtypes[] = {
                        MT_DOG,
                    };
                    for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                        AABB2D bb =
                            AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.mobtype = mobtypes[i];
                        Assert(*len < cap);
                        specs[(*len)++] = spec;
                    }
                }

                // Exits.
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(Vec2Lit(1277.0f, 672.0f)),
                                                         AsepritePxTR(Vec2Lit(1283.0f, 639.0f)));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.room_idx_to_be = 3;

                        room.next_room_exit_spec = spec;
                    }
                }

                stage.rooms[stage.rlen++] = room;
            }

            // Room 3
            {
                Room room = {0};
                // Camera info
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1282.0f, 752.0f), Vec2Lit(1537.0f, 513.0f));
                        room.camera_start = Vec3Lit(bb.c.x, bb.c.y, 1.0f);
                    }
                    room.static_camera = true;
                }
                {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1294.0f, 672.0f), Vec2Lit(1317.0f, 649.0f));
                    room.player_start = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                }

                // Enemies
                {
                    EntitySpec* specs = room.enemies;
                    u32* len = &room.elen;
                    u32 cap = ArraySizeOf(room.enemies);
                    Vec2 positions[][2] = {
                        {Vec2Lit(1441.0f, 640.0f), Vec2Lit(1504.0f, 609.0f)},
                    };
                    MobType mobtypes[] = {
                        MT_DOG,
                    };
                    for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                        AABB2D bb =
                            AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.mobtype = mobtypes[i];
                        Assert(*len < cap);
                        specs[(*len)++] = spec;
                    }
                }

                // Exits.
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(Vec2Lit(1533.0f, 704.0f)),
                                                         AsepritePxTR(Vec2Lit(1541.0f, 640.0f)));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.room_idx_to_be = 4;

                        room.next_room_exit_spec = spec;
                    }
                }

                stage.rooms[stage.rlen++] = room;
            }

            // Room 4
            {
                Room room = {0};
                // Camera info
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1541.0f, 752.0f), Vec2Lit(1795.0f, 513.0f));
                        room.camera_start = Vec3Lit(bb.c.x, bb.c.y, 1.0f);
                    }
                    room.static_camera = true;
                }
                {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1543.0f, 704.0f), Vec2Lit(1566.0f, 681.0f));
                    room.player_start = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                }

                // Enemies
                {
                    EntitySpec* specs = room.enemies;
                    u32* len = &room.elen;
                    u32 cap = ArraySizeOf(room.enemies);
                    Vec2 positions[][2] = {
                        {Vec2Lit(1729.0f, 608.0f), Vec2Lit(1792.0f, 577.0f)},
                    };
                    MobType mobtypes[] = {
                        MT_DOG,
                    };
                    for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                        AABB2D bb =
                            AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.mobtype = mobtypes[i];
                        Assert(*len < cap);
                        specs[(*len)++] = spec;
                    }
                }

                // Exits.
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(Vec2Lit(1785.0f, 608.0f)),
                                                         AsepritePxTR(Vec2Lit(1794.0f, 529.0f)));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.room_idx_to_be = 5;

                        room.next_room_exit_spec = spec;
                    }
                }

                stage.rooms[stage.rlen++] = room;
            }

            // Room 5
            {
                Room room = {0};
                // Camera info
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1793.0f, 752.0f), Vec2Lit(2048.0f, 513.0f));
                        room.camera_start = Vec3Lit(bb.c.x, bb.c.y, 1.0f);
                    }
                    room.static_camera = true;
                }
                {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1813.0f, 608.0f), Vec2Lit(1836.0f, 585.0f));
                    room.player_start = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                }

                // Enemies
                {
                    EntitySpec* specs = room.enemies;
                    u32* len = &room.elen;
                    u32 cap = ArraySizeOf(room.enemies);
                    Vec2 positions[][2] = {};
                    for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                        AABB2D bb =
                            AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        Assert(*len < cap);
                        specs[(*len)++] = spec;
                    }
                }

                // Exits.
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(Vec2Lit(1983.0f, 515.0f)),
                                                         AsepritePxTR(Vec2Lit(2003.0f, 510.0f)));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.room_idx_to_be = 6;

                        room.next_room_exit_spec = spec;
                    }
                }

                stage.rooms[stage.rlen++] = room;
            }

            // Room 6
            {
                Room room = {0};
                // Camera info
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1793.0f, 512.0f), Vec2Lit(2048.0f, 273.0f));
                        room.camera_start = Vec3Lit(bb.c.x, bb.c.y, 1.0f);
                    }
                    room.static_camera = true;
                }
                {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1962.0f, 448.0f), Vec2Lit(1985.0f, 425.0f));
                    room.player_start = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                }

                // Enemies
                {
                    EntitySpec* specs = room.enemies;
                    u32* len = &room.elen;
                    u32 cap = ArraySizeOf(room.enemies);
                    Vec2 positions[][2] = {
                        {Vec2Lit(1864, 342), Vec2Lit(1879, 327)},
                        {Vec2Lit(1906, 374), Vec2Lit(1921, 359)},
                    };
                    MobType mobtypes[] = {
                        MT_BAT,
                        MT_BAT,
                    };
                    for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                        AABB2D bb =
                            AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.mobtype = mobtypes[i];
                        Assert(*len < cap);
                        specs[(*len)++] = spec;
                    }
                }

                // Exits.
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(Vec2Lit(1838.0f, 275.0f)),
                                                         AsepritePxTR(Vec2Lit(1859.0f, 266.0f)));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.room_idx_to_be = 7;

                        room.next_room_exit_spec = spec;
                    }
                }

                stage.rooms[stage.rlen++] = room;
            }

            // Room 7
            {
                Room room = {0};
                // Camera info
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1793.0f, 240.0f), Vec2Lit(2048.0f, 1.0f));
                        room.camera_start = Vec3Lit(bb.c.x, bb.c.y, 1.0f);
                    }
                    room.static_camera = false;
                    room.room_bounds = AABB2DFromAsepriteBB(tx, Vec2Lit(1793.0f, 240.0f), Vec2Lit(3072.0f, 1.0f));
                }
                {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1860.0f, 192.0f), Vec2Lit(1883.0f, 169.0f));
                    room.player_start = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                }

                // Enemies
                {
                    EntitySpec* specs = room.enemies;
                    u32* len = &room.elen;
                    u32 cap = ArraySizeOf(room.enemies);
                    Vec2 positions[][2] = {
                        // {Vec2Lit(1864, 342), Vec2Lit(1879, 327)},
                    };
                    for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                        AABB2D bb =
                            AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        Assert(*len < cap);
                        specs[(*len)++] = spec;
                    }
                }

                // Spawners
                {
                    EntitySpec* specs = room.spawners;
                    u32* len = &room.spawnerslen;
                    u32 cap = ArraySizeOf(room.spawners);
                    Vec2 positions[][2] = {
                        {Vec2Lit(2753, 62), Vec2Lit(2768, 50)},
                    };
                    for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                        AABB2D bb =
                            AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        Assert(*len < cap);
                        specs[(*len)++] = spec;
                    }
                }

                // Exits.
                {
                    // {
                    //     AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(Vec2Lit(1277.0f, 672.0f)),
                    //                                      AsepritePxTR(Vec2Lit(1283.0f, 639.0f)));
                    //     EntitySpec spec = {0};
                    //     spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                    //     spec.shape.type = PH_AABB2D;
                    //     spec.shape.aabb2d.c = Vec2Zero();
                    //     spec.shape.aabb2d.r = bb.r;
                    //     spec.room_idx_to_be = 3;

                    //     room.next_room_exit_spec = spec;
                    // }
                }

                stage.rooms[stage.rlen++] = room;
            }

            STAGES[STAGE_WOODMAN] = stage;
        }

        // {
        //     LTexture tx = GetTexture(TX_WOODMAN_STAGE);
        //     Stage stage = {0};
        //     stage.background = tx;
        //     {
        //         AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(1.0f, 1.0f), Vec2Lit(256.0f, 240.0f));
        //         stage.camera_start = Vec3Lit(bb.c.x, bb.c.y, 1.0f);
        //     }
        //     {
        //         AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(118.0f, 192.0f), Vec2Lit(141.0f, 169.0f));
        //         stage.player_start = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
        //     }

        //     // Walls
        //     {
        //         EntitySpec* specs = stage.walls;
        //         u32* len = &stage.wlen;
        //         u32 cap = ArraySizeOf(stage.walls);
        //         Vec2 positions[][2] = {
        //             {Vec2Lit(1.0f, 256.0f), Vec2Lit(256.0f, 193.0f)},
        //             {Vec2Lit(257.0f, 256.0f), Vec2Lit(320.0f, 177.0f)},
        //             {Vec2Lit(320.0f, 256.0f), Vec2Lit(421.0f, 193.0f)},
        //             {Vec2Lit(401.0f, 256.0f), Vec2Lit(489.0f, 177.0f)},
        //             {Vec2Lit(481.0f, 256.0f), Vec2Lit(704.0f, 161.0f)},
        //             {Vec2Lit(699.0f, 256.0f), Vec2Lit(832.0f, 177.0f)},
        //             {Vec2Lit(828.0f, 346.0f), Vec2Lit(1184.0f, 193.0f)},
        //             {Vec2Lit(0.0f, 256.0f), Vec2Lit(0.0f, 0.0f)},
        //             {Vec2Lit(1201.0f, 302.0f), Vec2Lit(1280.0f, 1.0f)},
        //         };
        //         for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
        //             AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]),
        //             AsepritePxTR(positions[i][1])); EntitySpec spec = {0}; spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
        //             spec.shape.type = PH_AABB2D;
        //             spec.shape.aabb2d.c = Vec2Zero();
        //             spec.shape.aabb2d.r = bb.r;
        //             Assert(*len < cap);
        //             specs[(*len)++] = spec;
        //         }
        //     }

        //     // Enemies
        //     {
        //         EntitySpec* specs = stage.enemies;
        //         u32* len = &stage.elen;
        //         u32 cap = ArraySizeOf(stage.enemies);
        //         Vec2 positions[][2] = {
        //             {Vec2Lit(441.0f, 176.0f), Vec2Lit(472.0f, 145.0f)},
        //             {Vec2Lit(757.0f, 176.0f), Vec2Lit(788.0f, 145.0f)},
        //         };
        //         for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
        //             AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]),
        //             AsepritePxTR(positions[i][1])); EntitySpec spec = {0}; spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
        //             spec.shape.type = PH_AABB2D;
        //             spec.shape.aabb2d.c = Vec2Zero();
        //             spec.shape.aabb2d.r = bb.r;
        //             Assert(*len < cap);
        //             specs[(*len)++] = spec;
        //         }
        //     }

        //     // Ladders
        //     {
        //         EntitySpec* specs = stage.ladders;
        //         u32* len = &stage.llen;
        //         u32 cap = ArraySizeOf(stage.ladders);
        //         Vec2 positions[][2] = {
        //             {Vec2Lit(1185.0f, 336.0f), Vec2Lit(1200.0f, 187.0f)},
        //         };
        //         for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
        //             AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]),
        //             AsepritePxTR(positions[i][1])); EntitySpec spec = {0}; spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
        //             spec.shape.type = PH_AABB2D;
        //             spec.shape.aabb2d.c = Vec2Zero();
        //             spec.shape.aabb2d.r = bb.r;
        //             Assert(*len < cap);
        //             specs[(*len)++] = spec;
        //         }
        //     }

        //     STAGES[STAGE_WOODMAN] = stage;
        // }

        // {
        //     LTexture tx = GetTexture(TX_LADDER_TEST_STAGE);
        //     Stage stage = {0};
        //     stage.background = tx;
        //     {
        //         AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(128.0f, 271.0f), Vec2Lit(383.0f, 32.0f));
        //         stage.camera_start = Vec3Lit(bb.c.x, bb.c.y, 1.0f);
        //     }
        //     {
        //         AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(224.0f, 271.0f), Vec2Lit(247.0f, 248.0f));
        //         stage.player_start = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
        //     }

        //     // Walls
        //     {
        //         EntitySpec* specs = stage.walls;
        //         u32* len = &stage.wlen;
        //         u32 cap = ArraySizeOf(stage.walls);
        //         Vec2 positions[][2] = {
        //             {Vec2Lit(112.0f, 319.0f), Vec2Lit(367.0f, 272.0f)},
        //             {Vec2Lit(128.0f, 191.0f), Vec2Lit(159.0f, 176.0f)},
        //             {Vec2Lit(176.0f, 191.0f), Vec2Lit(255.0f, 176.0f)},
        //         };
        //         for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
        //             AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]),
        //             AsepritePxTR(positions[i][1])); EntitySpec spec = {0}; spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
        //             spec.shape.type = PH_AABB2D;
        //             spec.shape.aabb2d.c = Vec2Zero();
        //             spec.shape.aabb2d.r = bb.r;
        //             Assert(*len < cap);
        //             specs[(*len)++] = spec;
        //         }
        //     }

        //     // Enemies
        //     {
        //         EntitySpec* specs = stage.enemies;
        //         u32* len = &stage.elen;
        //         u32 cap = ArraySizeOf(stage.enemies);
        //         Vec2 positions[][2] = {};
        //         for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
        //             AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]),
        //             AsepritePxTR(positions[i][1])); EntitySpec spec = {0}; spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
        //             spec.shape.type = PH_AABB2D;
        //             spec.shape.aabb2d.c = Vec2Zero();
        //             spec.shape.aabb2d.r = bb.r;
        //             Assert(*len < cap);
        //             specs[(*len)++] = spec;
        //         }
        //     }

        //     // Ladders
        //     {
        //         EntitySpec* specs = stage.ladders;
        //         u32* len = &stage.llen;
        //         u32 cap = ArraySizeOf(stage.ladders);
        //         Vec2 positions[][2] = {
        //             {Vec2Lit(160.0f, 271.0f), Vec2Lit(175.0f, 176.0f)},
        //             {Vec2Lit(192.0f, 175.0f), Vec2Lit(207.0f, 96.0f)},
        //             {Vec2Lit(208.0f, 271.0f), Vec2Lit(223.0f, 192.0f)},
        //         };
        //         for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
        //             AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]),
        //             AsepritePxTR(positions[i][1])); EntitySpec spec = {0}; spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
        //             spec.shape.type = PH_AABB2D;
        //             spec.shape.aabb2d.c = Vec2Zero();
        //             spec.shape.aabb2d.r = bb.r;
        //             Assert(*len < cap);
        //             specs[(*len)++] = spec;
        //         }
        //     }

        //     STAGES[STAGE_LADDER_TEST] = stage;
        // }

        {
            LTexture tx = GetTexture(TX_ROOM_TEST_STAGE);
            Stage stage = {0};
            stage.background = tx;

            // Walls
            {
                EntitySpec* specs = stage.walls;
                u32* len = &stage.wlen;
                u32 cap = ArraySizeOf(stage.walls);
                Vec2 positions[][2] = {
                    {Vec2Lit(0.0f, 499.0f), Vec2Lit(110.0f, 409.0f)},
                    {Vec2Lit(100.0f, 499.0f), Vec2Lit(599.0f, 409.0f)},
                };
                for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                    EntitySpec spec = {0};
                    spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                    spec.shape.type = PH_AABB2D;
                    spec.shape.aabb2d.c = Vec2Zero();
                    spec.shape.aabb2d.r = bb.r;
                    Assert(*len < cap);
                    specs[(*len)++] = spec;
                }
            }

            // Ladders
            {
                EntitySpec* specs = stage.ladders;
                u32* len = &stage.llen;
                u32 cap = ArraySizeOf(stage.ladders);
                Vec2 positions[][2] = {};
                for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                    EntitySpec spec = {0};
                    spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                    spec.shape.type = PH_AABB2D;
                    spec.shape.aabb2d.c = Vec2Zero();
                    spec.shape.aabb2d.r = bb.r;
                    Assert(*len < cap);
                    specs[(*len)++] = spec;
                }
            }

            // Room 0
            {
                Room room = {0};
                // Camera info
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(100.0f, 499.0f), Vec2Lit(355.0f, 210.0f));
                        room.camera_start = Vec3Lit(bb.c.x, bb.c.y, 1.0f);
                    }
                    room.static_camera = false;
                    room.room_bounds = AABB2DFromAsepriteBB(tx, Vec2Lit(0.0f, 449.0f), Vec2Lit(355.0f, 210.0f));
                }
                {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(163.0f, 408.0f), Vec2Lit(186.0f, 385.0f));
                    room.player_start = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                }

                // Enemies
                {
                    EntitySpec* specs = room.enemies;
                    u32* len = &room.elen;
                    u32 cap = ArraySizeOf(room.enemies);
                    Vec2 positions[][2] = {
                        {Vec2Lit(104.0f, 408.0f), Vec2Lit(135.0f, 377.0f)},
                    };
                    for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                        AABB2D bb =
                            AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        Assert(*len < cap);
                        specs[(*len)++] = spec;
                    }
                }

                // Exits.
                {
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(Vec2Lit(358.0f, 408.0f)),
                                                         AsepritePxTR(Vec2Lit(364.0f, 249.0f)));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.room_idx_to_be = 1;

                        room.next_room_exit_spec = spec;
                    }
                }

                stage.rooms[stage.rlen++] = room;
            }

            // Room 1
            {
                Room room = {0};
                room.static_camera = true;
                {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(325.0f, 499.0f), Vec2Lit(580.0f, 210.0f));
                    room.camera_start = Vec3Lit(bb.c.x, bb.c.y, 1.0f);
                }
                {
                    AABB2D bb = AABB2DFromAsepriteBB(tx, Vec2Lit(421.0f, 408.0f), Vec2Lit(444.0f, 385.0f));
                    room.player_start = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                }

                // Enemies
                {
                    EntitySpec* specs = room.enemies;
                    u32* len = &room.elen;
                    u32 cap = ArraySizeOf(room.enemies);
                    Vec2 positions[][2] = {
                        {Vec2Lit(534.0f, 408.0f), Vec2Lit(565.0f, 377.0f)},
                    };
                    for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
                        AABB2D bb =
                            AABB2DFromAsepriteBB(tx, AsepritePxBL(positions[i][0]), AsepritePxTR(positions[i][1]));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        Assert(*len < cap);
                        specs[(*len)++] = spec;
                    }
                }

                // Exits
                {
                    // Prev
                    {
                        AABB2D bb = AABB2DFromAsepriteBB(tx, AsepritePxBL(Vec2Lit(326.0f, 408.0f)),
                                                         AsepritePxTR(Vec2Lit(331.0f, 249.0f)));
                        EntitySpec spec = {0};
                        spec.pos = Vec3Lit(bb.c.x, bb.c.y, 0.0f);
                        spec.shape.type = PH_AABB2D;
                        spec.shape.aabb2d.c = Vec2Zero();
                        spec.shape.aabb2d.r = bb.r;
                        spec.room_idx_to_be = 0;

                        room.prev_room_exit_spec = spec;
                    }
                }

                stage.rooms[stage.rlen++] = room;
            }

            STAGES[STAGE_ROOM_TEST] = stage;
        }
    }

    return &STAGES[sid];
}

////////////////////////////////////////////////////

#define MAX_SCENES 10

typedef enum {
    SCENE_DEFAULT = 0,
    SCENE_TITLE_SCREEN,
    SCENE_STAGE_SELECT,
    SCENE_STAGE,
    SCENE_TRUE_TYPE,
    SCENE_COUNT,
} SceneId;

typedef struct GameState {
    /////////////////////////////////////////
    // Core fields
    bool debug;
    bool update;
    bool game_over;
    bool init;  // False on first frame of scene load. Used to tell update function to perform init.

    // TODO: Move this to RenderState and move a reference to RSTATE here. Or just get rid of this and have a
    // GetScreen/RenderWidth() function that reads RSTATE.
    u32 renderw;
    u32 renderh;
    u32 screenw;
    u32 screenh;

    bool debug_ever;

    f32 dt;  // frame delta time.
    f32 timer;
    u32 nframe;  // num frame we are on.

    void (*scenefuncs[MAX_SCENES])(struct GameState*);
    u32 scenefuncssize;
    SceneId sceneid;
    SceneId nextsceneid;

    /////////////////////////////////////////
    // Scene fields
    Stage* stage;
    u32 room_idx;  // Current room idx;
    u32 room_idx_to_be;

    EntityId loadable_entities[100];
    u32 lelen;
    u32 lecap;
} GameState;

static GameState GState;

#define GetGameState() (&GState)

typedef void (*SceneTickFunc)(GameState* gstate);

// Resets global state/system state that contains per scene info. Experimental approach
// for use in scene switching as opposed to putting all this state into GameState object
// and passing that around. The globalness of these systems is kind of convenient at the
// moment.
static void GameStateLoadScene(GameState* gstate, SceneId sceneid) {
    Assert(sceneid < MAX_SCENES);

    // TODO: Big todo here. Need to come up with a more elegant way to load/unload scene data.
    // Luckily for megaman I don't think there will be entities that persist across scenes.
    EntitySystemInit();
    RenderDataInit();
    LoadSounds();

    // Signal to scene update func to run init.
    gstate->init = false;
    gstate->update = true;
    gstate->debug = false;
    gstate->debug_ever = false;
    gstate->timer = 0.0f;
    gstate->nframe = 0;

    gstate->sceneid = sceneid;
    gstate->nextsceneid = gstate->sceneid;
}

typedef enum {
    SS_NONE = 0,
    SS_RESPAWN,
    SS_SPAWNED,
    SS_PLAYING,
    SS_GAMEOVER,
    SS_DYING,
    SS_COUNT,
} SceneState;

static const Vec2 GRIDR = {.x = GRID_WIDTH, .y = GRID_HEIGHT};

typedef enum {
    PS_NONE = 0,
    PS_IDLE,
    PS_CLIMBING,
    PS_JUMPING,
    PS_DYING,
    PS_WALKING,
    PS_HURT,
    PS_COUNT,
} PlayerState;

static const i32 KUP = KEY_UP;
static const i32 KDOWN = KEY_DOWN;
static const i32 KLEFT = KEY_LEFT;
static const i32 KRIGHT = KEY_RIGHT;
static const i32 KJUMP = KEY_Z;
static const i32 KSHOOT = KEY_X;

static void SceneDefaultTick(GameState* gstate) {
    BeginRender();

    ShaderPass* pass = BeginShaderPass(GetShader());
    u32 width = gstate->renderw;
    u32 height = gstate->renderh;
    pass->proj = Mat4Ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);

    char msg[] = "DEFAULT SCENE";
    u32 size = strlen(msg);

    LFont font = GetLFont(LFONT_DEFAULT);
    Vec2 dim = LFontMeasureText(font, msg, size);

    // Center text by shifting center forward by half a text cell size and then shifting backward
    // by half the text size.
    Vec2 pos = Vec2Lit(0.5f * (width - dim.x), 0.70f * height);
    DrawLTextEx(font, msg, size, pos, LYELLOW);

    EndShaderPass();

    EndRender();
}

LCORE void TrueTypeTickRaw(GameState* gstate) {
    static stbtt_bakedchar CDATA[96];  // ASCII 32..126 is 95 glyphs
    static LTexture TX = {0};

    if (!gstate->init) {
        byte ttf_buffer[1 << 20];
        byte bitmap_alpha[512 * 512];
        fread(ttf_buffer, 1, 1 << 20, fopen("c:/windows/fonts/times.ttf", "rb"));
        stbtt_BakeFontBitmap(ttf_buffer, 0, 32.0, bitmap_alpha, 512, 512, 32, 96, CDATA);

        // Output alpha bitmap to pgm file (greyscal file). viewable in GIMP for debugging.
        if (false) {
            FILE* out = fopen("C:/Users/rvbea/scratch/font_dump.pgm", "wb");
            if (out) {
                fprintf(out, "P5\n%d %d\n255\n", 512, 512);
                fwrite(bitmap_alpha, 1, 512 * 512, out);
                fclose(out);
            } else {
                perror("Failed to open font_dump.pgm for writing");
            }
        }

        // The baked bitmap assumes origin in top left with +y pointing down. Flip the engine expects
        // origin in bottom left with +y going up.
        for (i32 i = 0; i < (512 * 512) / 2; ++i) {
            i32 col = i % 512;
            i32 top_row = i / 512;
            i32 bottom_row = 512 - top_row - 1;
            i32 top_idx = (top_row * 512) + col;
            i32 bottom_idx = (bottom_row * 512) + col;

            byte temp = bitmap_alpha[top_idx];
            bitmap_alpha[top_idx] = bitmap_alpha[bottom_idx];
            bitmap_alpha[bottom_idx] = temp;
        }

        if (false) {
            FILE* out = fopen("C:/Users/rvbea/scratch/font_dump_flipped.pgm", "wb");
            if (out) {
                fprintf(out, "P5\n%d %d\n255\n", 512, 512);
                fwrite(bitmap_alpha, 1, 512 * 512, out);
                fclose(out);
            } else {
                perror("Failed to open font_dump.pgm for writing");
            }
        }

        // The baked bitmap contains only alpha channel information. Convert to rgba so that we can draw
        // this with our normal shader. yes this is a bit wasteful.
        byte* bitmap = calloc(512 * 512 * 4, sizeof(byte));
        for (i32 i = 0; i < (512 * 512); ++i) {
            bitmap[(i * 4) + 0] = 255;
            bitmap[(i * 4) + 1] = 255;
            bitmap[(i * 4) + 2] = 255;
            bitmap[(i * 4) + 3] = bitmap_alpha[i];
        }

        TX.width = 512;
        TX.height = 512;
        glGenTextures(1, &TX.id);
        glBindTexture(GL_TEXTURE_2D, TX.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TX.width, TX.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        free(bitmap);
    }

    BeginRender();

    ShaderPass* pass = BeginShaderPass(GetShader());

    ClearBuffer(LBLUE);

    u32 width = gstate->renderw;
    u32 height = gstate->renderh;
    pass->proj = Mat4Ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);

    // DrawLTexture(TX, Vec2Lit(0.5f * width, 0.3f * height), 0.5f);

    char text[] = "Goodnight, my love";
    Vec2 pos = Vec2Lit(100.0f, 150.0f);

    char* s = text;
    i32 len = strlen(s);
    for (i32 i = 0; i < len; ++i) {
        // Vec2 pos = Vec2Lit(0.5f * width, 0.5f * height);
        stbtt_aligned_quad quad = {0};
        // The y position passed in here is the baseline of the glpyhs (i.e. the common line they will sit on)
        stbtt_GetBakedQuad(CDATA, TX.width, TX.height, s[i] - 32, &pos.x, &pos.y, &quad, true);

        /*
            Remember. the key to understanding all of these transformations is to visualize them from the perspective
            of the originating coordinate system (stb_truetype's coordinate system). If you try to visualize
            them from the perspective of the final coordinate system, it will be hard to do.
        */

        Vec2 qdims = Vec2Sub(Vec2Lit(quad.x1, quad.y1), Vec2Lit(quad.x0, quad.y0));
        // Note we don't need to flip anything about the quads world space positions even though stb_truetype assumes
        // origin top left and +Y is down. The rendering position data will work.
        Vec2 qorigin = {0};
        qorigin.x = quad.x0 + (0.5f * qdims.x);
        // We need to flip the y position coords since stb_truetype assumes that positioning is relative to a
        // baseline where the origin is top left and +y is down. Our pos.y is our baseline, so to reflect
        // we take the height relative to the baseline and subtract that from the baseline. Think of this as if
        // we were in the coordinate space of stb_truetype. we have a baseline pos.y and some position quad.y1
        // which is the highest y position of the glyph. We need to subtract the height delta from the baseline
        // to reflect in stb_truetype coordinate system which will result in upright positions in lcore engine.
        f32 height_relative_to_baseline = quad.y1 - pos.y;
        qorigin.y = (pos.y - height_relative_to_baseline) + (0.5f * qdims.y);

        // Don't need to modify anything to calculate the dims since dimensions are not in reference of a particular
        // coordinate system.
        Vec2 quvdims = Vec2Sub(Vec2Lit(quad.s1, quad.t1), Vec2Lit(quad.s0, quad.t0));
        Vec2 quvoffset = {0};
        quvoffset.x = quad.s0;
        // We do need to flip the y text coords because we have flipped the texture after baking.
        // 1 - t0 will map the top left of the sub rect to the new top left of the flipped texture.
        // By subtracting the height we then get the bottom left corner which is what the engine expects.
        quvoffset.y = 1.0f - quad.t0 - quvdims.y;

        if (gstate->debug) {
            DrawQuad(Vec2ToVec3(qorigin), qdims.x, qdims.y, Mat4Identity(), LRED);
        }
        DrawLTexturePro2(TX.id, quvoffset, quvdims, qorigin, qdims, 0.0f, LCLEAR);
    }
    EndShaderPass();

    EndRender();
}

static void TrueTypeTick(GameState* gstate) {
    if (!gstate->init) {
    }

    BeginRender();

    ShaderPass* pass = BeginShaderPass(GetShader());

    ClearBuffer(LBLUE);

    u32 width = gstate->renderw;
    u32 height = gstate->renderh;
    pass->proj = Mat4Ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);

    LFont font = GetLFont(LFONT_DEFAULT);

    char text[] = "The quick brown fox";
    i32 len = strlen(text);

    Vec2 dims = LFontMeasureText(font, text, len);
    Vec2 pos = Vec2Lit(0.5f * (gstate->renderw - dims.x), 0.5f * (gstate->renderh - dims.y));

    DrawLTextEx(font, text, len, pos, LRED);

    EndShaderPass();

    EndRender();
}

static void TitleScreenTick(GameState* gstate) {
    if (!gstate->init) {
    }

    if (IsKeyPressed(KEY_ENTER)) {
        gstate->nextsceneid = SCENE_STAGE_SELECT;
    }

    BeginRender();

    ShaderPass* pass = BeginShaderPass(GetShader());

    ClearBuffer(LBLACK);

    u32 width = gstate->renderw;
    u32 height = gstate->renderh;
    pass->proj = Mat4Ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);

    char msg[] = "Press ENTER To Start";
    u32 size = strlen(msg);

    LFont font = GetLFont(LFONT_SMALL);
    Vec2 dim = LFontMeasureText(font, msg, size);

    // Center text by shifting center forward by half a text cell size and then shifting backward
    // by half the text size.
    Vec2 pos = Vec2Lit(0.5f * (width - dim.x), 0.70f * height);
    DrawLTextEx(font, msg, size, pos, LYELLOW);

    EndShaderPass();

    EndRender();
}

static void StageSelectTick(GameState* gstate) {
    if (IsKeyPressed(KEY_ENTER)) {
        gstate->nextsceneid = SCENE_STAGE;
    }

    BeginRender();

    ShaderPass* pass = BeginShaderPass(GetShader());

    ClearBuffer(LRED);

    u32 width = gstate->renderw;
    u32 height = gstate->renderh;
    pass->proj = Mat4Ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);

    char msg[] = "Stage Select";
    u32 size = strlen(msg);

    LFont font = GetLFont(LFONT_DEFAULT);
    Vec2 dim = LFontMeasureText(font, msg, size);

    // Center text by shifting center forward by half a text cell size and then shifting backward
    // by half the text size.
    Vec2 pos = Vec2Lit(0.5f * (width - dim.x), 0.70f * height);
    DrawLText(msg, size, pos, LYELLOW);

    EndShaderPass();

    EndRender();
}

static void UnloadRoomEntities(GameState* gstate) {
    TraceLog(LOG_INFO, "Unloading Room");

    for (u32 i = 0; i < gstate->lelen; ++i) {
        EntityId id = gstate->loadable_entities[i];
        Entity* e = GetEntity(id);
        Assert(e != NULL);
        FlagOff(e->flags, EFLAG_ACTIVE);
    }

    for (u32 i = 0; i < gstate->lecap; ++i) {
        gstate->loadable_entities[i] = NULLID;
    }
    gstate->lelen = 0;
}

static const AnimationAseRect MOB_BUNNY = {
    .start = {.lc = Vec2Lit(20.0f, 79.0f), .r = Vec2Lit(32.0f, 40.0f)},
    .step = Vec2Lit(33.0f, 0.0f),
    .nframes = 3,
};

static const AseRect MOB_BAT_IDLE = {
    .lc = Vec2Lit(149.0f, 73.0f),
    .r = Vec2Lit(16.0f, 24.0f),
};
static const AnimationAseRect MOB_BAT_WAKEUP = {
    .start = {.lc = Vec2Lit(166.0f, 73.0f), .r = Vec2Lit(32.0f, 24.0f)},
    .step = Vec2Lit(33.0f, 0.0f),
    .nframes = 3,
};
static const AnimationAseRect MOB_BAT_AWAKE = {
    .start = {.lc = Vec2Lit(265.0f, 73.0f), .r = Vec2Lit(32.0f, 24.0f)},
    .step = Vec2Lit(33.0f, 0.0f),
    .nframes = 3,
};

static const AnimationAseRect MOB_DOG_IDLE = {
    .start = {.lc = Vec2Lit(122.0f, 471.0f), .r = Vec2Lit(72.0f, 62.0f)},
    .step = Vec2Lit(73.0f, 0.0f),
    .nframes = 6,
};

static const AnimationAseRect MOB_DOG_ATTACK = {
    .start = {.lc = Vec2Lit(560.0f, 471.0f), .r = Vec2Lit(72.0f, 62.0f)},
    .step = Vec2Lit(73.0f, 0.0f),
    .nframes = 2,
};

static const AnimationAseRect MOB_DOG_FIREBALL = {
    .start = {.lc = Vec2Lit(18.0f, 112.0f), .r = Vec2Lit(10.0f, 10.0f)},
    .step = Vec2Lit(11.0f, 0.0f),
    .nframes = 3,
};

static const AnimationAseRect MOB_BIRD_FLYING = {
    .start = {.lc = Vec2Lit(83.0f, 100.0f), .r = Vec2Lit(18.0f, 16.0f)},
    .step = Vec2Lit(19.0f, 0.0f),
    .nframes = 2,
};

static const AseRect MOB_EGG = {.lc = Vec2Lit(84.0f, 114.0f), .r = Vec2Lit(13.0f, 10.0f)};
static const AseRect MOB_EGG_BROKEN = {.lc = Vec2Lit(104.0f, 117.0f), .r = Vec2Lit(17.0f, 15.0f)};

static const AnimationAseRect MOB_CHICK_FLYING = {
    .start = {.lc = Vec2Lit(106.0f, 126.0f), .r = Vec2Lit(8.0f, 8.0f)},
    .step = Vec2Lit(9.0f, 0.0f),
    .nframes = 2,
};

static void LoadRoomEntities(GameState* gstate, Room* room) {
    Assert(gstate->lelen == 0);

    // Enemies.
    {
        EntitySpec* specs = room->enemies;
        u32 len = room->elen;
        for (u32 i = 0; i < len; ++i) {
            EntitySpec* spec = &specs[i];

            Entity* e = AllocateEntity();
            e->pos = spec->pos;
            e->shape = spec->shape;
            GraphicsInfoSetShape(&e->graphics, GetMesh(M_QUAD), Vec3Scale(Vec2ToVec3(spec->shape.aabb2d.r), 2.0f),
                                 LRED);
            FlagOn(e->flags, EFLAG_MOB);

            e->mobtype = spec->mobtype;
            switch (e->mobtype) {
                case MT_RABBIT: {
                    GraphicsInfoSetAnimationFromAseprite(&e->graphics, GetTexture(TX_MEGAMAN), MOB_BUNNY, 0.0f, 1.0f);
                    break;
                }
                case MT_BAT: {
                    GraphicsInfoSetSpriteFromAseprite(&e->graphics, GetTexture(TX_MEGAMAN), MOB_BAT_IDLE, 0.0f, 1.0f);
                    break;
                }
                case MT_DOG: {
                    GraphicsInfoSetAnimationFromAseprite(&e->graphics, GetTexture(TX_MEGAMAN), MOB_DOG_IDLE, 0.0f,
                                                         1.0f);
                    e->facing_left = true;
                    e->graphics.pos_offset = Vec3Lit(0.0f, 13.0f, 0.0f);
                    break;
                }
                default: {
                    TraceLog(LOG_ERROR, "Unknown mob type %d", e->mobtype);
                    exit(1);
                    return;
                }
            }

            e->health = 2;

            Assert(gstate->lelen < gstate->lecap);
            gstate->loadable_entities[gstate->lelen++] = e->id;
        }
    }

    // Spawners.
    {
        EntitySpec* specs = room->spawners;
        u32 len = room->spawnerslen;
        for (u32 i = 0; i < len; ++i) {
            EntitySpec* spec = &specs[i];

            Entity* e = AllocateEntity();
            e->pos = spec->pos;
            e->shape = spec->shape;
            GraphicsInfoSetShape(&e->graphics, GetMesh(M_QUAD), Vec3Lit(4.0f, 4.0f, 0.0f), LRED);
            FlagOn(e->flags, EFLAG_SPAWNER);

            Assert(gstate->lelen < gstate->lecap);
            gstate->loadable_entities[gstate->lelen++] = e->id;
        }
    }

    // Exits.
    {
        {
            EntitySpec* spec = &room->next_room_exit_spec;
            Entity* e = AllocateEntity();
            e->pos = spec->pos;
            e->shape = spec->shape;
            // GraphicsInfoSetShape(&e->graphics, GetMesh(M_QUAD),
            // Vec3Scale(Vec2ToVec3(spec->shape.aabb2d.r), 2.0f),
            //                      LGREEN);
            FlagOn(e->flags, EFLAG_EXIT);

            e->room_link = spec->room_idx_to_be;

            Assert(gstate->lelen < gstate->lecap);
            gstate->loadable_entities[gstate->lelen++] = e->id;
        }

        {
            EntitySpec* spec = &room->prev_room_exit_spec;
            Entity* e = AllocateEntity();
            e->pos = spec->pos;
            e->shape = spec->shape;
            // GraphicsInfoSetShape(&e->graphics, GetMesh(M_QUAD),
            // Vec3Scale(Vec2ToVec3(spec->shape.aabb2d.r), 2.0f),
            //                      LRED);
            FlagOn(e->flags, EFLAG_EXIT);

            e->room_link = spec->room_idx_to_be;

            Assert(gstate->lelen < gstate->lecap);
            gstate->loadable_entities[gstate->lelen++] = e->id;
        }
    }
}

static void GraphicsInfoSetPlayerVisuals(GraphicsInfo* graphics, PlayerState pstate, bool shooting) {
    static const AseRect M_IDLE = {.lc = Vec2Lit(1.0f, 466.0f), .r = Vec2Lit(24.0f, 24.0f)};
    static const AseRect M_JUMP = {.lc = Vec2Lit(1.0f, 409.0f), .r = Vec2Lit(32.0f, 32.0f)};
    static const AnimationAseRect M_WALK_CYCLE = {
        .start = {.lc = Vec2Lit(1.0f, 281.0f), .r = Vec2Lit(24.0f, 24.0f)},
        .step = Vec2Lit(25.0f, 0.0f),
        .nframes = 3,
    };
    static const AnimationAseRect M_CLIMB_CYCLE = {
        .start = {.lc = Vec2Lit(1.0f, 167.0f), .r = Vec2Lit(16.0f, 29.0f)},
        .step = Vec2Lit(17.0f, 0.0f),
        .nframes = 2,
    };
    static const AseRect M_HURT = {.lc = Vec2Lit(140.0f, 279.0f), .r = Vec2Lit(32.0f, 32.0f)};

    // Shooting related visuals.
    static const AseRect M_SHOOT_IDLE = {.lc = Vec2Lit(102.0f, 353.0f), .r = Vec2Lit(31.0f, 24.0f)};
    static const AnimationAseRect M_SHOOT_WALK_CYCLE = {
        .start = {.lc = Vec2Lit(131.0f, 386.0f), .r = Vec2Lit(32.0f, 24.0f)},
        .step = Vec2Lit(33.0f, 0.0f),
        .nframes = 3,
    };
    static const AseRect M_SHOOT_JUMP = {.lc = Vec2Lit(160.0f, 353.0f), .r = Vec2Lit(32.0f, 32.0f)};
    static const AseRect M_SHOOT_CLIMB = {.lc = Vec2Lit(57.0f, 181.0f), .r = Vec2Lit(24.0f, 32.0f)};

    // TODO: Hacky. I should be setting this properly on the graphics info.
    graphics->pos_offset = Vec3Zero();

    LTexture tx = GetTexture(TX_MEGAMAN);
    switch (pstate) {
        case PS_IDLE: {
            if (shooting) {
                GraphicsInfoSetSpriteFromAseprite(graphics, tx, M_SHOOT_IDLE, 0.0f, 1.0f);
            } else {
                GraphicsInfoSetSpriteFromAseprite(graphics, tx, M_IDLE, 0.0f, 1.0f);
            }
            break;
        }
        case PS_CLIMBING: {
            if (shooting) {
                GraphicsInfoSetSpriteFromAseprite(graphics, tx, M_SHOOT_CLIMB, 0.0f, 1.0f);
                graphics->pos_offset = Vec3Lit(4.0f, 0.0f, 0.0f);
            } else {
                GraphicsInfoSetAnimationFromAseprite(graphics, tx, M_CLIMB_CYCLE, 0.0f, 1.0f);
            }
            break;
        }
        case PS_WALKING: {
            if (shooting) {
                GraphicsInfoSetAnimationFromAseprite(graphics, tx, M_SHOOT_WALK_CYCLE, 0.0f, 1.0f);
            } else {
                GraphicsInfoSetAnimationFromAseprite(graphics, tx, M_WALK_CYCLE, 0.0f, 1.0f);
            }
            break;
        }
        case PS_JUMPING: {
            if (shooting) {
                GraphicsInfoSetSpriteFromAseprite(graphics, tx, M_SHOOT_JUMP, 0.0f, 1.0f);
            } else {
                GraphicsInfoSetSpriteFromAseprite(graphics, tx, M_JUMP, 0.0f, 1.0f);
            }
            break;
        }
        case PS_HURT: {
            GraphicsInfoSetSpriteFromAseprite(graphics, tx, M_HURT, 0.0f, 1.0f);
            graphics->pos_offset = Vec3Lit(0.0f, -3.0f, 0.0f);
            break;
        }
        default: {
            TraceLog(LOG_ERROR, "Unknown player state: %d", pstate);
            exit(1);
        }
    }
}

// Returns reference to first entity with :flag that is hit by the 2x2 bb.
static Entity* EntityProbeWorld(Entity* e, Vec2 c, EntityFlag flag) {
    f32 probe_width = 2.0f;
    f32 probe_height = 2.0f;

    Polyhedron shape = {0};
    shape.type = PH_AABB2D;
    shape.aabb2d.c = c;
    shape.aabb2d.r = Vec2Lit(0.5f * probe_width, 0.5f * probe_height);

    shape = PolyhedronTransform(shape, Vec3Lit(1.0f, 1.0f, 1.0f), Mat4Identity(), e->pos);

    Entity* hit = NULL;
    for (u32 j = 0; j < GetNumEntities(); ++j) {
        Entity* e2 = GetEntity(j);
        if (e->id == e2->id) {
            continue;
        }
        if (!TestFlag(e2->flags, EFLAG_ACTIVE) || !TestFlag(e2->flags, flag) || e2->shape.type == PH_NONE) {
            continue;
        }

        Polyhedron shape2 = PolyhedronTransform(e2->shape, e2->scale, e2->orient, e2->pos);

        CollisionInfo info = {0};
        bool collides = TestPolyhedronVsPolyhedron(&shape, &shape2, &info.normal, &info.depth);
        if (collides) {
            hit = e2;
            break;
        }
    }

    DrawQuad(Vec3Lit(shape.aabb2d.c.x, shape.aabb2d.c.y, e->pos.z + 1), probe_width, probe_height, Mat4Identity(),
             hit != NULL ? LGREEN : LGRAY);
    return hit;
}

static void StageTick(GameState* gstate) {
    static LCamera CAMERA = {0};
    static bool SPAWNED = false;
    static PlayerState PSTATE = PS_NONE;
    static float CAMERA_TRANSITION_DT = 0.0f;
    static bool CAMERA_TRANSITIONING = false;
    static Vec3 CAMERA_EYE_START = {0};
    static const f32 GRAVITY = 350.0f;

    BeginRender();
    ShaderPass* pass = BeginShaderPass(GetShader());
    pass->view = GetCameraMat(CAMERA.eye, CAMERA.orient);

    u32 width = gstate->renderw;
    u32 height = gstate->renderh;
    f32 hwidth = 0.5f * width;
    f32 hheight = 0.5f * height;
    pass->proj = Mat4Ortho(-hwidth, hwidth, -hheight, hheight, -1.0f, 1.0f);

    f32 dt = gstate->dt;

    if (!gstate->init) {
        SPAWNED = false;

        u32 room_idx = 7;
        gstate->stage = GetStage(STAGE_WOODMAN);
        gstate->room_idx = room_idx;
        gstate->room_idx_to_be = room_idx;

        // walls.
        {
            EntitySpec* specs = gstate->stage->walls;
            u32 len = gstate->stage->wlen;
            for (u32 i = 0; i < len; ++i) {
                EntitySpec* spec = &specs[i];

                Entity* e = AllocateEntity();
                e->pos = spec->pos;
                e->shape = spec->shape;
                FlagOn(e->flags, EFLAG_WALL);
            }
        }

        // Ladders.
        {
            EntitySpec* specs = gstate->stage->ladders;
            u32 len = gstate->stage->llen;
            for (u32 i = 0; i < len; ++i) {
                EntitySpec* spec = &specs[i];

                Entity* e = AllocateEntity();
                e->pos = spec->pos;
                e->shape = spec->shape;
                FlagOff(e->flags, EFLAG_DRAW);
                FlagOn(e->flags, EFLAG_CLIMABLE);
            }
        }

        Room* room = &gstate->stage->rooms[gstate->room_idx];
        CAMERA.eye = room->camera_start;
        CAMERA.orient = Vec3Zero();
        LoadRoomEntities(gstate, room);
    }

    if (gstate->room_idx_to_be != gstate->room_idx) {
        TraceLog(LOG_INFO, "Loaded Room %d", gstate->room_idx_to_be);

        // Setup camera transition.
        CAMERA_TRANSITIONING = true;
        CAMERA_TRANSITION_DT += 0.0f;
        CAMERA_EYE_START = CAMERA.eye;

        gstate->room_idx = gstate->room_idx_to_be;

        UnloadRoomEntities(gstate);
    }

    Room* room = &gstate->stage->rooms[gstate->room_idx];

    Entity* player = NULL;
    for (u32 i = 0; i < GetNumEntities(); ++i) {
        Entity* e = GetEntity(i);
        if (!TestFlag(e->flags, EFLAG_ACTIVE)) {
            continue;
        }

        if (TestFlag(e->flags, EFLAG_PLAYER_CONTROLLED)) {
            player = e;
            break;
        }
    }

    if (!CAMERA_TRANSITIONING) {
        if (gstate->update) {
            if (!SPAWNED) {
                SPAWNED = true;
                Entity* e = AllocateEntity();
                e->pos = room->player_start;

                e->shape.type = PH_AABB2D;
                e->shape.aabb2d.c = Vec2Zero();
                e->shape.aabb2d.r = Vec2Lit(0.5f * 24.0f, 0.5f * 24.0f);

                PSTATE = PS_IDLE;
                GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, false);
                FlagOn(e->flags, EFLAG_PLAYER_CONTROLLED);

                e->health = 4;
            }

            // Update.
            for (u32 i = 0; i < GetNumEntities(); ++i) {
                Entity* e = GetEntity(i);
                if (!TestFlag(e->flags, EFLAG_ACTIVE)) {
                    continue;
                }

                e->active_dt += dt;

                e->ppos = e->pos;

                if (TestFlag(e->flags, EFLAG_PLAYER_CONTROLLED)) {
                    static const f32 WALK_DX_SPEED = 50.0f;
                    static const f32 WALK_DY_SPEED = 200.0f;
                    static f32 SHOOT_COOLDOWN = 0.0f;
                    static bool SHOOTING = false;
                    static f32 HITSTUN_COOLDOWN_DT = 0.0f;

                    HITSTUN_COOLDOWN_DT += dt;
                    if (e->attacker_id != NULLID && PSTATE != PS_HURT) {
                        PSTATE = PS_HURT;
                        HITSTUN_COOLDOWN_DT = 0.0f;
                        GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                        --e->health;
                        // Get x direction to move. if attack is perp and this == 0, then just pick a direciton.
                        f32 dir = Vec3Dot(e->attack_dir, Vec3Lit(1.0f, 0.0f, 0.0f));
                        Vec3 v = Vec3Zero();
                        v.x = (dir >= 0.0f ? 1.0f : -1.0f) * 0.5f * WALK_DX_SPEED;
                        e->v = v;
                    }
                    e->attacker_id = NULLID;
                    e->attack_dir = Vec3Zero();

                    if (e->health <= 0) {
                        PSTATE = PS_DYING;
                    }

                    // ground detection.
                    Entity* leftground = EntityProbeWorld(e, Vec2Lit(-6.0f, -12.0f), EFLAG_WALL);
                    Entity* middleground = EntityProbeWorld(e, Vec2Lit(0.0f, -12.0f), EFLAG_WALL);
                    Entity* rightground = EntityProbeWorld(e, Vec2Lit(6.0f, -12.0f), EFLAG_WALL);
                    bool no_ground = leftground == NULL && middleground == NULL && rightground == NULL;

                    // Ladder detection.
                    Entity* up_ladder = NULL;
                    {
                        Polyhedron shape = {0};
                        shape.type = PH_AABB2D;
                        shape.aabb2d.c = Vec2Zero();
                        shape.aabb2d.r = Vec2Lit(0.5f * 5.0f, 0.5f * 5.0f);

                        shape = PolyhedronTransform(shape, Vec3Lit(1.0f, 1.0f, 1.0f), Mat4Identity(), e->pos);
                        for (u32 j = 0; j < GetNumEntities(); ++j) {
                            Entity* e2 = GetEntity(j);
                            if (!TestFlag(e2->flags, EFLAG_ACTIVE) || !TestFlag(e2->flags, EFLAG_CLIMABLE) ||
                                e2->shape.type == PH_NONE) {
                                continue;
                            }

                            Polyhedron shape2 = PolyhedronTransform(e2->shape, e2->scale, e2->orient, e2->pos);

                            CollisionInfo info = {0};
                            bool collides = TestPolyhedronVsPolyhedron(&shape, &shape2, &info.normal, &info.depth);
                            if (collides) {
                                up_ladder = e2;
                                break;
                            }
                        }

                        DrawQuad(Vec3Lit(shape.aabb2d.c.x, shape.aabb2d.c.y, e->pos.z + 1), 5.0f, 5.0f, Mat4Identity(),
                                 up_ladder != NULL ? LGREEN : LGRAY);
                    }

                    Entity* down_ladder = NULL;
                    {
                        Polyhedron shape = {0};
                        shape.type = PH_AABB2D;
                        shape.aabb2d.c = Vec2Lit(0.0f, -12.0f + 0.5f * 5.0f);
                        shape.aabb2d.r = Vec2Lit(0.5f * 5.0f, 0.5f * 5.0f);

                        shape = PolyhedronTransform(shape, Vec3Lit(1.0f, 1.0f, 1.0f), Mat4Identity(), e->pos);
                        for (u32 j = 0; j < GetNumEntities(); ++j) {
                            Entity* e2 = GetEntity(j);
                            if (!TestFlag(e2->flags, EFLAG_ACTIVE) || !TestFlag(e2->flags, EFLAG_CLIMABLE) ||
                                e2->shape.type == PH_NONE) {
                                continue;
                            }

                            Polyhedron shape2 = PolyhedronTransform(e2->shape, e2->scale, e2->orient, e2->pos);

                            CollisionInfo info = {0};
                            bool collides = TestPolyhedronVsPolyhedron(&shape, &shape2, &info.normal, &info.depth);
                            if (collides) {
                                down_ladder = e2;
                                break;
                            }
                        }

                        DrawQuad(Vec3Lit(shape.aabb2d.c.x, shape.aabb2d.c.y, e->pos.z + 1), 5.0f, 5.0f, Mat4Identity(),
                                 down_ladder != NULL ? LGREEN : LGRAY);
                    }

                    switch (PSTATE) {
                        case PS_IDLE: {
                            // We are falling.
                            if (no_ground) {
                                PSTATE = PS_JUMPING;
                                e->v = Vec3Scale(Vec3Lit(0.0f, -1.0f, 0.0f), GRAVITY * dt);
                                GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                            } else if (up_ladder != NULL && IsKeyDown(KUP)) {
                                e->pos.x = up_ladder->pos.x;  // Teleport to middle of ladder.
                                // Super hacky. we should be more rigorous about this eventually.
                                PolyHedronSetAABB2DFromWH(&e->shape, Vec2Zero(), Vec2Lit(16.0f, 24.0f));

                                PSTATE = PS_CLIMBING;
                                GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                            } else if (down_ladder != NULL && IsKeyDown(KDOWN)) {
                                e->pos.x = down_ladder->pos.x;  // Teleport to middle of ladder.
                                // Super hacky. we should be more rigorous about this eventually.
                                PolyHedronSetAABB2DFromWH(&e->shape, Vec2Zero(), Vec2Lit(16.0f, 24.0f));

                                PSTATE = PS_CLIMBING;
                                GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                            } else {
                                // Movement.
                                Vec3 dx = Vec3Zero();
                                if (IsKeyDown(KLEFT)) {
                                    dx.x -= WALK_DX_SPEED;
                                    e->facing_left = true;
                                } else if (IsKeyDown(KRIGHT)) {
                                    dx.x += WALK_DX_SPEED;
                                    e->facing_left = false;
                                }

                                bool walking = IsKeyDown(KLEFT) || IsKeyDown(KRIGHT);

                                Vec3 dy = Vec3Lit(0.0f, e->v.y, 0.0f);
                                if (IsKeyPressed(KJUMP)) {
                                    PSTATE = PS_JUMPING;
                                    dy.y += WALK_DY_SPEED;
                                    GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                                } else if (walking) {
                                    PSTATE = PS_WALKING;
                                    GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                                }

                                e->v = Vec3Add(Vec3Add(dx, dy), Vec3Scale(Vec3Lit(0.0f, -1.0f, 0.0f), GRAVITY * dt));
                            }
                            break;
                        }
                        case PS_CLIMBING: {
                            static const f32 CLIMB_SPEED = 70.0f;

                            // Freeze climbing actions while shooting.
                            // Make sure we zero out y velocity or else if we are moving on ladder
                            // then shoot we will continue moving.
                            f32 dy = 0.0f;
                            if (!SHOOTING) {
                                if (down_ladder == NULL) {
                                    PSTATE = PS_IDLE;
                                    e->v = Vec3Zero();
                                    GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                                } else if (IsKeyDown(KDOWN) && middleground != NULL) {
                                    PSTATE = PS_IDLE;
                                    GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                                } else if (IsKeyPressed(KJUMP)) {
                                    PSTATE = PS_JUMPING;
                                    // TODO: Having to set velocity here for this state change is messy.
                                    e->v.y = 200.0f;

                                    // Super hacky. we should be more rigorous about this eventually.
                                    PolyHedronSetAABB2DFromWH(&e->shape, Vec2Zero(), Vec2Lit(24.0f, 24.0f));

                                    GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                                } else {
                                    if (IsKeyDown(KLEFT)) {
                                        e->facing_left = true;
                                    } else if (IsKeyDown(KRIGHT)) {
                                        e->facing_left = false;
                                    }

                                    if (IsKeyDown(KUP)) {
                                        dy += CLIMB_SPEED;
                                    } else if (IsKeyDown(KDOWN)) {
                                        dy -= CLIMB_SPEED;
                                    }

                                    // Update animation.
                                    {
                                        bool climbing = IsKeyDown(KUP) || IsKeyDown(KDOWN);
                                        if (climbing) {
                                            e->graphics.txinfo.animation.frame_dt += dt;

                                            if (e->graphics.txinfo.animation.frame_dt >= (8.0f / 60.0f)) {
                                                e->graphics.txinfo.animation.frame_dt = 0.0f;
                                                ++e->graphics.txinfo.animation.frame;
                                                e->graphics.txinfo.animation.frame %=
                                                    e->graphics.txinfo.animation.nframes;
                                            }
                                        }
                                    }
                                }
                            }

                            e->v = Vec3Lit(0.0f, dy, 0.0f);

                            break;
                        }
                        case PS_WALKING: {
                            bool walking = IsKeyDown(KLEFT) || IsKeyDown(KRIGHT);

                            // We are falling.
                            if (no_ground) {
                                PSTATE = PS_JUMPING;
                                e->v = Vec3Scale(Vec3Lit(0.0f, -1.0f, 0.0f), GRAVITY * dt);
                                GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                            } else if (walking) {
                                Vec3 dx = Vec3Zero();
                                if (IsKeyDown(KLEFT)) {
                                    dx.x -= WALK_DX_SPEED;
                                    e->facing_left = true;
                                } else if (IsKeyDown(KRIGHT)) {
                                    dx.x += WALK_DX_SPEED;
                                    e->facing_left = false;
                                }

                                Vec3 dy = Vec3Lit(0.0f, e->v.y, 0.0f);
                                if (IsKeyPressed(KJUMP)) {
                                    PSTATE = PS_JUMPING;
                                    dy.y += WALK_DY_SPEED;
                                    GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                                } else {
                                    e->graphics.txinfo.animation.frame_dt += dt;

                                    // Kind of a hacky ish way to control the walk cycle animation but seems to
                                    // work.
                                    if (e->graphics.txinfo.animation.frame_dt >= (8.0f / 60.0f)) {
                                        static bool FORWARD = true;
                                        e->graphics.txinfo.animation.frame_dt = 0.0f;

                                        if (FORWARD) {
                                            ++e->graphics.txinfo.animation.frame;
                                            if (e->graphics.txinfo.animation.frame >=
                                                e->graphics.txinfo.animation.nframes) {
                                                e->graphics.txinfo.animation.frame -= 2;
                                                FORWARD = false;
                                            }
                                        } else {
                                            if (e->graphics.txinfo.animation.frame == 0) {
                                                e->graphics.txinfo.animation.frame = 1;
                                                FORWARD = true;
                                            } else {
                                                --e->graphics.txinfo.animation.frame;
                                            }
                                        }
                                    }
                                }

                                e->v = Vec3Add(Vec3Add(dx, dy), Vec3Scale(Vec3Lit(0.0f, -1.0f, 0.0f), GRAVITY * dt));
                            } else {
                                PSTATE = PS_IDLE;
                                GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                            }
                            break;
                        }
                        case PS_JUMPING: {
                            // Movement.
                            Vec3 dx = Vec3Zero();
                            if (IsKeyDown(KLEFT)) {
                                dx.x -= WALK_DX_SPEED;
                                e->facing_left = true;
                            } else if (IsKeyDown(KRIGHT)) {
                                dx.x += WALK_DX_SPEED;
                                e->facing_left = false;
                            }

                            Vec3 dy = Vec3Lit(0.0f, e->v.y, 0.0f);
                            // if (IsKeyPressed(KJUMP)) {
                            //     PSTATE = PS_JUMPING;
                            //     dy.y += WALK_DY_SPEED;
                            //     GraphicsInfoSetSpriteFromAseprite(&e->graphics, GetTexture(TX_MEGAMAN), M_JUMP,
                            //     0.0f,
                            //                                       1.0f);
                            // }

                            e->v = Vec3Add(Vec3Add(dx, dy), Vec3Scale(Vec3Lit(0.0f, -1.0f, 0.0f), GRAVITY * dt));

                            bool moving = IsKeyDown(KLEFT) || IsKeyDown(KRIGHT);
                            if (!no_ground) {
                                if (moving) {
                                    PSTATE = PS_WALKING;
                                    GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                                } else {
                                    PSTATE = PS_IDLE;
                                    GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                                }
                            } else if (up_ladder != NULL && IsKeyPressed(KUP)) {
                                e->v = Vec3Zero();
                                e->pos.x = up_ladder->pos.x;  // Teleport to middle of ladder.
                                // Super hacky. we should be more rigorous about this eventually.
                                PolyHedronSetAABB2DFromWH(&e->shape, Vec2Zero(), Vec2Lit(16.0f, 24.0f));

                                PSTATE = PS_CLIMBING;
                                GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                            }

                            break;
                        }
                        case PS_HURT: {
                            e->v.y += -GRAVITY * dt;

                            if (HITSTUN_COOLDOWN_DT >= 0.8f) {
                                PSTATE = PS_IDLE;
                                SHOOTING = false;
                                GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                            }
                            break;
                        }
                        case PS_DYING: {
                            SoftDeactivateEntity(e->id);
                            SPAWNED = false;
                            break;
                        }
                        default: {
                            TraceLog(LOG_ERROR, "Unknown Player State %d", PSTATE);
                            exit(0);
                        }
                    }

                    e->pos = Vec3Add(e->pos, Vec3Scale(e->v, dt));

                    SHOOT_COOLDOWN += dt;
                    if (SHOOT_COOLDOWN >= (20.0f / 60.0f)) {
                        if (SHOOTING) {
                            SHOOTING = false;
                            GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, SHOOTING);
                        }
                    }

                    if (IsKeyPressed(KSHOOT) && SHOOT_COOLDOWN >= (6.0f / 60.0f)) {
                        // If we are shooting, don't update the visuals or else shooting animations will
                        // always restart from frame 0.
                        if (!SHOOTING) {
                            GraphicsInfoSetPlayerVisuals(&e->graphics, PSTATE, true);
                        }
                        SHOOTING = true;
                        SHOOT_COOLDOWN = 0.0f;

                        // Spawn shot.
                        {
                            Entity* shot = AllocateEntity();
                            shot->pos = e->pos;
                            shot->v = Vec3Lit(200.0f * (e->facing_left ? -1.0f : 1.0f), 0.0f, 0.0f);

                            shot->shape.type = PH_AABB2D;
                            shot->shape.aabb2d.c = Vec2Zero();
                            shot->shape.aabb2d.r = Vec2Lit(0.5f * 8.0f, 0.5f * 8.0f);

                            GraphicsInfoSetShape(&shot->graphics, GetMesh(M_QUAD), Vec3Lit(8.0f, 8.0f, 1.0f), LYELLOW);
                            FlagOn(shot->flags, EFLAG_PROJECTILE);
                            shot->attacker_id = e->id;
                        }
                    }
                }

                if (TestFlag(e->flags, EFLAG_MOB)) {
                    // ground detection.
                    Entity* ground = NULL;
                    {
                        Polyhedron shape = {0};
                        shape.type = PH_AABB2D;
                        shape.aabb2d.c = Vec2Lit(0.0f, -(e->shape.aabb2d.r.y + 0.3f * 5.0f));
                        shape.aabb2d.r = Vec2Lit(0.5f * 5.0f, 0.5f * 5.0f);

                        shape = PolyhedronTransform(shape, Vec3Lit(1.0f, 1.0f, 1.0f), Mat4Identity(), e->pos);
                        for (u32 j = 0; j < GetNumEntities(); ++j) {
                            Entity* e2 = GetEntity(j);
                            if (!TestFlag(e2->flags, EFLAG_ACTIVE) || !TestFlag(e2->flags, EFLAG_WALL) ||
                                e2->shape.type == PH_NONE) {
                                continue;
                            }

                            Polyhedron shape2 = PolyhedronTransform(e2->shape, e2->scale, e2->orient, e2->pos);

                            CollisionInfo info = {0};
                            bool collides = TestPolyhedronVsPolyhedron(&shape, &shape2, &info.normal, &info.depth);
                            if (collides) {
                                ground = e2;
                                break;
                            }
                        }

                        DrawQuad(Vec3Lit(shape.aabb2d.c.x, shape.aabb2d.c.y, e->pos.z + 1), 5.0f, 5.0f, Mat4Identity(),
                                 ground != NULL ? LGREEN : LGRAY);
                    }

                    if (e->health <= 0) {
                        SoftDeactivateEntity(e->id);
                    }

                    f32 gravity = GRAVITY;

                    switch (e->mobtype) {
                        case MT_RABBIT: {
                            if (ground != NULL) {
                                e->movement_cooldown_dt += dt;

                                if (e->movement_cooldown_dt >= 2.0f) {
                                    e->movement_cooldown_dt = 0.0f;

                                    if (player != NULL) {
                                        Vec3 dist = Vec3Sub(player->pos, e->pos);
                                        f32 dir = Vec3Dot(dist, Vec3Lit(1.0f, 0.0f, 0.0f));
                                        e->v = Vec3Lit((dir >= 0.0f ? 1.0f : -1.0f) * 100.0f, 250.0f, 0.0f);
                                        e->facing_left = dir < 0.0f;
                                        e->graphics.txinfo.animation.frame = 2;
                                    }
                                } else if (e->movement_cooldown_dt >= 1.8f) {
                                    e->graphics.txinfo.animation.frame = 1;
                                } else {
                                    e->graphics.txinfo.animation.frame = 0;
                                }
                            } else {
                                e->graphics.txinfo.animation.frame = 2;
                            }
                            break;
                        }
                        case MT_BAT: {
                            if (player != NULL) {
                                Vec3 dir = Vec3Sub(player->pos, e->pos);
                                if (e->awake) {
                                    e->v = Vec3Scale(Vec3Normalize(dir), 20.0f);

                                    e->graphics.txinfo.animation.frame_dt += dt;
                                    if (e->graphics.txinfo.animation.frame_dt > 0.12f) {
                                        ++e->graphics.txinfo.animation.frame;
                                        e->graphics.txinfo.animation.frame %= e->graphics.txinfo.animation.nframes;
                                        e->graphics.txinfo.animation.frame_dt = 0.0f;
                                    }
                                } else if (e->wakeup_dt > 0.0f) {
                                    e->wakeup_dt += dt;
                                    if (e->wakeup_dt >= 0.6f) {
                                        e->awake = true;
                                        GraphicsInfoSetAnimationFromAseprite(&e->graphics, GetTexture(TX_MEGAMAN),
                                                                             MOB_BAT_AWAKE, 0.0f, 1.0f);
                                    } else if (e->wakeup_dt >= 0.4f) {
                                        e->graphics.txinfo.animation.frame = 2;
                                    } else if (e->wakeup_dt >= 0.2f) {
                                        e->graphics.txinfo.animation.frame = 1;
                                    }
                                } else if (dir.y <= 0.0f) {
                                    GraphicsInfoSetAnimationFromAseprite(&e->graphics, GetTexture(TX_MEGAMAN),
                                                                         MOB_BAT_WAKEUP, 0.0f, 1.0f);
                                    e->wakeup_dt += dt;
                                }
                            }
                            gravity = 0.0f;
                            break;
                        }
                        case MT_DOG: {
                            if (e->awake) {
                                static const f32 SHOOTING_TIME = 0.7f;
                                e->shooting_dt += dt;
                                if (e->shooting_dt >= SHOOTING_TIME) {
                                    e->shooting_dt = 0.0f;
                                    e->awake = false;
                                    GraphicsInfoSetAnimationFromAseprite(&e->graphics, GetTexture(TX_MEGAMAN),
                                                                         MOB_DOG_IDLE, 0.0f, 1.0f);
                                } else {
                                    static const f32 SHOT_COOLDOWN_DT = 0.12f;
                                    if (e->nshots < 3 && e->shooting_dt >= SHOT_COOLDOWN_DT * e->nshots) {
                                        ++e->nshots;
                                        {
                                            Entity* shot = AllocateEntity();

                                            PolyhedronSetCircle(&shot->shape, Vec2Zero(), 4.0f);
                                            FlagOn(shot->flags, EFLAG_IGNORE_WALL_ON_HIT);
                                            shot->lifetime = 4.0f;
                                            shot->attacker_id = e->id;

                                            // GraphicsInfoSetShape(&shot->graphics, GetMesh(M_ICOSPHERE),
                                            //                      Vec3Lit(5.0f, 5.0f, 1.0f), LRED);
                                            GraphicsInfoSetAnimationFromAseprite(
                                                &shot->graphics, GetTexture(TX_MEGAMAN), MOB_DOG_FIREBALL, 0.0f, 1.0f);
                                            FlagOn(shot->flags, EFLAG_PROJECTILE);
                                            shot->pos_parametric = true;
                                            shot->pos_start = Vec3Add(e->pos, Vec3Lit(-10.0f, 0.0f, 0.0f));
                                        }
                                    }

                                    e->graphics.txinfo.animation.frame_dt += dt;
                                    if (e->graphics.txinfo.animation.frame_dt >=
                                        SHOOTING_TIME / e->graphics.txinfo.animation.nframes) {
                                        ++e->graphics.txinfo.animation.frame;
                                        e->graphics.txinfo.animation.frame_dt = 0.0f;
                                    }
                                }
                            } else {
                                static const f32 IDLE_TIME = 1.5f;
                                e->idle_dt += dt;
                                if (e->idle_dt > IDLE_TIME) {
                                    e->awake = true;
                                    e->idle_dt = 0.0f;
                                    e->nshots = 0;
                                    GraphicsInfoSetAnimationFromAseprite(&e->graphics, GetTexture(TX_MEGAMAN),
                                                                         MOB_DOG_ATTACK, 0.0f, 1.0f);
                                } else {
                                    e->graphics.txinfo.animation.frame_dt += dt;
                                    if (e->graphics.txinfo.animation.frame_dt >=
                                        IDLE_TIME / e->graphics.txinfo.animation.nframes) {
                                        ++e->graphics.txinfo.animation.frame;
                                        e->graphics.txinfo.animation.frame_dt = 0.0f;
                                    }
                                }
                            }
                            break;
                        }
                        case MT_BIRD: {
                            gravity = 0.0f;

                            if (player != NULL) {
                                Vec3 diff = Vec3Sub(player->pos, e->pos);
                                Entity* egg = GetEntity(e->eggid);
                                if (f32abs(diff.x) <= 20.0f && TestFlag(egg->flags, EFLAG_ACTIVE)) {
                                    TraceLog(LOG_INFO, "Egg Awake");
                                    egg->awake = true;
                                    e->eggid = NULLID;
                                }
                            }

                            e->graphics.txinfo.animation.frame_dt += dt;
                            if (e->graphics.txinfo.animation.frame_dt >= 0.1f) {
                                e->graphics.txinfo.animation.frame_dt = 0.0f;
                                ++e->graphics.txinfo.animation.frame;
                                e->graphics.txinfo.animation.frame %= e->graphics.txinfo.animation.nframes;
                            }
                            break;
                        }
                        case MT_EGG: {
                            Entity* bird = GetEntity(e->birdid);
                            if (e->awake) {
                                if (e->broken) {
                                    gravity = 0.0f;
                                    e->v = Vec3Zero();
                                    e->broken_dt += dt;
                                    if (e->broken_dt >= 0.2f) {
                                        SoftDeactivateEntity(e->id);
                                    }
                                } else if (ground != NULL) {
                                    e->broken = true;
                                    GraphicsInfoSetSpriteFromAseprite(&e->graphics, GetTexture(TX_MEGAMAN),
                                                                      MOB_EGG_BROKEN, 0.0f, 1.0f);
                                    PolyhedronInit(&e->shape);

                                    {
                                        Entity* chick = AllocateEntity();
                                        chick->pos = e->pos;

                                        // GraphicsInfoSetShape(&chicks->graphics, GetMesh(M_QUAD), Vec3Lit(6, 6, 1),
                                        //                      LORANGE);
                                        GraphicsInfoSetAnimationFromAseprite(&chick->graphics, GetTexture(TX_MEGAMAN),
                                                                             MOB_CHICK_FLYING, 0.0f, 1.0f);

                                        FlagOn(chick->flags, EFLAG_MOB);
                                        chick->health = 1;
                                        chick->mobtype = MT_CHICK;
                                    }
                                }
                            } else {
                                gravity = 0.0f;

                                // This could potentially be incorrect. Need to support generations in ids.
                                // The egg parent bird could be killed and then a new one spawned.
                                if (TestFlag(bird->flags, EFLAG_ACTIVE)) {
                                    // This will be laggy potentially. I think a better approach is for children
                                    // positions to be given in relative offsets to parent posistions. Then in graphics
                                    // update, we incorporate the parent position in transform to find correct world
                                    // positions. This way parent and child positions will be in sync.
                                    e->pos = Vec3Add(bird->pos, Vec3Lit(1.0f, -12.0f, 0.0f));
                                }
                            }
                            break;
                        }
                        case MT_CHICK: {
                            gravity = 0.0f;

                            e->graphics.txinfo.animation.frame_dt += dt;
                            if (e->graphics.txinfo.animation.frame_dt >= 0.1f) {
                                e->graphics.txinfo.animation.frame_dt = 0.0f;
                                ++e->graphics.txinfo.animation.frame;
                                e->graphics.txinfo.animation.frame %= e->graphics.txinfo.animation.nframes;
                            }
                            break;
                        }
                        default: {
                            TraceLog(LOG_ERROR, "Unknown mob type: %d", e->mobtype);
                            exit(1);
                            return;
                        }
                    }

                    e->v.y += -1.0f * gravity * dt;
                    e->pos = Vec3Add(e->pos, Vec3Scale(e->v, dt));
                }

                if (TestFlag(e->flags, EFLAG_PROJECTILE)) {
                    if (e->pos_parametric) {
                        if (e->active_dt >= e->lifetime) {
                            SoftDeactivateEntity(e->id);
                        } else {
                            e->parametric_dt += dt;
                            Vec3 curve = {0};
                            curve.x = -e->parametric_dt * 150.0f;
                            // A controls acceleration. B controls initial velocity.
                            curve.y = (120.0f * e->parametric_dt * e->parametric_dt) - (100.0f * e->parametric_dt);
                            e->pos = Vec3Add(e->pos_start, curve);
                        }

                        if (e->graphics.txinfo.type == TI_ANIMATION) {
                            e->graphics.txinfo.animation.frame_dt += dt;
                            if (e->graphics.txinfo.animation.frame_dt >= 0.05f) {
                                e->graphics.txinfo.animation.frame_dt = 0.0f;
                                ++e->graphics.txinfo.animation.frame;
                                e->graphics.txinfo.animation.frame %= e->graphics.txinfo.animation.nframes;
                            }
                        }
                    } else {
                        e->pos = Vec3Add(e->pos, Vec3Scale(e->v, dt));
                    }
                }

                if (TestFlag(e->flags, EFLAG_SPAWNER)) {
                    if (player != NULL) {
                        Vec3 diff = Vec3Sub(player->pos, e->pos);
                        Entity* spawned = GetEntity(e->spawnedid);
                        // TraceLog(LOG_INFO, "diff %f", diff.x);

                        bool has_spawned = TestFlag(spawned->flags, EFLAG_ACTIVE) &&
                                           TestFlag(spawned->flags, EFLAG_MOB) && spawned->mobtype == MT_BIRD;
                        if (f32abs(diff.x) <= 612.0f && !has_spawned) {
                            TraceLog(LOG_INFO, "Spawned");
                            spawned = AllocateEntity();
                            spawned->pos = e->pos;

                            GraphicsInfoSetAnimationFromAseprite(&spawned->graphics, GetTexture(TX_MEGAMAN),
                                                                 MOB_BIRD_FLYING, 0.0f, 1.0f);
                            // GraphicsInfoSetShape(&spawned->graphics, GetMesh(M_QUAD), Vec3Lit(6, 6, 1), LBLUE);

                            FlagOn(spawned->flags, EFLAG_MOB);
                            spawned->mobtype = MT_BIRD;
                            spawned->health = 2;
                            spawned->facing_left = true;
                            spawned->v = Vec3Lit(-100.0f, 0.0f, 0.0f);

                            PolyHedronSetAABB2DFromWH(&spawned->shape, Vec2Zero(), Vec2Lit(17.0f, 15.0f));

                            e->spawnedid = spawned->id;

                            // Spawn egg
                            {
                                Entity* egg = AllocateEntity();
                                egg->pos = Vec3Add(spawned->pos, Vec3Lit(0.0f, -8.0f, 0.0f));

                                GraphicsInfoSetSpriteFromAseprite(&egg->graphics, GetTexture(TX_MEGAMAN), MOB_EGG, 0.0f,
                                                                  1.0f);
                                // GraphicsInfoSetShape(&egg->graphics, GetMesh(M_QUAD), Vec3Lit(6, 6, 1), LBLUE);

                                FlagOn(egg->flags, EFLAG_MOB);
                                egg->mobtype = MT_EGG;
                                egg->health = 2;
                                egg->birdid = spawned->id;

                                spawned->eggid = egg->id;
                                // egg->facing_left = true;
                                // egg->v = Vec3Lit(-100.0f, 0.0f, 0.0f);

                                PolyHedronSetAABB2DFromWH(&egg->shape, Vec2Zero(), Vec2Lit(11.0f, 8.0f));
                            }
                        }
                    }
                }
            }

            // Collisions
            {
                CollisionInfo collisions[20];
                u32 clen = 0;
                u32 ccap = ArraySizeOf(collisions);
                for (u32 i = 0; i < GetNumEntities(); ++i) {
                    Entity* ei = GetEntity(i);
                    if (!TestFlag(ei->flags, EFLAG_ACTIVE) || ei->shape.type == PH_NONE) {
                        continue;
                    }

                    Polyhedron pi = PolyhedronTransform(ei->shape, ei->scale, ei->orient, ei->pos);
                    for (u32 j = i + 1; j < GetNumEntities(); ++j) {
                        Entity* ej = GetEntity(j);
                        if (!TestFlag(ej->flags, EFLAG_ACTIVE) || ej->shape.type == PH_NONE) {
                            continue;
                        }
                        if (TestFlag(ei->flags, EFLAG_WALL) && TestFlag(ej->flags, EFLAG_WALL)) {
                            continue;
                        }
                        if ((TestFlag(ei->flags, EFLAG_CLIMABLE) && TestFlag(ej->flags, EFLAG_WALL)) ||
                            (TestFlag(ei->flags, EFLAG_WALL) && TestFlag(ej->flags, EFLAG_CLIMABLE))) {
                            continue;
                        }

                        Polyhedron pj = PolyhedronTransform(ej->shape, ej->scale, ej->orient, ej->pos);

                        CollisionInfo info = {0};
                        info.id0 = ei->id;
                        info.id1 = ej->id;
                        bool collides = TestPolyhedronVsPolyhedron(&pi, &pj, &info.normal, &info.depth);
                        if (collides) {
                            Assert(clen < ccap);
                            collisions[clen++] = info;
                        }
                    }
                }

                for (u32 j = 0; j < clen; ++j) {
                    CollisionInfo* c = &collisions[j];

                    // To handle checking "both sides" of the collision. We just make two collisions
                    // with reversed diAserections.
                    CollisionInfo reverse = *c;
                    reverse.id0 = c->id1;
                    reverse.id1 = c->id0;
                    reverse.normal = Vec3Negate(reverse.normal);

                    CollisionInfo cs[2] = {*c, reverse};
                    for (u32 i = 0; i < 2; ++i) {
                        CollisionInfo* info = &cs[i];
                        Entity* e0 = GetEntity(info->id0);
                        Entity* e1 = GetEntity(info->id1);

                        if (TestFlag(e0->flags, EFLAG_PLAYER_CONTROLLED) && TestFlag(e1->flags, EFLAG_WALL)) {
                            Entity* player = e0;
                            player->pos = Vec3Add(player->pos, Vec3Scale(info->normal, info->depth));

                            // TODO: We don't really check closing velocity here.

                            // Only stop perp to surface velocity if we are traveling towards the surface. this
                            // prevents teleporting down to top of collider if we are hugging a wall and jump.
                            f32 vdir = Vec3Dot(player->v, info->normal);
                            if (vdir < 0.0f) {
                                Vec3 vplayer = player->v;
                                Vec3 vperp = Vec3Scale(info->normal, Vec3Dot(vplayer, info->normal));
                                Vec3 vparallel = Vec3Sub(vplayer, vperp);
                                player->v = vparallel;
                            }
                        }

                        if (TestFlag(e0->flags, EFLAG_MOB) && TestFlag(e1->flags, EFLAG_WALL)) {
                            Entity* mob = e0;
                            mob->pos = Vec3Add(mob->pos, Vec3Scale(info->normal, info->depth));
                            mob->v = Vec3Zero();
                        }

                        if (TestFlag(e0->flags, EFLAG_MOB) && TestFlag(e1->flags, EFLAG_PROJECTILE)) {
                            Entity* mob = e0;
                            Entity* shot = e1;

                            if (shot->attacker_id != mob->id) {
                                --mob->health;
                                SoftDeactivateEntity(shot->id);
                            }
                        }

                        if (TestFlag(e0->flags, EFLAG_PROJECTILE) && TestFlag(e1->flags, EFLAG_WALL)) {
                            Entity* shot = e0;
                            if (!TestFlag(shot->flags, EFLAG_IGNORE_WALL_ON_HIT)) {
                                SoftDeactivateEntity(shot->id);
                            }
                        }

                        if (TestFlag(e0->flags, EFLAG_PLAYER_CONTROLLED) && TestFlag(e1->flags, EFLAG_MOB)) {
                            Entity* player = e0;
                            Entity* attacker = e1;
                            if (player->attacker_id == NULLID) {
                                player->attacker_id = attacker->id;
                                player->attack_dir = info->normal;
                            }
                        }

                        if (TestFlag(e0->flags, EFLAG_PLAYER_CONTROLLED) && TestFlag(e1->flags, EFLAG_PROJECTILE)) {
                            Entity* player = e0;
                            Entity* attacker = e1;
                            if (player->attacker_id == NULLID && attacker->attacker_id != player->id) {
                                player->attacker_id = attacker->id;
                                player->attack_dir = info->normal;
                            }
                        }

                        if (TestFlag(e0->flags, EFLAG_PLAYER_CONTROLLED) && TestFlag(e1->flags, EFLAG_EXIT)) {
                            Entity* exit = e1;
                            gstate->room_idx_to_be = exit->room_link;
                        }
                    }
                }
            }

            // Update camera.
            if (!room->static_camera && player != NULL) {
                // Set camera x to player x but clamp to camera movement bounding box.
                f32 camera_half_width = 0.5f * gstate->renderw;
                f32 camera_left_bound_x = room->room_bounds.c.x - room->room_bounds.r.x + camera_half_width;
                f32 camera_right_bound_x = room->room_bounds.c.x + room->room_bounds.r.x - camera_half_width;
                CAMERA.eye.x = f32clamp(player->pos.x, camera_left_bound_x, camera_right_bound_x);
            }

            // // A bit hacky, but we clamp the camera position to be only integer values. If we don't then its
            // // possible for sprites to get rasterized incorrectly due to subpixel texture values.
            // CAMERA.eye.x = f32floor(CAMERA.eye.x);
            // CAMERA.eye.y = f32floor(CAMERA.eye.y);
        }
    } else {
        // Transitioning camera between rooms
        CAMERA_TRANSITION_DT += dt;
        if (CAMERA_TRANSITION_DT >= 1.0f) {
            CAMERA_TRANSITION_DT = 0.0f;
            CAMERA_TRANSITIONING = false;

            // TODO: Need to think about how to transition from a fixed camera room to a non fixed room.
            // Like if I enter a fixed room and exit the way I came, I can't move the camera position to
            // camera start because its the end of the room. maybe I also need to define a camera end...
            // or as a hacky first start I could just move the camera to the y position of camera_start.
            CAMERA.eye = room->camera_start;

            LoadRoomEntities(gstate, room);
        } else {
            CAMERA.eye = Vec3Lerp(CAMERA_EYE_START, room->camera_start, CAMERA_TRANSITION_DT);
        }
    }

    // Update graphics.
    for (u32 i = 0; i < GetNumEntities(); ++i) {
        Entity* e = GetEntity(i);
        if (!TestFlag(e->flags, EFLAG_ACTIVE)) {
            continue;
        }

        if (TestFlag(e->flags, EFLAG_DRAW)) {
            GraphicsInfo* graphics = &e->graphics;
            Vec3 pos_offset = graphics->pos_offset;
            pos_offset.x *= e->facing_left ? -1.0f : 1.0f;
            graphics->pos = Vec3Add(e->pos, pos_offset);
            graphics->orient = e->orient;

            TextureInfo* txinfo = &graphics->txinfo;
            switch (txinfo->type) {
                case TI_SPRITE: {
                    graphics->uvoffset = txinfo->sprite.minuv;
                    graphics->uvscale = txinfo->sprite.dimsuv;
                    break;
                }
                case TI_ANIMATION: {
                    graphics->uvoffset =
                        Vec2Add(txinfo->animation.minuv, Vec2Scale(txinfo->animation.stepuv, txinfo->animation.frame));
                    graphics->uvscale = txinfo->animation.dimsuv;
                    break;
                }
                default: {
                    TraceLog(LOG_ERROR, "Unknown texture info type: %d", txinfo->type);
                    exit(1);
                }
            }

            if (!e->facing_left) {
                // Flip sprite horizontally. Requires GL_REPEAT wrap mode.
                // Move the bottom left corner to the bottom right corner.
                graphics->uvoffset.x += graphics->uvscale.x;
                // Interpolate across from the bottom right corner to the top left corner.
                graphics->uvscale.x *= -1.0f;
            }
        }
    }

    // Draw entities.
    {
        // Draw background. Assumption is that origin is at center of background texture.
        {
            DrawLTexture(gstate->stage->background, Vec2Zero(), 1.0f);
        }

        for (u32 i = 0; i < GetNumEntities(); ++i) {
            Entity* e = GetEntity(i);
            if (!TestFlag(e->flags, EFLAG_ACTIVE)) {
                continue;
            }

            if (TestFlag(e->flags, EFLAG_DRAW)) {
                DrawCmd* cmd = AddDrawMeshCmd();

                cmd->meshcmd.modelm = Mat4SVRT(e->graphics.scale, e->graphics.orient, e->graphics.pos);
                cmd->meshcmd.color = e->graphics.txinfo.tint;
                cmd->meshcmd.uvoffset = e->graphics.uvoffset;
                cmd->meshcmd.uvscale = e->graphics.uvscale;
                cmd->meshcmd.txid = e->graphics.txinfo.txid;
                cmd->meshcmd.meshid = e->graphics.mid;
            }

            if (gstate->debug && e->shape.type != PH_NONE) {
                // TODO: Wireframe rasterization can lead to looking like the quad is offset. For now
                // Draw filled quads for debugging collisions.
                Vec3 pos = e->pos;
                pos.z += 1;  // give it something high so that it will draw over the entity.
                Polyhedron shape = PolyhedronTransform(e->shape, e->scale, e->orient, pos);
                Vec4 color = LYELLOW;
                DrawPolyhedron(&shape, color);
            }
        }
    }

    EndShaderPass();

    // UI. origin at bottom left hand of screen.
    {
        ShaderPass* pass = BeginShaderPass(GetShader());
        pass->view = GetCameraMat(CAMERA.eye, CAMERA.orient);

        u32 width = gstate->renderw;
        u32 height = gstate->renderh;
        pass->proj = Mat4Ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);

        char msg[] = "Stage 1";
        u32 size = strlen(msg);

        LFont font = GetLFont(LFONT_DEFAULT);
        Vec2 dim = LFontMeasureText(font, msg, size);

        // Center text by shifting center forward by half a text cell size and then shifting backward
        // by half the text size.
        Vec2 pos = Vec2Lit(0.5f * (width - dim.x), 0.70f * height);
        DrawLTextEx(font, msg, size, pos, LYELLOW);

        EndShaderPass();
    }

    EndRender();

    DeactivateEntities();
}

// TODO; Remove
LCORE void Stage2Tick(GameState* gstate) {
    static LCamera CAMERA = {0};

    if (!gstate->init) {
        gstate->stage = GetStage(STAGE_WOODMAN);

        Vec3 origin_offset = Vec3Lit(-0.5f * (gstate->stage->background.width - 1.0f),
                                     -0.5f * (gstate->stage->background.height - 1.0f), 0.0f);

        Vec3 min_aseprite = Vec3Lit(1, 1, 0);
        // Flip y.
        min_aseprite.y = gstate->stage->background.height - 1.0f - min_aseprite.y;
        Vec3 min_world = Vec3Add(min_aseprite, origin_offset);

        Vec3 max_aseprite = Vec3Lit(256, 240, 0);
        // Flip y.
        max_aseprite.y = gstate->stage->background.height - 1.0f - max_aseprite.y;
        Vec3 max_world = Vec3Add(max_aseprite, origin_offset);

        Vec3 mid = Vec3Lerp(max_world, min_world, 0.5f);

        CAMERA.eye = mid;
        CAMERA.orient = Vec3Zero();
    }

    float dx = 1.0f;
    if (IsKeyPressed(KEY_UP)) {
        CAMERA.eye.y += dx;
    } else if (IsKeyPressed(KEY_DOWN)) {
        CAMERA.eye.y -= dx;

    } else if (IsKeyPressed(KEY_LEFT)) {
        CAMERA.eye.x -= dx;

    } else if (IsKeyPressed(KEY_RIGHT)) {
        CAMERA.eye.x += dx;
    }

    BeginRender();

    {
        ShaderPass* pass = BeginShaderPass(GetShader());
        pass->view = GetCameraMat(CAMERA.eye, CAMERA.orient);
        // ClearBuffer(LBLUE);

        u32 width = gstate->renderw;
        u32 height = gstate->renderh;
        f32 hwidth = 0.5f * width;
        f32 hheight = 0.5f * height;
        pass->proj = Mat4Ortho(-hwidth, hwidth, -hheight, hheight, -1.0f, 1.0f);

        // Draw background. Assumption is that origin is at center of background texture.
        {
            DrawLTexture(gstate->stage->background, Vec2Zero(), 1.0f);
        }

        EndShaderPass();
    }

    // UI. origin at bottom left hand of screen.
    {
        ShaderPass* pass = BeginShaderPass(GetShader());
        pass->view = GetCameraMat(CAMERA.eye, CAMERA.orient);

        u32 width = gstate->renderw;
        u32 height = gstate->renderh;
        pass->proj = Mat4Ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);

        char msg[] = "Stage 1";
        u32 size = strlen(msg);

        LFont font = GetLFont(LFONT_DEFAULT);
        Vec2 dim = LFontMeasureText(font, msg, size);

        // Center text by shifting center forward by half a text cell size and then shifting backward
        // by half the text size.
        Vec2 pos = Vec2Lit(0.5f * (width - dim.x), 0.70f * height);
        DrawLTextEx(font, msg, size, pos, LYELLOW);

        EndShaderPass();
    }

    EndRender();
}

////////////////////////////////////////////////////////////

static void GameStateInit(GameState* gstate) {
    memset(gstate, 0, sizeof(GameState));

    gstate->renderw = 256.0f;
    gstate->renderh = 240.0f;

    u32 scale = 3;
    InitWindow(scale * gstate->renderw, scale * gstate->renderh, "Game");
    SetWindowPosition(100, 100);
    // SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetConfigFlags(FLAG_VSYNC_HINT);
    SetTargetFPS(60);
    // DisableCursor();

    InitAudio();
    InitFonts();
    ParticleSystemInit();

    RenderStateInit(gstate->renderw, gstate->renderh);

    {
        // glEnable(GL_DEPTH_TEST);

        // Allow blending to support drawing textures with transparency.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // Register scenes.
    {
        u32 idx = 0;
        gstate->scenefuncs[SCENE_DEFAULT] = SceneDefaultTick;
        gstate->scenefuncs[SCENE_TRUE_TYPE] = TrueTypeTick;
        gstate->scenefuncs[SCENE_TITLE_SCREEN] = TitleScreenTick;
        gstate->scenefuncs[SCENE_STAGE_SELECT] = StageSelectTick;
        gstate->scenefuncs[SCENE_STAGE] = StageTick;
        Assert(SCENE_COUNT < MAX_SCENES);
        gstate->scenefuncssize = idx;
    }
    GameStateLoadScene(gstate, SCENE_TITLE_SCREEN);

    gstate->lecap = ArraySizeOf(gstate->loadable_entities);
    gstate->lelen = 0;
}

static void GameStateTick(GameState* gstate) {
    gstate->dt = GetFrameTime();
    f32 dt = gstate->dt;

    gstate->screenw = GetScreenWidth();
    gstate->screenh = GetScreenHeight();

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
        static bool ENABLE_DEBUG_KEYMAP = true;

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
                TraceLog(LOG_INFO, "Debug %s", gstate->debug ? "On" : "Off");
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

    // Important this happens before the scene tick as we set init=false here and need to make sure the first
    // tick of the scene is with init=false.
    if (gstate->nextsceneid != gstate->sceneid) {
        GameStateLoadScene(gstate, gstate->nextsceneid);
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

    // Test Quat
    {
        // QuatFromAxisAngle
        {
            {
                Quat res = QuatFromAxisAngle(Vec3Lit(1.0f, 0.0f, 0.0f), Deg2Rad(90.0f));
                Quat expected = QuatLit(0.7071068f, 0.0f, 0.0f, 0.7071068f);
                TAssert(QuatEq(res, expected), "QuatFromAxisAngle 90 degrees about x axis");
            }
            {
                Quat res = QuatFromAxisAngle(Vec3Lit(0.0f, 1.0f, 0.0f), Deg2Rad(180.0f));
                Quat expected = QuatLit(0.0f, 1.0f, 0.0f, 0.0f);
                TAssert(QuatEq(res, expected), "QuatFromAxisAngle 180 degrees about y axis");
            }
            {
                Quat res = QuatFromAxisAngle(Vec3Lit(0.57735027f, 0.57735027f, 0.57735027f), Deg2Rad(45.0f));
                Quat expected = QuatLit(0.2214466f, 0.2214466f, 0.2214466f, 0.9238795f);
                TAssert(QuatEq(res, expected), "QuatFromAxisAngle 45 degrees about diagonal axis");
            }
        }

        // QuatLen
        {
            {
                f32 res = QuatLen(QuatLit(1.0f, 2.0f, 3.0f, 4.0f));
                TAssert(f32eq(res, 5.4772256f), "QuatLen");
            }
        }

        // QuatMult
        {
            {
                Quat q1 = QuatLit(0.0f, 0.0f, 0.0f, 1.0f);        // Identity quaternion
                Quat q2 = QuatLit(0.7071f, 0.0f, 0.0f, 0.7071f);  // 90° rotation around X
                Quat res = QuatMult(q1, q2);
                TAssert(QuatEq(res, q2), "QuatMult with identity");
            }
            {
                Quat q1 = QuatLit(0.7071f, 0.0f, 0.0f, 0.7071f);  // 90° rotation around X
                Quat q2 = QuatLit(0.0f, 0.7071f, 0.0f, 0.7071f);  // 90° rotation around Y
                Quat res = QuatMult(q2, q1);
                Quat expected = QuatLit(0.5f, 0.5f, 0.5f, 0.5f);  // 120° rotation around (1,1,1)
                TAssert(QuatEq(res, expected), "QuatMult multiple rotations");
            }
            {
                Quat q1 = QuatLit(0.0f, 0.7071f, 0.0f, 0.7071f);   // 90° rotation around Y
                Quat q2 = QuatLit(0.0f, -0.7071f, 0.0f, 0.7071f);  // Inverse of q1
                Quat res = QuatMult(q2, q1);
                Quat expected = QuatLit(0.0f, 0.0f, 0.0f, 1.0f);  // identity
                TAssert(QuatEq(res, expected), "QuatMult qq^-1 = Identity");
            }
            {
                Quat q1 = QuatLit(0.3f, 0.5f, 0.1f, 0.8f);
                Quat q2 = QuatLit(0.4f, -0.2f, 0.7f, 0.5f);
                Quat res = QuatMult(q2, q1);
                Quat expected = QuatLit(0.84f, -0.08f, 0.35f, 0.31f);
                TAssert(QuatEq(res, expected), "QuatMult non trivial");
            }
        }

        // QuatTransform
        {
            {
                Quat q = QuatLit(0.0f, 0.0f, 0.0f, 1.0f);  // Identity quaternion
                Vec3 v = Vec3Lit(1.0f, 2.0f, 3.0f);
                Vec3 res = QuatTransform(q, v);
                Vec3 expected = Vec3Lit(1.0f, 2.0f, 3.0f);
                TAssert(Vec3Eq(res, expected), "QuatTransform identity");
            }
            {
                Quat q = QuatLit(0.0f, 0.0f, 0.7071f, 0.7071f);  // rotate 90 about z
                Vec3 v = Vec3Lit(1.0f, 0.0f, 0.0f);
                Vec3 res = QuatTransform(q, v);
                Vec3 expected = Vec3Lit(0.0f, 1.0f, 0.0f);
                TAssert(Vec3Eq(res, expected), "QuatTransform 90 deg about z axis");
            }
            {
                Quat q = QuatLit(0.0f, 0.3827f, 0.0f, 0.9239f);  // rotate about y axis by 45 deg
                Vec3 v = Vec3Lit(1.0f, 2.0f, 3.0f);
                Vec3 res = QuatTransform(q, v);
                Vec3 expected = Vec3Lit(2.8284f, 2.0f, 1.4142f);
                TAssert(Vec3Eq(res, expected), "QuatTransform 45 deg about y axis");
            }
        }

        // Quat2Mat4
        {
            {
                Quat q = QuatLit(0.0f, 0.0f, 0.0f, 1.0f);  // Identity quaternion
                Mat4 res = Quat2Mat4(q);
                Mat4 expected = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 1.0f);
                TAssert(Mat4Eq(res, expected), "Quat2Mat4 identity");
            }
            {
                Quat q = QuatLit(1.0f, 0.0f, 0.0f, 0.0f);  // 180 around x
                Mat4 res = Quat2Mat4(q);
                Mat4 expected = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 1.0f);
                TAssert(Mat4Eq(res, expected), "Quat2Mat4 180 around x-axis");
            }
            {
                Quat q = QuatLit(0.2971f, 0.4952f, 0.1981f, 0.7924f);
                Mat4 res = Quat2Mat4(q);
                Mat4 expected = Mat4Lit(0.4312f, -0.0196f, 0.9022f, 0.0f, 0.6084f, 0.7450f, -0.2744f, 0.0f, -0.6666f,
                                        0.6668f, 0.3330f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
                TAssert(Mat4Eq(res, expected), "Quat2Mat4 complicated");
            }
        }
    }
}

static void TestMemory() {
    // NewArena
    {
        {
            Arena a = NewArena(1);
            TAssert(a.beg != NULL, "NewArena(1) beg not null");
            TAssert(a.end != a.beg, "NewArena(1) end != beg");
            TAssert(a.end == a.beg + 1, "NewArena(1) end = beg + 1");
            TAssert(a.end - a.beg == 1, "NewArena(1) end - beg == 1");
        }

        {
            Arena a = NewArena(1234);
            TAssert(a.end == a.beg + 1234, "NewArena(1234) end = beg + 1234");
            TAssert(a.end - a.beg == 1234, "NewArena(1234) end - beg == 1234");
        }
    }

    // New
    {
        {
            Arena a = NewArena(1);
            u8* begprev = a.beg;
            u8* b = New(&a, 1, u8);
            *b = 10;

            TAssert(b == begprev, "NewArena(1) + New(1, u8): b == begprev");
            TAssert(a.beg == a.end, "NewArena(1) + New(1, u8): beg == end");
            TAssert(*b == 10, "NewArena(1) + New(1, u8): *b == 10");
        }

        {
            Arena a = NewArena(10);
            u8* begprev = a.beg;
            u8* b = New(&a, 5, u8);
            b[0] = 10;
            b[1] = 20;
            b[2] = 30;
            b[3] = 40;
            b[4] = 50;
            TAssert(b == begprev, "NewArena(10) + New(5, u8): b == begprev");
            TAssert(a.beg != a.end, "NewArena(10) + New(5, u8): beg != end");
            TAssert(a.end - a.beg == 5, "NewArena(10) + New(5, u8): end - beg == 5");
            TAssert(b[0] == 10, "NewArena(10) + New(5, u8): b[0]");
            TAssert(b[1] == 20, "NewArena(10) + New(5, u8): b[1]");
            TAssert(b[2] == 30, "NewArena(10) + New(5, u8): b[2]");
            TAssert(b[3] == 40, "NewArena(10) + New(5, u8): b[3]");
            TAssert(b[4] == 50, "NewArena(10) + New(5, u8): b[4]");

            u8* begprev2 = a.beg;
            u8* b2 = New(&a, 5, u8);
            b2[0] = 60;
            b2[1] = 70;
            b2[2] = 80;
            b2[3] = 90;
            b2[4] = 100;
            TAssert(b2 == begprev2, "NewArena(10) + New(5, u8): b2 == begprev2");
            TAssert(a.beg == a.end, "NewArena(10) + New(5, u8): beg == end");
            TAssert(b[0] == 10, "NewArena(10) + New(5, u8) + New(5, u8): b[0]");
            TAssert(b[1] == 20, "NewArena(10) + New(5, u8) + New(5, u8): b[1]");
            TAssert(b[2] == 30, "NewArena(10) + New(5, u8) + New(5, u8): b[2]");
            TAssert(b[3] == 40, "NewArena(10) + New(5, u8) + New(5, u8): b[3]");
            TAssert(b[4] == 50, "NewArena(10) + New(5, u8) + New(5, u8): b[4]");
            TAssert(b2[0] == 60, "NewArena(10) + New(5, u8) + New(5, u8): b2[0]");
            TAssert(b2[1] == 70, "NewArena(10) + New(5, u8) + New(5, u8): b2[1]");
            TAssert(b2[2] == 80, "NewArena(10) + New(5, u8) + New(5, u8): b2[2]");
            TAssert(b2[3] == 90, "NewArena(10) + New(5, u8) + New(5, u8): b2[3]");
            TAssert(b2[4] == 100, "NewArena(10) + New(5, u8) + New(5, u8): b2[4]");
        }
    }
}

static void TestL() {
    TraceLog(LOG_INFO, "Running Tests");
    TestMath();
    TestMemory();

    if (TestFail) {
        TraceLog(LOG_ERROR, "Failed tests!");
        exit(1);
    }

    TraceLog(LOG_INFO, "Tests Pass");
}
