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
#define f32eq(x, y) (f32abs((x) - (y)) < (TOL))
// Prefer this lerp forumulation as at t=0 we will have exactly start and at t=1 we will have exactly end.
// The other formulation is subject to floating point precision issues where we might not get exactly
// end at t=1.
#define f32lerp(s, e, t) (((1.0f - (t)) * (s)) + ((t) * (e)))
// #define f32lerp(s, e, t) ((s) + (((e) - (s)) * (t)))
#define f32rand01() ((f32)rand() / (f32)RAND_MAX)
#define f32randab(a, b) (f32lerp((a), (b), (f32rand01())))

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
LCORE Mat4 Mat4PointAt(Vec3 forward) {
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

typedef struct Vertex {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
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

typedef u32 TextureId;

typedef struct LTexture {
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
typedef struct TextureCache {
    u32 txlen;
    u32 txcap;
    LTexture textures[MAX_TEXTURES];
} TextureCache;

static TextureCache TCACHE;

typedef enum TextureLabel { TX_DEFAULT = 0, TX_LFONT, TX_LFONT2, TX_WOODMAN_STAGE, TX_COUNT } TextureLabel;

static LTexture GetTexture(TextureLabel label) {
    TextureCache* tcache = &TCACHE;
    return tcache->textures[label];
}

#define TextureBottomLeft(tx) (Vec3Lit(-0.5f * (tx).width, -0.5f * (tx).height, 0.0f))

// We assume that a quad's center is the origin. aseprite assumes that a quads top left is the origin
// (with +x right and +y down). This will convert to a position that is properly offset from the
// quad center/origin.
static Vec3 AsepritePosToWorldPos(LTexture tx, Vec3 aseprite_pos) {
    // Flip y. origin now in bottom left corner.
    aseprite_pos.y = tx.height - aseprite_pos.y;

    // We want to shift the origin to the center but find the offset from the origin to
    // our aseprite pos hence the negative. Think about it in vectors. if my aseprite_pos
    // is a vector from origin (bottom left) to pos. then I shift my origin to the right. I now need
    // to find a vector from origin to bottom left (hence the negative) then add to that the
    // aseprite offset. the resulting vector is my new vector pos.
    //
    /*

             apos
             ^  ^
            /    \
           /       \
        bl <-------center
    */
    Vec3 pos = Vec3Add(aseprite_pos, Vec3Lit(-0.5f * tx.width, -0.5f * tx.height, 0.0f));
    return pos;
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

typedef struct LShader {
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
    // locations. just hardcode and make sure that the shader code specifies the correct layout.
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
            Vec2 uvscale;    // width/height in texture space of the rect to sample inside the texture [0, 1]
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

typedef struct ShaderPass {
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

typedef struct RenderState {
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

LCORE void DrawQuad(Vec3 center, f32 width, f32 height, Mat4 orient, Vec4 color) {
    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = GetMesh(M_QUAD);
    cmd->meshcmd.modelm = Mat4SVRT(Vec3Lit(width, height, 1.0f), orient, center);
    cmd->meshcmd.color = color;
}

LCORE void DrawAABB(Vec3 center, Vec3 radius, Vec4 color) {
    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = GetMesh(M_CUBE);
    cmd->meshcmd.modelm = Mat4SVRT(Vec3Scale(radius, 2.0f), Mat4Identity(), center);
    cmd->meshcmd.color = color;
}

// Remember
// (0, 0)
// * maps to bottom left corner in src texture space
// * maps to origin in dest world space.
// radius
// * need to be full width/height of quad in texture space
// * need to be half width/height in world space
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

LCORE void DrawLTextureEx(LTexture tx, Vec2 pos, f32 scale, f32 rotation, Vec4 color) {
    DrawLTexturePro(tx, Vec2Lit(0.0f, 0.0f), Vec2Lit(tx.width, tx.height), pos, Vec2Lit(scale, scale), rotation, color);
}

// Remember that pos will be the center of the quad.
// Scale will control the multiple of original size of the texture.
LCORE void DrawLTexture(LTexture tx, Vec2 pos, f32 scale) {
    DrawLTexturePro(tx, Vec2Lit(0.0f, 0.0f), Vec2Lit(tx.width, tx.height), pos,
                    Vec2Lit(tx.width * scale, tx.height * scale), 0.0f, LCLEAR);
}

// Assumes the font texture contains the ascii printable characters on a single line.
// Characters (note the space beginning the sequence is included):
// !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
LCORE void DrawLText(const char* text, u32 size, Vec3 pos, u32 fontsize, Vec4 color) {
    LTexture tx = GetTexture(TX_LFONT);

    u32 x = 28;
    u32 y = 52;
    // (width, height) of the src and dest recs).
    // misnomer name I think. the center adjusts by the full width each time.
    Vec2 r_src = Vec2Lit(x, y);
    // Vec2 r_dst = Vec2Lit(8, 8);
    Vec2 r_dst = Vec2Lit(fontsize, fontsize);

    for (u32 i = 0; i < size; ++i, pos = Vec3Add(pos, Vec3Lit(r_dst.x, 0.0f, 0.0f))) {
        u32 offset = text[i] - ' ';

        // center src and dest.
        Vec2 c_src = Vec2Lit((x * offset) % tx.width, y * (u32)((x * offset) / tx.width));
        Vec2 c_dst = Vec2Lit(pos.x, pos.y);
        DrawLTexturePro(tx, c_src, r_src, c_dst, r_dst, 0.0f, color);
    }
}

LCORE void DrawLTextF(Vec3 pos, u32 fontsize, Vec4 color, const char* format, ...) {
    char buffer[1024];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    DrawLText(buffer, strlen(buffer), pos, fontsize, color);
}

// Imagine the texture as a single pre baked one off texture. This will measure the width
// and height of that texture.
LCORE Vec2 MeasureLText(const char* text, u32 size, u32 fontsize) { return Vec2Lit(fontsize * size, fontsize); }

////////////////////////////////////////////////////////////
// S:Geometry

static bool DEBUG_EVER = false;

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

typedef struct Sphere {
    Vec3 c;  // center
    f32 r;   // radius
} Sphere;

// // Computes a sphere in the coord space of the passed in verts.
// static Sphere BoundingSphere(Vec3* vs, u32 vsize) {
//     Vec3 center = ComputeCentroid(vs, vsize);

//     f32 radius = 0.0f;
//     for (u32 i = 0; i < vsize; ++i) {
//         f32 len = Vec3Len(Vec3Sub(vs[i], center));
//         if (len > radius) {
//             radius = len;
//         }
//     }

//     return (Sphere){.c = center, .r = radius};
// }

// // Returns normal pointing from b to a if there is a collision.
// static bool TestSphereVsSphere(Sphere a, Sphere b, Vec3* normal, f32* depth) {
//     bool hit = false;
//     Vec3 dir = Vec3Sub(b.c, a.c);
//     f32 d = Vec3Dot(dir, dir);
//     f32 rr = a.r + b.r;
//     if (d <= rr * rr) {
//         hit = true;
//         // TODO: is this correct? shouldn't it be the diff between the dist between centers and the radius?
//         d = f32sqrt(d);
//         dir = Vec3Normalize(dir);
//     }

//     *normal = dir;
//     *depth = d;
//     return hit;
// }

typedef struct {
    Vec2 c;  // center.
    Vec2 r;  // half width extents along each axis.
} AABB2D;

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

    // If the center is inside of the aabb, then the normal and depth will not be correct, since the
    // closest point will be the center itself. But at least we will report the collision happened.
    *depth = f32sqrt(d) - circle.r;
    *normal = Vec2Normalize(disp);
    return true;
}

typedef struct AABB {
    Vec3 c;  // center.
    Vec3 r;  // half width extents along each axis.
} AABB;

// // Computes the axis aligned bounding box in the coordinate space of passed in vertices.
// static AABB ComputeAABB(Vec3* vs, u32 size) {
//     AABB aabb = {0};
//     for (u32 i = 0; i < ArraySizeOf(AXIS); ++i) {
//         Vec3 dir = AXIS[i];

//         f32 dmin = F32_MAX;
//         f32 dmax = -F32_MAX;
//         for (u32 j = 0; j < size; ++j) {
//             f32 d = Vec3Dot(vs[j], dir);
//             if (d < dmin) {
//                 dmin = d;
//             }
//             if (d > dmax) {
//                 dmax = d;
//             }
//         }

//         // compute the half width extent of the dimension of the axis.
//         // the center can be found by adding the radius to the min distance.
//         // value in the correct coordinate.
//         aabb.r.f[i] = 0.5f * (dmax - dmin);
//         aabb.c.f[i] = aabb.r.f[i] + dmin;
//     }

//     return aabb;
// }

// static bool TestAABBVsAABB(AABB a, AABB b, Vec3* normal, f32* depth) {
//     f32 dcx = a.c.f[0] - b.c.f[0];
//     f32 dx = (a.r.f[0] + b.r.f[0]) - f32abs(dcx);
//     if (dx <= 0.0) {
//         return false;
//     }

//     f32 dcy = a.c.f[1] - b.c.f[1];
//     f32 dy = (a.r.f[1] + b.r.f[1]) - f32abs(dcy);
//     if (dy <= 0.0f) {
//         return false;
//     }

//     f32 dcz = a.c.f[2] - b.c.f[2];
//     f32 dz = (a.r.f[2] + b.r.f[2]) - f32abs(dcz);
//     if (dz <= 0.0f) {
//         return false;
//     }

//     Vec3 n = Vec3Lit(1.0f * (dcx < 0.0f ? -1.0f : 1.0f), 0.0f, 0.0f);
//     f32 d = dx;
//     if (dy < d || dz < d) {
//         if (dy < dz) {
//             n = Vec3Lit(0.0f, 1.0f * (dcy < 0.0f ? -1.0f : 1.0f), 0.0f);
//             d = dy;
//         } else {
//             n = Vec3Lit(0.0f, 0.0f, 1.0f * (dcy < 0.0f ? -1.0f : 1.0f));
//             d = dz;
//         }
//     }

//     *normal = n;
//     *depth = d;
//     return true;
// }

// // Returns normal pointing from aabb to sphere if there is a collision.
// static bool TestSphereVsAABB(Sphere s, AABB aabb, Vec3* normal, f32* depth) {
//     Vec3 max = Vec3Add(aabb.c, aabb.r);
//     Vec3 min = Vec3Sub(aabb.c, aabb.r);

//     // Find closest point from sphere center to the space bounded by aabb (which might be inside of) aabb.
//     Vec3 cp = Vec3Zero();
//     for (u32 i = 0; i < ArraySizeOf(AXIS); ++i) {
//         f32 v = s.c.f[i];
//         if (v < min.f[i]) {
//             v = min.f[i];
//         }
//         if (v > max.f[i]) {
//             v = max.f[i];
//         }
//         cp.f[i] = v;
//     }

//     bool hit = false;
//     Vec3 dir = Vec3Sub(cp, s.c);
//     f32 d = Vec3Dot(dir, dir);
//     if (d <= s.r * s.r) {
//         // if (DEBUG_EVER) {
//         //     breakpoint();
//         // }
//         hit = true;
//         d = f32sqrt(d);
//         // TODO: Add SLOP.
//         d = (s.r - d);
//         // Want to point normal towards sphere so negate.
//         dir = Vec3Normalize(Vec3Negate(dir));
//     }

//     *normal = dir;
//     *depth = d;
//     return hit;
// }

typedef enum PolyhedronType { PH_NONE = 0, PH_CIRCLE, PH_SPHERE, PH_AABB2D, PH_AABB, PH_COUNT } PolyhedronType;

typedef struct Polyhedron {
    PolyhedronType type;

    union {
        Circle circle;
        AABB2D aabb2d;
        Sphere sphere;
        AABB aabb;
    };
} Polyhedron;

// lc: bottom left corner
// dims: full width/height of bbox
// grid_dims: width x height of sprite
// Useful for setting collision info based on visually drawn box in programs like aseprite.
// We assume that (0, 0) is the bottom left *corner* (not the center of the bottom left pixel).
// To translate from aseprite, count the number of pixels (where bottom left pixel counts as 1) until the first
// pixel of the collider, then subtract (-1, -1) to get the lc.
LCORE void PolyhedronSetAABB2DFromGridRect(Polyhedron* shape, Vec2 lc, Vec2 dims, Vec2 grid_dims) {
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

// Important to pass decomposed transformation matrix (scale, rot, translate) because some polyhedron
// like world axis-aligned shapes do not rotate for instance.
// TODO: Consider making an interface where I get the correct polyhedron transformation matrix as a separate
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

typedef struct {
    Vec3 pos;
    Vec3 scale;
    Mat4 orient;

    Vec3 posa;
    Vec3 posb;
    f32 posdt;

    LMesh mid;

    TextureId txid;
    Vec2 uvscale;
    Vec2 uvoffset;

    // Animation related fields. Below vec2s should be integer. Maybe I'm just being paranoid about float
    // precision issues.
    Vec2 c;     // bottom left of rect in texture space
    Vec2 r;     // full width and height of rect in texture space
    Vec2 step;  // How far along each axis to move to the next frame.
    u32 nframes;
    f32 frames_dt;    // Amount of time we've currently gone through the animation.
    f32 frames_time;  // total time for one loop of the animation.
    // u32 frame;
    // f32 frame_dt;
    // f32 frame_time;  // How long the animation is supposed to take.

    Vec4 color;
} GraphicsInfo;

static void GraphicsInfoMemset(GraphicsInfo* graphics) {
    memset(graphics, 0, sizeof(GraphicsInfo));
    graphics->orient = Mat4Identity();
    graphics->pos = Vec3Zero();
    graphics->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
    graphics->txid = GetTexture(TX_DEFAULT).id;
    graphics->uvscale = Vec2Lit(1.0f, 1.0f);
    graphics->uvoffset = Vec2Lit(0.0f, 0.0f);
    graphics->color = LCLEAR;

    graphics->c = Vec2Zero();
    graphics->r = Vec2Lit(1.0f, 1.0f);
    graphics->step = Vec2Zero();
    // graphics->nframes = 1;
    graphics->frames_dt = 0.0f;
    graphics->frames_time = 0.0f;
}

typedef struct {
    Vec2 c;  // bottom left corner.
    Vec2 r;  // width + height.
} Rect;

typedef struct {
    Rect start;  // Start rect of the animation.
    Vec2 step;
    u32 nframes;
} AnimationRect;

// c_src - bottom left hand corner of sprite (in png space, not uvspace)
// r_src - full width/height of sprite in png space not uvspace.
LCORE void GraphicsInfoSetSprite(GraphicsInfo* graphics, LTexture tx, Vec2 c_src, Vec2 r_src, f32 orient, f32 scale) {
    graphics->scale = Vec3Lit(r_src.x * scale, r_src.y * scale, 1.0f);
    graphics->orient = Mat4Rotate(orient, Vec3Lit(0.0f, 0.0f, 1.0f));

    graphics->mid = GetMesh(M_QUAD);

    graphics->txid = tx.id;
    // TODO: its possible we want these to be ints?
    graphics->uvscale = Vec2Lit(r_src.x / tx.width, r_src.y / tx.height);
    graphics->uvoffset = Vec2Lit(c_src.x / tx.width, c_src.y / tx.height);
}

LCORE void GraphicsInfoSetSpriteRect(GraphicsInfo* graphics, LTexture tx, Rect src, f32 orient, f32 scale) {
    graphics->scale = Vec3Lit(src.r.x * scale, src.r.y * scale, 1.0f);
    graphics->orient = Mat4Rotate(orient, Vec3Lit(0.0f, 0.0f, 1.0f));

    graphics->mid = GetMesh(M_QUAD);

    graphics->txid = tx.id;
    // TODO: its possible we want these to be ints?
    graphics->uvscale = Vec2Lit(src.r.x / tx.width, src.r.y / tx.height);
    graphics->uvoffset = Vec2Lit(src.c.x / tx.width, src.c.y / tx.height);
}

LCORE void GraphicsInfoSetSpriteAnimation(GraphicsInfo* graphics, LTexture tx, Rect src, Vec2 step, u32 nframes,
                                          f32 frames_time, f32 orient, f32 scale) {
    graphics->scale = Vec3Lit(src.r.x * scale, src.r.y * scale, 1.0f);
    graphics->orient = Mat4Rotate(orient, Vec3Lit(0.0f, 0.0f, 1.0f));

    graphics->mid = GetMesh(M_QUAD);

    graphics->txid = tx.id;
    graphics->c = src.c;
    graphics->r = src.r;
    graphics->step = step;
    graphics->nframes = nframes;
    graphics->frames_dt = 0.0f;
    graphics->frames_time = frames_time;

    // TODO: its possible we want these to be ints?
    // I need to set uvscale, since that does not change over the animation. But possible don't need to set uvoffset
    // since that can be calculated from the animation frame time.
    graphics->uvscale = Vec2Lit(src.r.x / tx.width, src.r.y / tx.height);
    graphics->uvoffset = Vec2Lit(src.c.x / tx.width, src.c.y / tx.height);
}

LCORE void GraphicsInfoSetSpriteAnimationRect(GraphicsInfo* graphics, LTexture tx, AnimationRect rect, f32 frames_time,
                                              f32 orient, f32 scale) {
    GraphicsInfoSetSpriteAnimation(graphics, tx, rect.start, rect.step, rect.nframes, frames_time, orient, scale);
}

LCORE void GraphicsInfoSetTexture(GraphicsInfo* graphics, LTexture tx, f32 orient, f32 scale) {
    GraphicsInfoSetSprite(graphics, tx, Vec2Zero(), Vec2Lit(tx.width, tx.height), orient, scale);
}

LCORE void GraphicsInfoSetShape(GraphicsInfo* graphics, LMesh mesh, Vec3 scale, Vec4 color) {
    graphics->mid = mesh;
    graphics->scale = scale;
    graphics->color = color;
}

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
    EFLAG_GOAL = 1 << 6,
    EFLAG_HAZARD = 1 << 7,
    EFLAG_PLATFORM = 1 << 8,
    EFLAG_SUBMERGABLE = 1 << 9,
} EntityFlag;

typedef u32 EntityId;
static const EntityId NULLID = 0;

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
    Vec3 forces;
    f32 friction;
    f32 restitution;

    // TODO: Deprecate
    LMesh mesh;

    Vec4 color;

    Polyhedron shape;

    GraphicsInfo graphics;

    /////////////////////////////////////////////////////////////////
    // Gameplay fields.
    EntityId hazardid;
    EntityId platformid;
    EntityId prev_platformid;
    f32 leaping;

    bool surfaced;
    f32 surfaced_time;
    f32 submerged_time;
    f32 surfaced_dt;
} Entity;

#define ENTITY_CAP 1024
#define ENTITY_INACTIVE_TO_BE_CAP 20

typedef struct EntityManager {
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
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
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
    GraphicsInfoMemset(&entity->graphics);
    FlagOn(entity->flags, EFLAG_DRAW);
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
typedef struct CollisionInfo {
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

typedef struct {
    LTexture background;
    // How to find. make a 128 x 120 selection in aseprite. the pixel value that is 1, 1 off from the
    // corner you finish selecting is the start.
    // This will end up being the origin in worldspace.
    Vec3 camera_start;
    Vec3 player_start;
} Stage;

typedef enum {
    STAGE_NONE = 0,
    STAGE_WOODMAN,
    STAGE_COUNT,
} StageId;

static Stage GetStage(StageId sid) {
    static Stage STAGES[STAGE_COUNT] = {0};
    static bool INIT = false;

    if (!INIT) {
        INIT = true;

        {
            LTexture tx = GetTexture(TX_WOODMAN_STAGE);
            Stage stage = {0};
            stage.background = tx;
            stage.camera_start = AsepritePosToWorldPos(tx, Vec3Lit(129, 121, 1.0f));
            stage.player_start = AsepritePosToWorldPos(tx, Vec3Lit(127.0f, 181.0f, 0.0f));
            STAGES[STAGE_WOODMAN] = stage;
        }
    }

    return STAGES[sid];
}

////////////////////////////////////////////////////

#define MAX_SCENES 10

typedef enum {
    SCENE_DEFAULT = 0,
    SCENE_TITLE_SCREEN,
    SCENE_STAGE_SELECT,
    SCENE_STAGE,
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
    Stage stage;
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

static void SceneDefaultTick(GameState* gstate) {
    BeginRender();

    ShaderPass* pass = BeginShaderPass(GetShader());
    u32 width = gstate->renderw;
    u32 height = gstate->renderh;
    pass->proj = Mat4Ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);

    char msg[] = "DEFAULT SCENE";
    u32 size = strlen(msg);

    u32 fontsize = 10.0f;
    Vec2 dim = MeasureLText(msg, size, fontsize);

    // Center text by shifting center forward by half a text cell size and then shifting backward
    // by half the text size.
    Vec3 pos = Vec3Lit(0.5f * (width + fontsize - dim.x), 0.70f * height, 0);
    DrawLText(msg, size, pos, fontsize, LYELLOW);

    EndShaderPass();

    EndRender();
}

static void TitleScreenTick(GameState* gstate) {
    static Vec3 POS = {0};

    if (!gstate->init) {
        u32 width = gstate->renderw;
        u32 height = gstate->renderh;

        // char msg[] = "Press ENTER To Start";
        char msg[] = "E";
        u32 size = strlen(msg);

        u32 fontsize = 10.0f;
        Vec2 dim = MeasureLText(msg, size, fontsize);

        // Center text by shifting center forward by half a text cell size and then shifting backward
        // by half the text size.
        POS = Vec3Lit(0.5f * (width + fontsize - dim.x), 0.70f * height, 0);
    }

    if (IsKeyPressed(KEY_ENTER)) {
        gstate->nextsceneid = SCENE_STAGE_SELECT;
    }

    // Debug text positioning + rendering.
    if (true) {
        float dx = 1.0f;
        if (IsKeyPressed(KEY_UP)) {
            POS.y += dx;
        } else if (IsKeyPressed(KEY_DOWN)) {
            POS.y -= dx;
        } else if (IsKeyPressed(KEY_LEFT)) {
            POS.x -= dx;
        } else if (IsKeyPressed(KEY_RIGHT)) {
            POS.x += dx;
        }
    }

    BeginRender();

    ShaderPass* pass = BeginShaderPass(GetShader());

    ClearBuffer(LBLACK);

    u32 width = gstate->renderw;
    u32 height = gstate->renderh;
    pass->proj = Mat4Ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);

    char msg[] = "Press ENTER To Start";
    u32 size = strlen(msg);

    u32 fontsize = 10.0f;
    Vec2 dim = MeasureLText(msg, size, fontsize);

    // Center text by shifting center forward by half a text cell size and then shifting backward
    // by half the text size.
    Vec3 pos = Vec3Lit(0.5f * (width + fontsize - dim.x), 0.70f * height, 0);
    DrawLText(msg, size, pos, fontsize, LYELLOW);

    // LTexture tx = GetTexture(TX_LFONT2);
    // DrawLTexture(GetTexture(TX_LFONT2), Vec3ToVec2(POS), 1.0f);

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

    u32 fontsize = 10.0f;
    Vec2 dim = MeasureLText(msg, size, fontsize);

    // Center text by shifting center forward by half a text cell size and then shifting backward
    // by half the text size.
    Vec3 pos = Vec3Lit(0.5f * (width + fontsize - dim.x), 0.70f * height, 0);
    DrawLText(msg, size, pos, fontsize, LYELLOW);

    EndShaderPass();

    EndRender();
}

static void StageTick(GameState* gstate) {
    static LCamera CAMERA = {0};
    static bool SPAWNED = false;

    if (!gstate->init) {
        gstate->stage = GetStage(STAGE_WOODMAN);
        CAMERA.eye = gstate->stage.camera_start;
        CAMERA.orient = Vec3Zero();
        SPAWNED = false;
    }

    // Debug text positioning + rendering.
    if (true) {
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
    }

    if (gstate->update) {
        if (!SPAWNED) {
            SPAWNED = true;
            Entity* e = AllocateEntity();
            e->pos = gstate->stage.player_start;
            GraphicsInfoSetShape(&e->graphics, GetMesh(M_QUAD), Vec3Lit(24.0f, 24.0f, 0.0f), LBLUE);
            FlagOn(e->flags, EFLAG_PLAYER_CONTROLLED);
        }

        // Update graphics.
        for (u32 i = 0; i < GetNumEntities(); ++i) {
            Entity* e = GetEntity(i);
            if (!TestFlag(e->flags, EFLAG_ACTIVE)) {
                continue;
            }

            if (TestFlag(e->flags, EFLAG_DRAW)) {
                GraphicsInfo* graphics = &e->graphics;
                graphics->pos = e->pos;
                graphics->orient = e->orient;
            }
        }
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
            DrawLTexture(gstate->stage.background, Vec2Zero(), 1.0f);
        }

        for (u32 i = 0; i < GetNumEntities(); ++i) {
            Entity* e = GetEntity(i);
            if (!TestFlag(e->flags, EFLAG_ACTIVE)) {
                continue;
            }

            if (TestFlag(e->flags, EFLAG_DRAW)) {
                DrawCmd* cmd = AddDrawMeshCmd();

                cmd->meshcmd.modelm = Mat4SVRT(e->graphics.scale, e->graphics.orient, e->graphics.pos);
                cmd->meshcmd.color = e->graphics.color;
                cmd->meshcmd.uvscale = e->graphics.uvscale;
                cmd->meshcmd.uvoffset = e->graphics.uvoffset;
                cmd->meshcmd.txid = e->graphics.txid;
                cmd->meshcmd.meshid = e->graphics.mid;
            }
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

        u32 fontsize = 10.0f;
        Vec2 dim = MeasureLText(msg, size, fontsize);

        // Center text by shifting center forward by half a text cell size and then shifting backward
        // by half the text size.
        Vec3 pos = Vec3Lit(0.5f * (width + fontsize - dim.x), 0.70f * height, 0);
        DrawLText(msg, size, pos, fontsize, LYELLOW);

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
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetConfigFlags(FLAG_VSYNC_HINT);
    SetTargetFPS(60);
    // DisableCursor();

    InitAudio();
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
        gstate->scenefuncs[SCENE_TITLE_SCREEN] = TitleScreenTick;
        gstate->scenefuncs[SCENE_STAGE_SELECT] = StageSelectTick;
        gstate->scenefuncs[SCENE_STAGE] = StageTick;
        Assert(SCENE_COUNT < MAX_SCENES);
        gstate->scenefuncssize = idx;
    }
    GameStateLoadScene(gstate, SCENE_TITLE_SCREEN);
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
                DEBUG_EVER = true;
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

static void TestL() {
    TraceLog(LOG_INFO, "Running Tests");
    TestMath();

    if (TestFail) {
        TraceLog(LOG_ERROR, "Failed tests!");
        exit(1);
    }

    TraceLog(LOG_INFO, "Tests Pass");
}
