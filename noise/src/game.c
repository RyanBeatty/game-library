#ifndef UNITY_BUILD
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <platform.h>
#include <shared.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "glad.h"
#include "stb_truetype.h"
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
        PlatformLog(LLOG_INFO, "expected Mat4 b:");
        Mat4Print(b);
        PlatformLog(LLOG_INFO, "got Mat4 a:");
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
// S:Color

typedef Vec4 RGBA;

#define RGBALit(r, g, b, a) (Vec4Lit((r) / 255.0f, (g) / 255.0f, (b) / 255.0f, (a)))
#define RGBLit(r, g, b) (RGBALit((r), (g), (b), 1.0f))

static const Vec4 LCLEAR = Vec4Lit(1.0f, 1.0f, 1.0f, 1.0f);
static const Vec4 LRED = Vec4Lit(1.0f, 0.0f, 0.0f, 1.0f);
static const Vec4 LGREEN = Vec4Lit(0.0f, 1.0f, 0.0f, 1.0f);
static const Vec4 LBLUE = Vec4Lit(0.0f, 0.0f, 1.0f, 1.0f);
static const Vec4 LBLACK = Vec4Lit(0.0f, 0.0f, 0.0f, 1.0f);
static const Vec4 LPURPLE = Vec4Lit(1.0f, 0.0f, 1.0f, 1.0f);
static const Vec4 LORANGE = Vec4Lit(1.0f, 0.5f, 0.0f, 1.0f);
static const Vec4 LGRAY = Vec4Lit(199.0f / 255.0f, 197.0f / 255.0f, 198.0f / 255.0f, 1.0f);
static const Vec4 LYELLOW = Vec4Lit(1.0f, 1.0f, 0.0f, 1.0f);
static const Vec4 LWHITE = Vec4Lit(1.0f, 1.0f, 1.0f, 1.0f);

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

    byte* data;
} LTexture;

typedef enum {
    FILTER_NONE = 0,
    FILTER_NEAREST = GL_NEAREST,
    FILTER_LINEAR = GL_LINEAR,
} TextureFilter;

typedef struct {
    union {
        u32 rgba;

        struct {
            u8 r;
            u8 g;
            u8 b;
            u8 a;
        };
    };
} RGBA256;

LCORE RGBA256 LTextureGetPx(LTexture tx, i32 x, i32 y) {
    RGBA256 rgba = {0};
    // Remember the size in bytes for each cell is sizeof(u32)
    i32 offset = (tx.width * y * sizeof(u32)) + (x * sizeof(u32));
    rgba.r = tx.data[offset + 0];
    rgba.g = tx.data[offset + 1];
    rgba.b = tx.data[offset + 2];
    rgba.a = tx.data[offset + 3];
    return rgba;
}

// :data - flattened 2d array of rgba data (e.g. 4 bytes, 1 byte per channel).
// :width - length of width of buffer in rgba units.
// :height - length of height of buffer in rgba units.
// So size of buffer should be (width * 4 bytes) * (height * 4 bytes)
// (0, 0) in uv space maps to the bottom right corner of the texture visually which maps to
// the start of the buffer.
// NOTE: We take ownership of data here.
LCORE LTexture TextureFromBuffer(byte* data, i32 width, i32 height, TextureFilter filter) {
    LTexture tx = {0};
    tx.width = width;
    tx.height = height;
    tx.data = data;
    glGenTextures(1, &tx.id);
    glBindTexture(GL_TEXTURE_2D, tx.id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tx.width, tx.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tx.data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tx;
}

static LTexture TextureFromFile(const char* filename, TextureFilter filter) {
    LTexture tx = {0};
    i32 n = 0;
    // TODO: Change the allocator on stb.
    stbi_set_flip_vertically_on_load(true);
    u8* data = stbi_load(filename, &tx.width, &tx.height, &n, 0);
    Assert(data != NULL);

    // The texture takes ownership of the data.
    tx = TextureFromBuffer(data, tx.width, tx.height, filter);
    // stbi_image_free(data);
    return tx;
}

#define MAX_TEXTURES 32
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
    TX_BLUE_WIZARD,
    TX_IMAGE_TEST,
    TX_IMAGE_TEST2,
    TX_IMAGE_TEST3,
    TX_IMAGE_TEST4,
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

typedef struct {
    LTexture tx;

    // The min and max coordinate in image coordinates [0, with], [0, height]
    Vec2 min;
    Vec2 max;
    Vec2 dims;

    // UV space coordinates for drawing (computed once when loaded)
    Vec2 minuv;
    Vec2 dimsuv;
} Sprite;

#define MAX_SPRITES (32)

typedef struct {
    // TODO: Figure out memory later if we really need to make this dynamic. Remember that
    // audio memory takes up so much that it might not make sense to focus on optimizing this.
    Sprite data[MAX_SPRITES];
    isize len;
    isize cap;
} SpriteArr;

typedef enum {
    SBB_FULL = 0,  // bb = width, height of texture
    SBB_CLOSEST_FIT,
} SpriteBoundingBox;

LCORE Sprite SpriteFromMinMax(LTexture tx, Vec2 min, Vec2 max) {
    Sprite sprite = {.tx = tx, .min = min, .max = max};
    // think more about if adding 1,1 here is correct. I think it is.
    sprite.dims = Vec2Sub(Vec2Add(sprite.max, Vec2Lit(1.0f, 1.0f)), sprite.min);
    sprite.minuv = Vec2Lit(sprite.min.x / sprite.tx.width, sprite.min.y / sprite.tx.height);
    sprite.dimsuv = Vec2Lit(sprite.dims.x / sprite.tx.width, sprite.dims.y / sprite.tx.height);
    return sprite;
}

LCORE Sprite SpriteFromLTexture(LTexture tx, SpriteBoundingBox bbox) {
    Vec2 min = {0};
    Vec2 max = {0};
    switch (bbox) {
        case SBB_FULL: {
            min = Vec2Zero();
            max = Vec2Lit(tx.width - 1, tx.height - 1);
            break;
        }
        case SBB_CLOSEST_FIT: {
            // Init the min to be the max px index of the texture. If we don't do this, min will always be (0, 0).
            min = Vec2Lit(tx.width - 1, tx.height - 1);
            for (i32 i = 0; i < tx.width; ++i) {
                for (i32 j = 0; j < tx.height; ++j) {
                    Vec2 pos = Vec2Lit(i, j);
                    RGBA256 rgba = LTextureGetPx(tx, i, j);
                    if (rgba.a != 0.0f) {
                        min.x = f32min(min.x, pos.x);
                        min.y = f32min(min.y, pos.y);
                        max.x = f32max(max.x, pos.x);
                        max.y = f32max(max.y, pos.y);
                    }
                }
            }
            break;
        }
        default: {
            PlatformLog(LLOG_ERROR, "Unknown sprite bounding box %d", bbox);
            Assert(0);
        }
    }

    Sprite sprite = SpriteFromMinMax(tx, min, max);
    return sprite;
}

LCORE Sprite DefaultSprite() { return SpriteFromLTexture(GetTexture(TX_DEFAULT), SBB_FULL); }

// This extra layer of indirection might be redundant. But possible we will want to store other data here, so
// leave for now.
typedef struct {
    SpriteArr sprites;
    // For time based animations, this will be elapsed time in seconds for event based animations (like
    // walk cycles), this will be in range 0-1
    f32 duration;
} Animation;

LCORE Animation AnimationFromSprite(Sprite sprite) {
    Animation animation = {.duration = 0.0f};
    animation.sprites.cap = ArraySizeOf(animation.sprites.data);
    ArrPush(&animation.sprites, sprite);
    return animation;
}

LCORE Animation DefaultAnimation() { return AnimationFromSprite(DefaultSprite()); }

// TODO: This should be some sort of platform layer thing.
i32 CountFilesInDir(const char* path) {
    DIR* dir;
    struct dirent* entry;
    i32 count = 0;

    dir = opendir(path);
    if (dir == NULL) {
        PlatformLog(LLOG_ERROR, "error opening dir: %s", path);
    }

    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (entry->d_name[0] == '.' &&
            (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
            continue;
        }

        count++;
    }

    closedir(dir);
    return count;
}

// Assumes that all files under :dir: follow the format of :filename_prefixNNNNN.png where NNNNN
// is a 0 left padded number denoting the frame.
LCORE Animation AnimationFromDisk(const char* dir, const char* filename_prefix, f32 duration) {
    Animation animation = {.duration = duration};
    animation.sprites.cap = ArraySizeOf(animation.sprites.data);

    SpriteArr* sprites = &animation.sprites;
    i32 nfiles = CountFilesInDir(dir);

    TextureFilter filter = FILTER_LINEAR;
    for (isize i = 0; i < nfiles; ++i) {
        char filename[256] = {0};
        snprintf(filename, ArraySizeOf(filename) - 1, "%s/%s%05lld.png", dir, filename_prefix, i);
        LTexture tx = TextureFromFile(filename, filter);
        ArrPush(sprites, SpriteFromLTexture(tx, SBB_FULL));
    }

    return animation;
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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        tcache->textures[TX_DEFAULT] = tx;
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_LFONT] = TextureFromFile("assets/textures/LFont.png", FILTER_NEAREST);
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_LFONT2] = TextureFromFile("assets/textures/lfont2.png", FILTER_NEAREST);
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_WOODMAN_STAGE] = TextureFromFile("assets/textures/woodman-stage.png", FILTER_NEAREST);
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_LADDER_TEST_STAGE] =
            TextureFromFile("assets/textures/stages/ladder-test.png", FILTER_NEAREST);
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_ROOM_TEST_STAGE] = TextureFromFile("assets/textures/stages/room-test.png", FILTER_NEAREST);
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_MEGAMAN] = TextureFromFile("assets/textures/megaman.png", FILTER_NEAREST);
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_MEGAMAN2] = TextureFromFile("assets/textures/megaman2.png", FILTER_NEAREST);
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_BLUE_WIZARD] = TextureFromFile("assets/textures/blue-wizard.png", FILTER_LINEAR);
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_IMAGE_TEST] = TextureFromFile("assets/textures/image-test.png", FILTER_NEAREST);
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_IMAGE_TEST2] = TextureFromFile("assets/textures/image-test2.png", FILTER_NEAREST);
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_IMAGE_TEST3] = TextureFromFile("assets/textures/image-test3.png", FILTER_NEAREST);
        ++tcache->txlen;
    }

    {
        Assert(tcache->txlen < tcache->txcap);
        tcache->textures[TX_IMAGE_TEST4] = TextureFromFile("assets/textures/image-test4.png", FILTER_NEAREST);
        ++tcache->txlen;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// S:noise

// https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
u32 PCGHash(u32 x) {
    u32 state = x * 747796405u + 2891336453u;
    u32 word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

u32 PCGHash2D(u32 x, u32 y) {
    u32 seed = (x * 1610612741u) + (y * 1791552031u);
    return PCGHash(seed);
}

// Generates a whitenoise texture with pixel dimension width x height
static LTexture GenerateWhitenoiseTexture(Arena* temp, i32 width, i32 height) {
    ByteBuffer buffer = {.cap = width * height * 4};  // * 4 is because we store rgba data
    buffer.data = New(temp, buffer.cap, byte);

    for (i32 i = 0; i < width; ++i) {
        for (i32 j = 0; j < height; ++j) {
            u32 rand = PCGHash((i * width) + j);
            // shift noise from [0-1] with some precision loss potentially due to large float division.
            f32 noise = ((f32)rand) / ((f32)U32_MAX);
            // shift noise from [0-1] to [0-255]
            i32 rgb = (i32)(noise * 255.0f);
            PushArrS(&buffer, rgb);
            PushArrS(&buffer, rgb);
            PushArrS(&buffer, rgb);
            PushArrS(&buffer, 255);
        }
    }

    return TextureFromBuffer(buffer.data, width, height, FILTER_NEAREST);
}

static LTexture GenValueNoiseTexture(Arena* temp, i32 width, i32 height, i32 cell_size) {
    ByteBuffer buffer = {.cap = width * height * 4};
    buffer.data = New(temp, buffer.cap, byte);

    for (i32 i = 0; i < width; ++i) {
        for (i32 j = 0; j < height; ++j) {
            // Determine bottom left corner coordinates based on lattice size.
            u32 x = i / (u32)cell_size;
            u32 y = j / (u32)cell_size;

            f32 bl = (f32)PCGHash2D(x, y) / ((f32)U32_MAX);
            f32 br = (f32)PCGHash2D(x + cell_size, y) / ((f32)U32_MAX);
            f32 tl = (f32)PCGHash2D(x, y + cell_size) / ((f32)U32_MAX);
            f32 tr = (f32)PCGHash2D(x + cell_size, y + cell_size) / ((f32)U32_MAX);

            // Determine the percentage within the cell we are in x and y direction, that
            // is our lerp factor.
            f32 tx = ((f32)(i - x) / cell_size);
            f32 ty = ((f32)(j - y) / cell_size);

            // bilinear lerp between the bottom and top lerped values.
            // Shift noise range from [0, 1], to [0, 255]
            f32 l1 = f32lerp(bl, br, tx);
            f32 l2 = f32lerp(tl, tr, tx);
            u32 noise = (u32)(f32lerp(l1, l2, ty) * 255.0f);

            // // Find bottom left corner uv.
            // Vec2 bl = Vec2Lit((f32)i / (f32)width, (f32)j / (f32)height);
            // Vec2 br = Vec2Add(bl, Vec2Lit(1.0f, 0.0f));
            // Vec2 tl = Vec2Add(bl, Vec2Lit(0.0f, 1.0f));
            // Vec2 tr = Vec2Add(bl, Vec2Lit(1.0f, 1.0f));

            PushArrS(&buffer, noise);
            PushArrS(&buffer, noise);
            PushArrS(&buffer, noise);
            PushArrS(&buffer, 255);
        }
    }

    return TextureFromBuffer(buffer.data, width, height, FILTER_NEAREST);
}

///////////////////////////////////////////////////////////////////////////////
// S:render

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
    // TODO: Unclear to me if we want linear or nearest here. pixel art seems to look better with nearest
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
                Triangle tri = ts[ti];
                u32 edges[][2] = {{tri.va, tri.vb}, {tri.vb, tri.vc}, {tri.vc, tri.va}};
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

                u32 a = midpoints[tri.va][tri.vb];
                u32 b = midpoints[tri.vb][tri.vc];
                u32 c = midpoints[tri.vc][tri.va];

                Assert(ts2size < ArraySizeOf(ts2));
                ts2[ts2size++] = TriLit(tri.va, a, c);
                Assert(ts2size < ArraySizeOf(ts2));
                ts2[ts2size++] = TriLit(tri.vb, b, a);
                Assert(ts2size < ArraySizeOf(ts2));
                ts2[ts2size++] = TriLit(tri.vc, c, b);
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
            Vec2 uvoffset;  // offset of the bottom left corner/origin in texture space (0, 0)
            Vec2 uvscale;   // width/height in texture space of the Aserect to sample inside the texture [0, 1]
            LTexture tx;
        } meshcmd;

        // Clears buffer with color.
        struct {
            Vec4 color;
        } clearcmd;
    };

    // Pass arbitrary shader data (ex. like new uniforms to set) via this member.
    // Mostly exists as convenience when iterating on new shader ideas.
    void* extra;
} DrawCmd;

// Assumes the struct has been memset already.
LCORE void DrawMeshCmdInit(DrawCmd* cmd) {
    cmd->type = CMD_DRAW_MESH;
    cmd->meshcmd.color = LCLEAR;
    cmd->meshcmd.modelm = Mat4Identity();
    cmd->meshcmd.uvoffset = Vec2Lit(0.0f, 0.0f);
    cmd->meshcmd.uvscale = Vec2Lit(1.0f, 1.0f);
    cmd->meshcmd.tx = GetTexture(TX_DEFAULT);
}

#define GLSL(src) "#version 330 core\n" #src

typedef u32 ShaderId;

// Forward decls to be able to define enable shader func. This is somewhat akward, maybe consider
// trying to do what the File Pilot guy does and auto gen header files. That might be work tho.
struct ShaderPass;
typedef struct ShaderPass ShaderPass;
struct LShader;
typedef struct LShader LShader;

typedef void (*EnableShaderFunc)(LShader*, ShaderPass*);
typedef void (*SetUniformsFunc)(LShader*, DrawCmd*);

struct LShader {
    ShaderId pid;  // program id.
    u32 vid;
    u32 fid;

    EnableShaderFunc enable_func;
    SetUniformsFunc set_uniforms_func;
};

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
            PlatformLog(LLOG_ERROR, "Error compiling vertex shader: %s", error);
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
            PlatformLog(LLOG_ERROR, "Error compiling fragment shader: %s", error);
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

#define MAX_CMDS 4092

// No typedef because it is forward declared above.
struct ShaderPass {
    ShaderId _sid;
    LShader shader;

    ///////////////////////
    // Uniform data that should be set once for a pass here.
    Mat4 view;
    Mat4 proj;
    ///////////////////////

    Vec2 resolution;
    f32 dt;
    f32 timer;
    i32 nframe;

    u32 clen;
    u32 ccap;
    DrawCmd cmds[MAX_CMDS];
};

LCORE void EnableDefaultShader(LShader* shader, ShaderPass* pass) {
    static bool INIT = false;
    static i32 V_VIEW = 0;
    static i32 V_PROJ = 0;
    static i32 F_RESOLUTION = 0;
    static i32 F_TIMER = 0;
    static i32 F_NFRAME = 0;

    glUseProgram(shader->pid);

    if (!INIT) {
        INIT = true;
        V_VIEW = glGetUniformLocation(shader->pid, "vView");
        V_PROJ = glGetUniformLocation(shader->pid, "vProj");
        F_RESOLUTION = glGetUniformLocation(shader->pid, "resolution");
        F_TIMER = glGetUniformLocation(shader->pid, "timer");
        F_NFRAME = glGetUniformLocation(shader->pid, "NFrame");

        // IMPORTANT! Set texture uniform loc.
        glUniform1i(glGetUniformLocation(shader->pid, "texture0"), 0);
    }

    glUniformMatrix4fv(V_VIEW, 1, GL_FALSE, (const f32*)&pass->view.m);
    glUniformMatrix4fv(V_PROJ, 1, GL_FALSE, (const f32*)&pass->proj.m);

    glUniform2fv(F_RESOLUTION, 1, (const f32*)&pass->resolution.f);
    glUniform1f(F_TIMER, pass->timer);

    glUniform1i(F_NFRAME, pass->nframe);
}

LCORE void DefaultShaderSetUniforms(LShader* shader, DrawCmd* cmd) {
    static bool INIT = false;
    static i32 V_MODEL = 0;
    static i32 V_UVCOLOR = 0;
    static i32 V_UVSCALE = 0;
    static i32 V_UVOFFSET = 0;
    static i32 F_TEXTURE0_RESOLUTION = 0;

    Assert(cmd->type == CMD_DRAW_MESH);
    if (!INIT) {
        INIT = true;

        V_MODEL = glGetUniformLocation(shader->pid, "vModel");
        V_UVCOLOR = glGetUniformLocation(shader->pid, "vUColor");
        V_UVOFFSET = glGetUniformLocation(shader->pid, "vUVOffset");
        V_UVSCALE = glGetUniformLocation(shader->pid, "vUVScale");
        F_TEXTURE0_RESOLUTION = glGetUniformLocation(shader->pid, "Texture0Resolution");
    }

    glUniformMatrix4fv(V_MODEL, 1, GL_FALSE, (const f32*)&cmd->meshcmd.modelm.m);
    glUniform4fv(V_UVCOLOR, 1, (const f32*)&cmd->meshcmd.color.f);
    glUniform2fv(V_UVOFFSET, 1, (const f32*)&cmd->meshcmd.uvoffset.f);
    glUniform2fv(V_UVSCALE, 1, (const f32*)&cmd->meshcmd.uvscale.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cmd->meshcmd.tx.id);
    glUniform2f(F_TEXTURE0_RESOLUTION, cmd->meshcmd.tx.width, cmd->meshcmd.tx.height);
}

static LShader GetDefaultShader() {
    // clang-format off
    static const char* VSRC = GLSL(
        layout (location = 0) in vec3 vPos;
        layout (location = 2) in vec2 vTextCoords;
        
        uniform mat4 vModel;
        uniform mat4 vView;
        uniform mat4 vProj;
        uniform vec4 vUColor;
        uniform vec2 vUVOffset;
        uniform vec2 vUVScale;
        
        out vec4 fColor;
        out vec2 fTextCoords;

        void main() {
           fTextCoords = vUVOffset + (vUVScale * vTextCoords);
           gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);
           fColor = vUColor;
        }
    );
    // clang-format on

    // clang-format off
    static const char* FSRC = GLSL(
        in vec4 fColor; in vec2 fTextCoords;

        uniform sampler2D texture0;
        uniform vec2 Texture0Resolution;
        uniform vec2 resolution;
        uniform float timer;
        uniform int NFrame;

        out vec4 FragColor;

        void main() {
            vec4 txColor = texture(texture0, fTextCoords);
            // Crucial for having depth buffer + blending enabled. when I write quads with textures with some tansparent
            // parts. We want to discard those transparent parts else those values will get written to the depth buffer.
            if (txColor.a < 0.01) {
                discard;
            }
            // For blending to work here properly, its important that either texture is specified or texture
            // is default (all black) and a uniform color is specified. also 2d needs to be drawn after 3d.
            FragColor = txColor * fColor;
        }
    );
    // clang-format on

    static bool INIT = false;
    static LShader SHADER = {0};

    if (!INIT) {
        INIT = true;
        LShaderInit(&SHADER, VSRC, FSRC);
        SHADER.enable_func = EnableDefaultShader;
        SHADER.set_uniforms_func = DefaultShaderSetUniforms;
    }

    return SHADER;
}

LCORE void EnableLineShader(LShader* shader, ShaderPass* pass) {
    static bool INIT = false;
    static i32 V_VIEW = 0;
    static i32 V_PROJ = 0;
    static i32 F_RESOLUTION = 0;
    static i32 F_TIMER = 0;
    static i32 F_NFRAME = 0;

    glUseProgram(shader->pid);

    if (!INIT) {
        INIT = true;
        V_VIEW = glGetUniformLocation(shader->pid, "vView");
        V_PROJ = glGetUniformLocation(shader->pid, "vProj");
        F_RESOLUTION = glGetUniformLocation(shader->pid, "resolution");
        F_TIMER = glGetUniformLocation(shader->pid, "timer");
        F_NFRAME = glGetUniformLocation(shader->pid, "NFrame");

        // IMPORTANT! Set texture uniform loc.
        glUniform1i(glGetUniformLocation(shader->pid, "texture0"), 0);
    }

    glUniformMatrix4fv(V_VIEW, 1, GL_FALSE, (const f32*)&pass->view.m);
    glUniformMatrix4fv(V_PROJ, 1, GL_FALSE, (const f32*)&pass->proj.m);

    glUniform2fv(F_RESOLUTION, 1, (const f32*)&pass->resolution.f);
    glUniform1f(F_TIMER, pass->timer);

    glUniform1i(F_NFRAME, pass->nframe);
}

LCORE void LineShaderSetUniforms(LShader* shader, DrawCmd* cmd) {
    static bool INIT = false;
    static i32 V_MODEL = 0;
    static i32 V_UVCOLOR = 0;
    static i32 V_UVSCALE = 0;
    static i32 V_UVOFFSET = 0;
    static i32 F_TEXTURE0_RESOLUTION = 0;

    Assert(cmd->type == CMD_DRAW_MESH);
    if (!INIT) {
        INIT = true;

        V_MODEL = glGetUniformLocation(shader->pid, "vModel");
        V_UVCOLOR = glGetUniformLocation(shader->pid, "vUColor");
        V_UVOFFSET = glGetUniformLocation(shader->pid, "vUVOffset");
        V_UVSCALE = glGetUniformLocation(shader->pid, "vUVScale");
        F_TEXTURE0_RESOLUTION = glGetUniformLocation(shader->pid, "Texture0Resolution");
    }

    glUniformMatrix4fv(V_MODEL, 1, GL_FALSE, (const f32*)&cmd->meshcmd.modelm.m);
    glUniform4fv(V_UVCOLOR, 1, (const f32*)&cmd->meshcmd.color.f);
    glUniform2fv(V_UVOFFSET, 1, (const f32*)&cmd->meshcmd.uvoffset.f);
    glUniform2fv(V_UVSCALE, 1, (const f32*)&cmd->meshcmd.uvscale.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cmd->meshcmd.tx.id);
    glUniform2f(F_TEXTURE0_RESOLUTION, cmd->meshcmd.tx.width, cmd->meshcmd.tx.height);
}

static LShader GetLineShader() {
    // clang-format off
    static const char* VSRC = GLSL(
        layout (location = 0) in vec3 vPos;
        layout (location = 2) in vec2 vTextCoords;
        
        uniform mat4 vModel;
        uniform mat4 vView;
        uniform mat4 vProj;
        uniform vec4 vUColor;
        uniform vec2 vUVOffset;
        uniform vec2 vUVScale;
        
        out vec4 fColor;
        out vec2 fTextCoords;
        out vec2 fPosLocal;

        void main() {
           fTextCoords = vUVOffset + (vUVScale * vTextCoords);
           gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);
           fColor = vUColor;
           fPosLocal = vPos.xy;
        }
    );
    // clang-format on

    // clang-format off
    static const char* FSRC = GLSL(
        in vec4 fColor;
        in vec2 fTextCoords;
        in vec2 fPosLocal;

        uniform sampler2D texture0;
        uniform vec2 Texture0Resolution;
        uniform vec2 resolution;
        uniform float timer;
        uniform int NFrame;

        out vec4 FragColor;

        void main() {
            vec4 txColor = texture(texture0, fTextCoords);
            // Crucial for having depth buffer + blending enabled. when I write quads with textures with some tansparent
            // parts. We want to discard those transparent parts else those values will get written to the depth buffer.
            if (txColor.a < 0.01) {
                discard;
            }
            // For blending to work here properly, its important that either texture is specified or texture
            // is default (all black) and a uniform color is specified. also 2d needs to be drawn after 3d.
            FragColor = txColor * fColor;

            float alpha_mask = 1.0;

            // {
            //     float u = fTextCoords.x;
            //     // aa radius. This represents the radius of a circle that determines how much alpha falloff
            //     // the line pixels experience as our u coordinate goes from inside the line to outside the line.
            //     // we are fully within the line in the range [r, 1-r] and outside the line outside of that range.
            //     // Note that this assumes that the drawn line quad is oriented such that the u coordinate of each
            //     // fragment lies along the width axis of the quad.
            //     float r = 0.3;
            //     // Construct a piecewise smoothing function that transitions smoothly from [0, 1] between the
            //     // domain of [0, r], stays at 1 from [r, 1-r] and transitions smoothly decreasing from [1, 0]
            //     // in the domain [1-r, r].
            //     alpha_mask = smoothstep(0.0, 1.0, u / r) - 
            //                     smoothstep(0.0, 1.0, (max(u - 1 + r, 0.0)) / r);
            // }

            {
                // Calculates AA in local space of the quad model. Assumes the quad has width of 0.5 in local space
                // and that we are using an orthographic projection (I think due to perspective correct interpolation of 
                // vertex attribute concerns.)
                float x = abs(fPosLocal.x);  // pass in our vertex position in local space.
                float edge = 0.5;            // Assume the quad model has edge vertex of 0.5
                float w = 2.0 * fwidth(x);         // Get size of 1 pixel in screen space
                alpha_mask *= 1.0 - smoothstep(edge - w, edge, x);  // Apply alpha blending of 1 pixel width at edge.
            
                // Do the same for AA the end edges of the line. Note that we use y for fwidth
                // which I'm not 100% sure is correct.
                float y = abs(fPosLocal.y);
                float h = fwidth(y);  // Get ~size of 1 pixel in screen space in the y direction.
                alpha_mask *= 1.0 - smoothstep(edge - h, edge, y);
            }

            FragColor.a *= alpha_mask;
        }
    );
    // clang-format on

    static bool INIT = false;
    static LShader SHADER = {0};

    if (!INIT) {
        INIT = true;
        LShaderInit(&SHADER, VSRC, FSRC);
        SHADER.enable_func = EnableLineShader;
        SHADER.set_uniforms_func = LineShaderSetUniforms;
    }

    return SHADER;
}

LCORE void EnableNoiseShader(LShader* shader, ShaderPass* pass) {
    static bool INIT = false;
    static i32 V_VIEW = 0;
    static i32 V_PROJ = 0;
    static i32 F_RESOLUTION = 0;
    static i32 F_TIMER = 0;
    static i32 F_NFRAME = 0;

    glUseProgram(shader->pid);

    if (!INIT) {
        INIT = true;
        V_VIEW = glGetUniformLocation(shader->pid, "vView");
        V_PROJ = glGetUniformLocation(shader->pid, "vProj");
        F_RESOLUTION = glGetUniformLocation(shader->pid, "resolution");
        F_TIMER = glGetUniformLocation(shader->pid, "timer");
        F_NFRAME = glGetUniformLocation(shader->pid, "NFrame");

        // IMPORTANT! Set texture uniform loc.
        glUniform1i(glGetUniformLocation(shader->pid, "texture0"), 0);
    }

    glUniformMatrix4fv(V_VIEW, 1, GL_FALSE, (const f32*)&pass->view.m);
    glUniformMatrix4fv(V_PROJ, 1, GL_FALSE, (const f32*)&pass->proj.m);

    glUniform2fv(F_RESOLUTION, 1, (const f32*)&pass->resolution.f);
    glUniform1f(F_TIMER, pass->timer);

    glUniform1i(F_NFRAME, pass->nframe);
}

LCORE void NoiseShaderSetUniforms(LShader* shader, DrawCmd* cmd) {
    static bool INIT = false;
    static i32 V_MODEL = 0;
    static i32 V_UVCOLOR = 0;
    static i32 V_UVSCALE = 0;
    static i32 V_UVOFFSET = 0;
    static i32 F_TEXTURE0_RESOLUTION = 0;

    Assert(cmd->type == CMD_DRAW_MESH);
    if (!INIT) {
        INIT = true;

        V_MODEL = glGetUniformLocation(shader->pid, "vModel");
        V_UVCOLOR = glGetUniformLocation(shader->pid, "vUColor");
        V_UVOFFSET = glGetUniformLocation(shader->pid, "vUVOffset");
        V_UVSCALE = glGetUniformLocation(shader->pid, "vUVScale");
        F_TEXTURE0_RESOLUTION = glGetUniformLocation(shader->pid, "Texture0Resolution");
    }

    glUniformMatrix4fv(V_MODEL, 1, GL_FALSE, (const f32*)&cmd->meshcmd.modelm.m);
    glUniform4fv(V_UVCOLOR, 1, (const f32*)&cmd->meshcmd.color.f);
    glUniform2fv(V_UVOFFSET, 1, (const f32*)&cmd->meshcmd.uvoffset.f);
    glUniform2fv(V_UVSCALE, 1, (const f32*)&cmd->meshcmd.uvscale.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cmd->meshcmd.tx.id);
    glUniform2f(F_TEXTURE0_RESOLUTION, cmd->meshcmd.tx.width, cmd->meshcmd.tx.height);
}

LCORE LShader GetNoiseShader() {
    // clang-format off
    static const char* VSRC = GLSL(
        layout (location = 0) in vec3 vPos;
        layout (location = 2) in vec2 vTextCoords;
        
        uniform mat4 vModel;
        uniform mat4 vView;
        uniform mat4 vProj;
        uniform vec4 vUColor;
        uniform vec2 vUVOffset;
        uniform vec2 vUVScale;
        
        out vec4 fColor;
        out vec2 fTextCoords;
        out vec3 fWorldPos;

        void main() {
           fTextCoords = vUVOffset + (vUVScale * vTextCoords);
           gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);
           fColor = vUColor;
           fWorldPos = (vModel * vec4(fWorldPos, 1.0)).xyz;
        }
    );
    // clang-format on

    // clang-format off
    static const char* FSRC = GLSL(
        in vec4 fColor; 
        in vec2 fTextCoords;
        in vec3 fWorldPos;

        uniform sampler2D texture0;
        uniform vec2 Texture0Resolution;
        uniform vec2 resolution;
        uniform float timer;
        uniform int NFrame;

        out vec4 FragColor;

        // https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
        uint pcg_hash(uint x) {
            uint state = x * 747796405u + 2891336453u;
            uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
            return (word >> 22u) ^ word;
        }

        float uv2whitenoise(vec2 uv, ivec2 dims) {
            ivec2 texel = ivec2(floor(uv * dims));
            uint seed = uint((texel.x * dims.y) + texel.y);
            return float(pcg_hash(seed)) * (1.0 / 4294967295.0);
        }

        float whitenoise(vec3 value) {
            // Produce a random value by dotting it with a random direction vector.
            // The resulting random value will be how close our input vector is to the direciton vector
            // multiplied by magnitudes.
            float random = dot(value, vec3(2.971, 49.149, 0));
            // Random could be a large number since our vectors are assumed to be unit vectors
            // clamp to -1 to 1 range with sin function. This also gives our random value a wave like
            // property. then to avoid tiling, multiply with a very large random number to essentially
            // convert our small number to range [0, large number] which should eliminate tiling.
            return fract(sin(random) * 143758.5453);
        }

        void main() {
            vec4 txColor = texture(texture0, fTextCoords);
            // Crucial for having depth buffer + blending enabled. when I write quads with textures with some tansparent
            // parts. We want to discard those transparent parts else those values will get written to the depth buffer.
            if (txColor.a < 0.01) {
                discard;
            }
            // For blending to work here properly, its important that either texture is specified or texture
            // is default (all black) and a uniform color is specified. also 2d needs to be drawn after 3d.
            FragColor = txColor * fColor;
            float noise = whitenoise(vec3(fTextCoords, 1.0));
            // Remember cant use texture dims as some textures are 1 pixel.
            float noisehash = uv2whitenoise(fTextCoords, ivec2(10000, 10000));
            FragColor = vec4(vec3(noisehash), 1.0);
            if (noisehash == 0.0) {
                FragColor = vec4(1, 0, 0, 1);
            }
        }
    );
    // clang-format on

    static bool INIT = false;
    static LShader SHADER = {0};

    if (!INIT) {
        INIT = true;
        LShaderInit(&SHADER, VSRC, FSRC);
        SHADER.enable_func = EnableNoiseShader;
        SHADER.set_uniforms_func = NoiseShaderSetUniforms;
    }

    return SHADER;
}

LCORE void EnableEffectsShader(LShader* shader, ShaderPass* pass) {
    static bool INIT = false;
    static i32 V_VIEW = 0;
    static i32 V_PROJ = 0;

    glUseProgram(shader->pid);

    if (!INIT) {
        INIT = true;
        V_VIEW = glGetUniformLocation(shader->pid, "vView");
        V_PROJ = glGetUniformLocation(shader->pid, "vProj");

        // IMPORTANT! Set texture uniform loc.
        glUniform1i(glGetUniformLocation(shader->pid, "texture0"), 0);
    }

    glUniformMatrix4fv(V_VIEW, 1, GL_FALSE, (const f32*)&pass->view.m);
    glUniformMatrix4fv(V_PROJ, 1, GL_FALSE, (const f32*)&pass->proj.m);
}

LCORE void EffectsShaderSetUniforms(LShader* shader, DrawCmd* cmd) {
    static bool INIT = false;
    static i32 V_MODEL = 0;
    static i32 V_UVCOLOR = 0;
    static i32 V_UVSCALE = 0;
    static i32 V_UVOFFSET = 0;

    Assert(cmd->type == CMD_DRAW_MESH);
    if (!INIT) {
        INIT = true;

        V_MODEL = glGetUniformLocation(shader->pid, "vModel");
        V_UVCOLOR = glGetUniformLocation(shader->pid, "vUColor");
        V_UVOFFSET = glGetUniformLocation(shader->pid, "vUVOffset");
        V_UVSCALE = glGetUniformLocation(shader->pid, "vUVScale");
    }

    glUniformMatrix4fv(V_MODEL, 1, GL_FALSE, (const f32*)&cmd->meshcmd.modelm.m);
    glUniform4fv(V_UVCOLOR, 1, (const f32*)&cmd->meshcmd.color.f);
    glUniform2fv(V_UVOFFSET, 1, (const f32*)&cmd->meshcmd.uvoffset.f);
    glUniform2fv(V_UVSCALE, 1, (const f32*)&cmd->meshcmd.uvscale.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cmd->meshcmd.tx.id);
}

static LShader GetEffectsShader() {
    // clang-format off
    static const char* VSRC = GLSL(
        layout (location = 0) in vec3 vPos;
        layout (location = 2) in vec2 vTextCoords;
        
        uniform mat4 vModel;
        uniform mat4 vView;
        uniform mat4 vProj;
        
        uniform vec4 vUColor;
        uniform vec2 vUVOffset;
        uniform vec2 vUVScale;
        
        out vec4 fColor;
        out vec2 fTextCoords;

        void main() {
           fTextCoords = vUVOffset + (vUVScale * vTextCoords);
           gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);
           fColor = vUColor;
        }
    );
    // clang-format on

    // clang-format off
    static const char* FSRC = GLSL(
        in vec4 fColor;
        in vec2 fTextCoords;

        uniform sampler2D texture0;
        
        out vec4 FragColor;
        
        vec2 uvremap(vec2 uv) {
            // as we near the edge of our screen apply greater distortion using a cubic function
            vec2 curvature = vec2(7.0, 7.0);
            uv = uv * 2.0 - 1.0;
            vec2 offset = abs(uv.yx) / vec2(curvature.x, curvature.y);
            uv = uv + uv * offset * offset;
            uv = uv * 0.5 + 0.5;
            return uv;
        }
        
        vec4 scanLineIntensity(float uv, float resolution, float opacity) {
            float PI = 3.1415926535;
            float intensity = sin(uv * resolution * PI * 2.0);
            intensity = ((0.5 * intensity) + 0.5) * 0.9 + 0.1;
            return vec4(vec3(pow(intensity, opacity)), 1.0);
        }
        
        void main() {
           vec2 uv = fTextCoords;
           vec4 txcolor = texture(texture0, uv);
           FragColor = txcolor;
        }
    );
    // clang-format on

    static bool INIT = false;
    static LShader SHADER = {0};

    if (!INIT) {
        INIT = true;
        LShaderInit(&SHADER, VSRC, FSRC);
        SHADER.enable_func = EnableEffectsShader;
        SHADER.set_uniforms_func = EffectsShaderSetUniforms;
    }

    return SHADER;
}

static void ShaderPassInit(ShaderPass* pass, LShader shader) {
    memset(pass, 0, sizeof(ShaderPass));
    pass->shader = shader;
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
    LShader shader = pass->shader;
    Assert(shader.enable_func != NULL);
    Assert(shader.set_uniforms_func != NULL);

    shader.enable_func(&shader, pass);

    for (u32 i = 0; i < pass->clen; ++i) {
        DrawCmd* cmd = &pass->cmds[i];
        switch (cmd->type) {
            case CMD_DRAW_MESH: {
                shader.set_uniforms_func(&shader, cmd);

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
}

#define MAX_SHADER_PASSES 10
static const u32 PASSNULL = MAX_SHADER_PASSES;

typedef struct {
    u32 passidx;
    u32 passlen;
    u32 passcap;
    ShaderPass passes[MAX_SHADER_PASSES];

    f32 dt;
    f32 timer;
    i32 nframe;

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
static void BeginRender(f32 dt) {
    RenderState* rstate = &RSTATE;
    rstate->passidx = PASSNULL;
    rstate->passlen = 0;
    rstate->passcap = MAX_SHADER_PASSES - 1;  // last entry reserved for null idx.
    rstate->dt = dt;
    rstate->timer += dt;
    ++rstate->nframe;
}

static ShaderPass* BeginShaderPass(LShader shader) {
    RenderState* rstate = &RSTATE;
    Assert(rstate->passlen < rstate->passcap);
    Assert(rstate->passidx == PASSNULL);

    rstate->passidx = rstate->passlen++;
    ShaderPass* pass = &rstate->passes[rstate->passidx];
    ShaderPassInit(pass, shader);
    pass->resolution = Vec2Lit(rstate->target.width, rstate->target.height);
    pass->dt = rstate->dt;
    pass->timer = rstate->timer;
    pass->nframe = rstate->nframe;
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
        // Render draw commands to render target.
        glBindFramebuffer(GL_FRAMEBUFFER, rstate->target.fbid);
        // IMPORTANT.
        glViewport(0, 0, rstate->target.width, rstate->target.height);
        // I forget exactly why we want depth testing. I think maybe has to do with not drawing portions of
        // sprites that are blank? maybe thats just alpha blending.
        glEnable(GL_DEPTH_TEST);
        // Pass depth test if incomming_depth <= stored_depth. This is necessary to ensure we still draw
        // sprites which have the same z value which so far seems like a desirable thing.
        glDepthFunc(GL_LEQUAL);

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
        glViewport(0, 0, PlatformScreenWidth(), PlatformScreenHeight());
        glDisable(GL_DEPTH_TEST);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ShaderPass pass;
        ShaderPassInit(&pass, GetEffectsShader());
        pass.dt = rstate->dt;
        pass.timer = rstate->timer;
        pass.resolution = Vec2Lit(PlatformScreenWidth(), PlatformScreenHeight());
        DrawCmd* cmd = ShaderPassAddCmd(&pass);
        DrawMeshCmdInit(cmd);
        cmd->meshcmd.meshid = GetMesh(M_QUAD);
        // Since we only set a scale matrix, essentially we are specifying this quad in clip space. since the quad
        // is from -0.5, 0.5, we mult by 2 to get -1, 1 so we cover the entire screen.
        cmd->meshcmd.modelm = Mat4ScaleV(Vec3Lit(2.0f, 2.0f, 1.0f));
        // cmd->meshcmd.color = LRED;  // For debug.
        cmd->meshcmd.tx.id = rstate->target.txid;
        cmd->meshcmd.tx.width = rstate->target.width;
        cmd->meshcmd.tx.height = rstate->target.height;
        ShaderPassExecute(&pass);
    }
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

LCORE DrawCmd* ClearBuffer(Vec4 color) {
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

// Note assumes drawing in 2d. the z is just for depth ordering.
// We assume that the quad's up direction is oriented parallel to the line segment.
LCORE DrawCmd* DrawLine(Vec3 start, Vec3 end, f32 width, Vec4 color) {
    // Assumption here is that we are applying anti aliasing in frag shader. That AA algo requires some
    // area of the line quad to apply smoothing, so lets just bump the size of the requested line by
    // that area amount to give us some buffer. This does put a cap on the smallest line we can draw.
    // It maybe makes sense to make this slightly bigger than the aa radius.
    static const f32 AA_BUFFER_X = 3.0;

    Assert(start.z == end.z);
    Vec3 pos = Vec3Lerp(start, end, 0.5f);
    Vec3 span = Vec3Sub(end, start);
    Vec3 dir = Vec3Normalize(span);

    // Remember we aren't computing a rotation (which would be some displacement difference
    // between identity orient and new orient), we are just computing an orientation.
    // The direction of line is our up
    Vec3 up = dir;
    // Compute right basis vector by taking 2d normal (since z is assumed for depth only).
    Vec3 right = Vec3Lit(dir.y, -dir.x, dir.z);
    Vec3 forward = Vec3Lit(0.0f, 0.0f, 1.0f);

    Mat4 rot = Mat4Identity();
    rot.v.a = right;
    rot.v.b = up;
    rot.v.c = forward;

    f32 height = Vec3Len(span);
    return DrawQuad(pos, width + AA_BUFFER_X, height, rot, color);
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
    cmd->meshcmd.tx = tx;
    cmd->meshcmd.meshid = GetMesh(M_QUAD);
}

// Remember
// src == where to draw in texture space
// dest == where to draw in world space.
// (0, 0)
// * maps to bottom left corner in src texture space (uv space)
// * maps to origin in world space.
// :c_src - source origin (lower left corner) of where to read from texture in uv space
// :r_src - full width/height of sub rect to read from texture in uv space
// :c_dst - destination origin (center of quad) of where to draw in world space
// :r_dst - full width + height of quad
LCORE DrawCmd* DrawLTextureProUV(LTexture tx, Vec2 c_src, Vec2 r_src, Vec2 c_dst, Vec2 r_dst, f32 rotation,
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
    cmd->meshcmd.tx = tx;
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
            Vec3 r_bb = Vec3Transform(Vec2ToVec3(shape.aabb2d.r), m);
            shape.aabb2d.r = Vec2Lit(r_bb.x, r_bb.y);
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
//             PlatformLog(LLOG_ERROR,
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
        DrawLTextureProUV(font.tx, quvoffset, quvdims, qorigin, qdims, 0.0f, color);
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

LCORE void DrawLTextF(Vec2 pos, Vec4 color, const char* format, ...) {
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
    SOUND_COUNT,
} SoundTag;

static SoundId SOUNDS[SOUND_COUNT] = {0};

#define GetSound(sid) (SOUNDS[(sid)])

static void LoadSounds() {
    static bool INIT = false;
    if (INIT) {
        return;
    }
    INIT = true;
    SOUNDS[SOUND_NONE] = 0;
    SOUNDS[SOUND_BEEP] = PlatformLoadSound("assets/sounds/beep.wav");
}

////////////////////////////////////////////////////////////
typedef enum {
    PF_NONE = 1 << 0,
    PF_ACTIVE = 1 << 1,
    PF_GRAVITY = 1 << 2,
} ParticleFlag;

typedef struct {
    ParticleFlag flags;

    Vec3 p;
    Vec3 v;
    Vec3 scale;
    Vec4 color;
    Mat4 orient;

    f32 lifetime;
    f32 lifedt;
    // For the first x% of the lifetime of the particle. linearly fade from transparent to color.alpha.
    f32 fade_in_pct;
    // For the last x% of the lifetime of the particle. linearly fade from color.alpha to transparent.
    // By default set to 1.0 for the full range.
    f32 fade_out_pct;
    // For the first x% of lifetime, scale in from 0 to full size.
    f32 scale_in_pct;
    // For the last x% of lifetime, scale out from full size to 0.
    f32 scale_out_pct;

} Particle;

LCORE void ParticleSetFadeIn(Particle* particle, f32 fade_in_pct) {
    particle->fade_in_pct = fade_in_pct;
    f32 rem = 1.0f - (particle->fade_in_pct + particle->fade_out_pct);
    particle->fade_out_pct += rem < 0.0f ? rem : 0.0f;
}

LCORE void ParticleSetFadeOut(Particle* particle, f32 fade_out_pct) {
    particle->fade_out_pct = fade_out_pct;
    f32 rem = 1.0f - (particle->fade_out_pct + particle->fade_out_pct);
    particle->fade_in_pct += rem < 0.0f ? rem : 0.0f;
}

#define MAX_PARTICLES 512

typedef struct {
    Particle particles[MAX_PARTICLES];
    u32 pcap;
} ParticleSystem;

static ParticleSystem PARTICLES;

#define GetParticleSystem() (&PARTICLES)

static void ParticleSystemInit() {
    ParticleSystem* particles = GetParticleSystem();
    ParticleSystem zero = {0};
    *particles = zero;
    particles->pcap = MAX_PARTICLES;
}

LCORE Particle* AllocateParticle() {
    ParticleSystem* particles = GetParticleSystem();
    Particle* particle = NULL;
    for (i32 i = 0; i < particles->pcap; ++i) {
        if (!TestFlag(particles->particles[i].flags, PF_ACTIVE)) {
            particle = &particles->particles[i];
            break;
        }
    }
    Assert(particle != NULL);
    memset(particle, 0, sizeof(Particle));
    particle->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
    particle->orient = Mat4Identity();
    particle->fade_out_pct = 1.0f;
    FlagOn(particle->flags, PF_ACTIVE);
    return particle;
}

LCORE Particle* SpawnParticle(Vec3 p, Vec2 dims, f32 lifetime, RGBA color) {
    Assert(lifetime > 0.0f);
    Particle* particle = AllocateParticle();
    particle->p = p;
    particle->scale = Vec3Lit(dims.x, dims.y, 1.0f);
    particle->lifetime = lifetime;
    particle->color = color;
    return particle;
}

LCORE void ParticleSystemTick(f32 dt) {
    ParticleSystem* particles = GetParticleSystem();
    for (u32 i = 0; i < particles->pcap; ++i) {
        Particle* particle = &particles->particles[i];
        particle->lifedt += dt;
        if (particle->lifedt >= particle->lifetime) {
            FlagOff(particle->flags, PF_ACTIVE);
            continue;
        }

        if (TestFlag(particle->flags, PF_GRAVITY)) {
            Vec3 G = Vec3Lit(0.0f, -350.0f, 0.0f);
            particle->v = Vec3Add(particle->v, Vec3Scale(G, dt));
        }
        particle->p = Vec3Add(particle->p, Vec3Scale(particle->v, dt));
        // particle->color.a = f32lerp(1.0f, 0.1f, particle->lifedt / particle->lifetime);

        // Find % along lifetime we are.
        f32 lifedt_pct = f32lerp_pct(0.0f, particle->lifetime, particle->lifedt);

        f32 alpha_mask = 1.0f;
        if (particle->fade_in_pct != 0.0f) {
            // Linearly fade in if we are within the fade in window by the percent we are in the fade in window.
            // Remember that multiplying here is essentially our lerp because lerp(0, e, x) = e * x. X is
            // already a percentage in range 0..1 because of f32lerp_pct.
            alpha_mask *= f32lerp_pct(0.0f, particle->fade_in_pct, lifedt_pct);
        }
        // Determine the % of the way through we are through the fade out window. Since our fade out window
        // is given in percent of the ending (i.e. relative). We need to subtract 100% - fadeout pct to get
        // absolute percent. then we can determine percent we are through using abs percent. Need to do
        // 1.0f - lerp pct because we are fading out not in.
        // Because we are fading out f32lerp(color.a, 0.0f, x) this reduces to (1.0f - x) * color.a
        // Which reduces to alpha *= 1.0f - x where x is percentage way through fade out window.
        alpha_mask *= 1.0f - f32lerp_pct(1.0f - particle->fade_out_pct, 1.0f, lifedt_pct);
        RGBA color = particle->color;
        color.a *= alpha_mask;

        f32 scale_mask = 1.0f;
        if (particle->scale_in_pct != 0.0f) {
            scale_mask *= f32lerp_pct(0.0f, particle->scale_in_pct, lifedt_pct);
        }
        if (particle->scale_out_pct != 0.0f) {
            scale_mask *= 1.0f - f32lerp_pct(1.0f - particle->scale_out_pct, 1.0f, lifedt_pct);
        }
        Vec3 scale = Vec3Scale(particle->scale, scale_mask);

        DrawQuad(particle->p, scale.x, scale.y, particle->orient, color);
        // DrawCmd* cmd = AddDrawMeshCmd();
        // cmd->meshcmd.modelm = Mat4SVRT(particle->scale, particle->orient, particle->p);
        // cmd->meshcmd.meshid = GetMesh(M_QUAD);
        // cmd->meshcmd.color = particle->color;
    }
}

////////////////////////////////////////////////////////////

// I assume this will mostly be used for prototyping and debugging since entities will have dynamic positions.
LCORE DrawCmd* DrawSprite(Sprite sprite, Vec3 pos, Mat4 orient, Vec3 scale) {
    // By default scale of 1 will mean draw the sprite with same width/height
    DrawCmd* cmd = DrawQuad(pos, sprite.dims.x * scale.x, sprite.dims.y * scale.y, orient, LCLEAR);
    cmd->meshcmd.tx = sprite.tx;
    cmd->meshcmd.uvoffset = sprite.minuv;
    cmd->meshcmd.uvscale = sprite.dimsuv;
    return cmd;
}

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

// A model is basically a "static" set of geometry and visual info data.
// We can load a model that has been created and durably stored a priori once and then
// just refer to it in many places (e.g. load the idle state model and swap out with jump state model).
typedef struct {
    // The underlying geometry info.
    // These transform fields are "static", i.e. they are just set once on model load.
    // Allows for easy persistent transform (i.e. load this model rotated by 90 degrees).
    Vec3 pos;
    Mat4 orient;
    Vec3 scale;
    Vec4 tint;  // Color.
    LMesh mesh;

    // This is like the material info basically. How the model "looks"
    // Unclear where custom shader info will need to go, maybe on the graphics state?
    Animation animation;
} LModel;

LCORE LModel LModelFromAnimation(Animation animation, Vec3 pos, Mat4 orient, Vec3 scale, Vec4 tint) {
    LModel model = {
        .pos = pos,
        .orient = orient,
        // By default init the model scale to match the pixel dimensions of the bb of the first frame
        // of the animation.
        .scale = scale,
        .tint = tint,
        .mesh = GetMesh(M_QUAD),
        .animation = animation,
    };
    return model;
}

LCORE LModel LModelFromSprite(Sprite sprite) {
    Animation animation = AnimationFromSprite(sprite);
    return LModelFromAnimation(animation, Vec3Zero(), Mat4Identity(), Vec3LitAll(1.0f), LCLEAR);
}

// Draw an lmodel animation for a specific frame. You can use this to animate over time.
LCORE DrawCmd* DrawLModelEx(LModel model, isize frame_idx, Vec3 pos, Mat4 orient, Vec3 scale) {
    Assert(model.animation.sprites.len > frame_idx);
    // Always draw the sprite at a base width/height that matches the original sprite. Then we modify by the
    // static scale of the model + any additional user scale here.
    Sprite* sprite = ArrGetR(&model.animation.sprites, frame_idx);
    f32 scalex = scale.x * model.scale.x * sprite->dims.x;
    f32 scaley = scale.y * model.scale.y * sprite->dims.y;
    DrawCmd* cmd = DrawQuad(Vec3Add(pos, model.pos), scalex, scaley, Mat4Multiply(orient, model.orient), LCLEAR);
    // Draw first frame of animation (if its an animation, else same as Drawsprite).
    cmd->meshcmd.tx = sprite->tx;
    cmd->meshcmd.uvoffset = sprite->minuv;
    cmd->meshcmd.uvscale = sprite->dimsuv;
    return cmd;
}

// I assume this will mostly be used for prototyping and debugging since entities will have dynamic positions.
LCORE DrawCmd* DrawLModel(LModel model, Vec3 pos, Mat4 orient, Vec3 scale) {
    return DrawLModelEx(model, 0, pos, orient, scale);
}

typedef struct {
    Vec3 pos;
    Vec3 scale;
    Mat4 orient;

    Vec3 posa;
    Vec3 posb;
    f32 posdt;

    LModel model;
    isize frame_idx;
    f32 elapsed;
} GraphicsInfo;

static void GraphicsInfoMemset(GraphicsInfo* graphics) {
    memset(graphics, 0, sizeof(GraphicsInfo));
    graphics->orient = Mat4Identity();
    graphics->pos = Vec3Zero();
    graphics->scale = Vec3Lit(1.0f, 1.0f, 1.0f);

    graphics->model = LModelFromAnimation(DefaultAnimation(), Vec3Zero(), Mat4Identity(), Vec3LitAll(1.0f), LCLEAR);
}

// #define GraphicsInfoSetLModel(graphics, m) ((graphics)->model = (m))

LCORE void DrawGraphicsInfo(GraphicsInfo* graphics) {
    DrawLModelEx(graphics->model, graphics->frame_idx, graphics->pos, graphics->orient, graphics->scale);
}

// // min - bottom left hand corner of sprite (in png space, not uvspace)
// // dims - full width/height of sprite in png space not uvspace.
// LCORE void GraphicsInfoSetSprite(GraphicsInfo* graphics, LTexture tx, Vec2 min, Vec2 dims, f32 orient, f32 scale) {
//     graphics->scale = Vec3Lit(dims.x * scale, dims.y * scale, 1.0f);
//     graphics->orient = Mat4Rotate(orient, Vec3Lit(0.0f, 0.0f, 1.0f));

//     graphics->mid = GetMesh(M_QUAD);

//     TextureInfo res = {0};
//     res.tx = tx;
//     res.type = TI_SPRITE;
//     res.sprite.minuv = Vec2Lit(min.x / tx.width, min.y / tx.height);
//     res.sprite.dimsuv = Vec2Lit(dims.x / tx.width, dims.y / tx.height);
//     res.tint = LCLEAR;
//     graphics->txinfo = res;
// }

// // Remember rect coords are in aseprite space.
// // TODO: This feels inefficient. Feels like what I really want is to be able to save visual info loaded from aseprite
// // once as a constant and set an entities' visuals to the constant. problem is that I can't save entire graphics info
// // or else when I set entity graphics, I overwrite position. So I need some sort of notion for the "static" entity
// // data.
// // NOTE: We don't overwrite all graphics info data because we don't set GraphicsInfoMemset, so data like position
// // is preserved.
// LCORE void GraphicsInfoSetSpriteFromAseprite(GraphicsInfo* graphics, LTexture tx, AseRect src, f32 orient, f32 scale)
// {
//     graphics->scale = Vec3Lit(src.r.x * scale, src.r.y * scale, 1.0f);
//     graphics->orient = Mat4Rotate(orient, Vec3Lit(0.0f, 0.0f, 1.0f));

//     graphics->mid = GetMesh(M_QUAD);

//     src.lc = AsepritePosToFlipped(tx, src.lc);

//     TextureInfo res = {0};
//     res.tx = tx;
//     res.type = TI_SPRITE;
//     res.sprite.minuv = Vec2Lit(src.lc.x / tx.width, src.lc.y / tx.height);
//     res.sprite.dimsuv = Vec2Lit(src.r.x / tx.width, src.r.y / tx.height);
//     res.tint = LCLEAR;
//     graphics->txinfo = res;
// }

// LCORE void GraphicsInfoSetAnimationFromAseprite(GraphicsInfo* graphics, LTexture tx, AnimationAseRect animation,
//                                                 f32 orient, f32 scale) {
//     graphics->scale = Vec3Lit(animation.start.r.x * scale, animation.start.r.y * scale, 1.0f);
//     graphics->orient = Mat4Rotate(orient, Vec3Lit(0.0f, 0.0f, 1.0f));

//     graphics->mid = GetMesh(M_QUAD);

//     animation.start.lc = AsepritePosToFlipped(tx, animation.start.lc);

//     f32 width_inv = 1.0f / tx.width;
//     f32 height_inv = 1.0f / tx.height;
//     TextureInfo txinfo = {0};
//     txinfo.tx = tx;
//     txinfo.type = TI_ANIMATION;
//     txinfo.animation.minuv = Vec2Lit(animation.start.lc.x * width_inv, animation.start.lc.y * height_inv);
//     txinfo.animation.dimsuv = Vec2Lit(animation.start.r.x * width_inv, animation.start.r.y * height_inv);
//     txinfo.animation.stepuv = Vec2Lit(animation.step.x * width_inv, animation.step.y * height_inv);
//     txinfo.animation.nframes = animation.nframes;
//     txinfo.animation.frame = 0;
//     txinfo.animation.frame_dt = 0.0f;
//     txinfo.tint = LCLEAR;

//     graphics->txinfo = txinfo;
// }

// LCORE void GraphicsInfoSetTexture(GraphicsInfo* graphics, LTexture tx, f32 orient, f32 scale) {
//     GraphicsInfoSetSprite(graphics, tx, Vec2Zero(), Vec2Lit(tx.width, tx.height), orient, scale);
// }

// LCORE void GraphicsInfoSetShape(GraphicsInfo* graphics, LMesh mesh, Vec3 scale, Vec4 color) {
//     graphics->mid = mesh;
//     graphics->scale = scale;
//     graphics->txinfo.tint = color;
// }

////////////////////////////////////////////////////////////

static const Vec3 ORIGIN = Vec3Lit(0.0f, 0.0f, 0.0f);
static const Vec2 ORIGIN2D = Vec2Lit(0.0f, 0.0f);

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
    EFLAG_SURVIVE_HIT = 1 << 11,
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

typedef struct {
    Vec3 eye;     // world space position of the camera.
    Vec3 orient;  // vector containing euler angle rotation vector. for identity rotation, set to Vec3Zero
} LCamera;

// static Mat4 LCameraGetView(LCamera c) {
//     return Mat4Invert(Mat4Multiply(Mat4TranslateV(c.eye), Mat4RotateEuler(c.orient)));
// }

LCORE Mat4 GetCameraMat(Vec3 eye, Vec3 orient) {
    return Mat4Invert(Mat4Multiply(Mat4TranslateV(eye), Mat4RotateEuler(orient)));
}

#define Grid2WorldEx(pos, w, h, d) (Vec3Lit((pos).x * (w), (pos).y * (h), (pos).z * (d)))

static const f32 GRID_WIDTH = 16.0f;
static const f32 GRID_HEIGHT = 16.0f;
static const f32 GRID_DEPTH = 1.0f;
#define Grid2World(pos) (Grid2WorldEx(pos, GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH))

#define World2GridEx(pos, w, h, d) (Vec3Lit((u32)((pos).x / (w)), (u32)((pos).y / (h)), (u32)((pos).z) / (d)))
#define World2Grid(pos) (World2GridEx(pos, GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH))

////////////////////////////////////////////////////

#define MAX_SCENES 10

typedef enum {
    SCENE_DEFAULT = 0,
    SCENE_NOISE,
    SCENE_LINE,
    SCENE_VECTOR,
    SCENE_COUNT,
} SceneId;

typedef struct GameState {
    /////////////////////////////////////////
    // Core fields
    Arena perm;
    Arena temp;

    bool debug;
    bool update;
    bool game_over;
    bool init;  // True on first frame of scene, tells scene to run any init code.

    // TODO: Move this to RenderState and move a reference to RSTATE here. Or just get rid of this and have a
    // GetScreen/RenderWidth() function that reads RSTATE.
    u32 renderw;
    u32 renderh;
    u32 screenw;
    u32 screenh;

    bool debug_ever;

    f32 dt;  // frame delta time.
    f32 timer;
    u32 nframe;            // num frame we are on.
    f32 frame_times[256];  // The incremental dts of each frame produced. its a ring buffer. used for fps count.
    i32 fts_tail;
    i32 fts_head;
    u32 fps;

    void (*scenefuncs[MAX_SCENES])(struct GameState*);
    u32 scenefuncssize;
    SceneId sceneid;
    SceneId nextsceneid;

    /////////////////////////////////////////
    // Scene fields
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

    // Signal to scene update func to run init.
    gstate->init = true;
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
    SS_COUNT,
} SceneState;

static const Vec2 GRIDR = {.x = GRID_WIDTH, .y = GRID_HEIGHT};

typedef enum {
    PS_NONE = 0,
    PS_COUNT,
} PlayerState;

static void SceneDefaultTick(GameState* gstate) {
    BeginRender(gstate->dt);

    ShaderPass* pass = BeginShaderPass(GetDefaultShader());
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

static void SceneNoiseTick(GameState* gstate) {
    static bool ROTATE = false;
    static LTexture WHITENOISE_TX = {0};
    static LTexture VALUENOISE_TX = {0};

    if (gstate->init) {
        Arena temp = TempArena(gstate->temp);
        WHITENOISE_TX = GenerateWhitenoiseTexture(&temp, 256, 256);
        VALUENOISE_TX = GenValueNoiseTexture(&temp, 256, 256, 3);
    }

    if (PlatformKeyPressed(LKEY_ENTER)) {
        ROTATE = !ROTATE;
    }

    BeginRender(gstate->dt);

    ShaderPass* pass = BeginShaderPass(GetDefaultShader());
    u32 width = gstate->renderw;
    u32 height = gstate->renderh;
    pass->proj = Mat4Ortho(-0.5f * width, 0.5f * width, -0.5f * height, 0.5f * height, -1.0f, 1.0f);

    // DrawCmd* cmd = DrawQuad(Vec3Zero(), 150.0f, 150.0f, rot, LCLEAR);
    // cmd->meshcmd.tx = TEXTURE;
    DrawLTexture(WHITENOISE_TX, Vec2Lit(-250.0f, 0.0f), 1.0f);
    DrawLTexture(VALUENOISE_TX, Vec2Lit(250.0f, 0.0f), 1.0f);

    EndShaderPass();

    EndRender();
}

static void SceneLineTick(GameState* gstate) {
    if (gstate->init) {
    }

    BeginRender(gstate->dt);

    ShaderPass* pass = BeginShaderPass(GetLineShader());
    u32 width = gstate->renderw;
    u32 height = gstate->renderh;
    pass->proj = Mat4Ortho(-0.5f * width, 0.5f * width, -0.5f * height, 0.5f * height, -1.0f, 1.0f);

    Vec3 dir = Vec3RotateAxis(Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f), gstate->timer);

    DrawLine(Vec3Lit(0.0f, 0.0f, 0.0f), Vec3Scale(dir, 100.0f), 5.0f, LRED);

    // DrawLine(Vec3Lit(-10.0f, -70.0f, 0.0f), Vec3Lit(30.0f, -80.0f, 0.0f), 5.0f, LBLUE);

    EndShaderPass();

    EndRender();
}

typedef enum {
    MS_IDLE = 0,
    MS_WALK,
    MS_JUMP,
} MoveState;

static void SceneVectorTick(GameState* gstate) {
    static Vec3 POS = {0};
    static f32 ROT = 0.0f;
    static LTexture TX_PACKED = {0};
    // static Sprite SPRITE_WIZARD = {0};
    // static Sprite SPRITE_MM = {0};
    static Sprite SPRITE_WIZARD2 = {0};
    static Animation IDLE_ANIMATION = {0};
    static Animation WALK_ANIMATION = {0};
    static Animation JUMP_ANIMATION = {0};
    static bool IDLE = true;
    static bool WALK = false;
    static bool JUMP = false;
    static MoveState MOVESTATE = MS_IDLE;
    static Animation ANIMATION = {0};
    static isize ANIMATION_IDX = 0;
    static f32 ANIMATION_PCT = 0.0f;
    static bool ANIMATION_CYCLE = true;

    if (gstate->init) {
        Arena temp = TempArena(gstate->temp);

        IDLE_ANIMATION = AnimationFromDisk("assets/entities/wizard/idle_animation", "Chara - BlueIdle", 1.0f);
        WALK_ANIMATION = AnimationFromDisk("assets/entities/wizard/walk_animation", "Chara_BlueWalk", 1.0f);
        JUMP_ANIMATION = AnimationFromDisk("assets/entities/wizard/jump_animation", "CharaWizardJump_", 0.1f);

        ANIMATION = IDLE_ANIMATION;

        // Pack texture.
        {
            isize width = 1024;
            isize height = 1024;
            RGBA256* data = New(&temp, width * height, RGBA256);

            LTexture txs[] = {
                GetTexture(TX_BLUE_WIZARD),
                GetTexture(TX_MEGAMAN2),
                GetTexture(TX_BLUE_WIZARD),
            };
            isize txslen = ArraySizeOf(txs);

            Sprite sprites[3] = {0};

            // Basic packing. Very inefficient. Optimize later.
            Vec2 offset = Vec2Zero();
            isize max_y = 0;
            for (isize i = 0; i < txslen; ++i) {
                LTexture tx = txs[i];

                isize rem_x = width - offset.x;
                if (rem_x < tx.width) {
                    // If we don't have enough width, then move up to our max height seen so far.
                    offset.x = 0;
                    offset.y = max_y;
                }

                // Update max height, assert we have headroom to write.
                max_y = f32max(max_y, offset.y + tx.height);
                isize rem_y = height - max_y;
                Assert(rem_y >= 0);

                // Copy the texture over.
                for (isize x = 0; x < tx.width; ++x) {
                    for (isize y = 0; y < tx.height; ++y) {
                        RGBA256 rgba = LTextureGetPx(tx, x, y);

                        isize offsetx = offset.x + x;
                        isize offsety = offset.y + y;
                        data[(width * offsety) + offsetx] = rgba;
                    }
                }

                sprites[i] = SpriteFromMinMax(tx, offset, Vec2Add(offset, Vec2Lit(tx.width - 1, tx.height - 1)));

                // Move along the row.
                offset.x += tx.width;
            }

            TX_PACKED = TextureFromBuffer((byte*)data, width, height, FILTER_LINEAR);
            // TODO: Remove this.
            sprites[0].tx = TX_PACKED;
            sprites[1].tx = TX_PACKED;
            sprites[2].tx = TX_PACKED;
            // SPRITE_WIZARD = sprites[0];
            // SPRITE_MM = sprites[1];
            SPRITE_WIZARD2 = sprites[2];
        }
    }

    f32 pct_delta = 0.0f;

    MoveState movestate_next = MOVESTATE;

    bool moved = PlatformKeyDown(LKEY_UP) || PlatformKeyDown(LKEY_DOWN) || PlatformKeyDown(LKEY_LEFT) ||
                 PlatformKeyDown(LKEY_RIGHT);
    bool jumped = PlatformKeyPressed(LKEY_A);

    switch (MOVESTATE) {
        case MS_IDLE: {
            if (moved) {
                ANIMATION = WALK_ANIMATION;
                ANIMATION_CYCLE = true;
                ANIMATION_IDX = 0;
                ANIMATION_PCT = 0.0f;
                movestate_next = MS_WALK;
            } else if (jumped) {
                ANIMATION = JUMP_ANIMATION;
                ANIMATION_CYCLE = false;
                ANIMATION_IDX = 0;
                ANIMATION_PCT = 0.0f;
                movestate_next = MS_JUMP;
            } else {
                if (PlatformKeyDown(LKEY_SPACE)) {
                    ROT += Deg2Rad(180.0f) * gstate->dt;
                }

                if (!moved) {
                    pct_delta = gstate->dt / (3.0f * (1 / 60.0f));
                }
            }
            break;
        }
        case MS_WALK: {
            if (moved) {
                Vec3 v = Vec3Zero();
                if (PlatformKeyDown(LKEY_UP)) {
                    v.y += 1.0f;
                }
                if (PlatformKeyDown(LKEY_DOWN)) {
                    v.y -= 1.0f;
                }
                if (PlatformKeyDown(LKEY_LEFT)) {
                    v.x -= 1.0f;
                }
                if (PlatformKeyDown(LKEY_RIGHT)) {
                    v.x += 1.0f;
                }
                v = Vec3Scale(v, 150.0f * gstate->dt);

                pct_delta = gstate->dt / (2.0f * (1 / 60.0f));

                POS = Vec3Add(POS, v);
            } else if (jumped) {
                ANIMATION = JUMP_ANIMATION;
                ANIMATION_CYCLE = false;
                ANIMATION_IDX = 0;
                ANIMATION_PCT = 0.0f;
                movestate_next = MS_JUMP;
            } else {
                ANIMATION = IDLE_ANIMATION;
                ANIMATION_CYCLE = true;
                ANIMATION_IDX = 0;
                ANIMATION_PCT = 0.0f;
                movestate_next = MS_IDLE;
            }
            break;
        }
        case MS_JUMP: {
            if ((ANIMATION_IDX + 1 >= ANIMATION.sprites.cap) && (ANIMATION_PCT >= 1.0f)) {
                // Animation finished.
                ANIMATION = IDLE_ANIMATION;
                ANIMATION_CYCLE = true;
                ANIMATION_IDX = 0;
                ANIMATION_PCT = 0.0f;
                movestate_next = MS_IDLE;
            } else {
                pct_delta = gstate->dt / (2.0f * (1 / 60.0f));
            }

            break;
        }
    }

    MOVESTATE = movestate_next;

    BeginRender(gstate->dt);

    ShaderPass* pass = BeginShaderPass(GetLineShader());
    u32 width = gstate->renderw;
    u32 height = gstate->renderh;
    pass->proj = Mat4Ortho(-0.5f * width, 0.5f * width, -0.5f * height, 0.5f * height, -1.0f, 1.0f);

    // Good load and draw model test
    if (true) {
        {
            LTexture tx = GetTexture(TX_BLUE_WIZARD);
            Sprite sprite = SpriteFromLTexture(tx, SBB_CLOSEST_FIT);
            LModel model = LModelFromSprite(sprite);
            DrawLModel(model, POS, Mat4RotateZ(ROT), Vec3LitAll(1.0f));
        }

        {
            LTexture tx = GetTexture(TX_IMAGE_TEST4);
            Sprite sprite = SpriteFromLTexture(tx, SBB_CLOSEST_FIT);
            LModel model = LModelFromSprite(sprite);
            DrawLModel(model, Vec3Add(POS, Vec3Lit(100, 0, 0)), Mat4RotateZ(ROT), Vec3LitAll(20.0f));
        }

        // Test static rotate by 90 degrees
        {
            LTexture tx = GetTexture(TX_IMAGE_TEST4);
            Sprite sprite = SpriteFromLTexture(tx, SBB_CLOSEST_FIT);
            LModel model = LModelFromAnimation(AnimationFromSprite(sprite), Vec3Zero(), Mat4RotateZ(Deg2Rad(90.0f)),
                                               Vec3LitAll(1.0f), LCLEAR);
            DrawLModel(model, Vec3Add(POS, Vec3Lit(0, 200, 0)), Mat4RotateZ(ROT), Vec3LitAll(20.0f));
        }

        // Test static position
        {
            LTexture tx = GetTexture(TX_BLUE_WIZARD);
            Sprite sprite = SpriteFromLTexture(tx, SBB_CLOSEST_FIT);
            LModel model = LModelFromAnimation(AnimationFromSprite(sprite), Vec3Lit(-200.0f, -200.0f, 0.f),
                                               Mat4RotateZ(Deg2Rad(45.0f)), Vec3LitAll(0.5f), LCLEAR);
            DrawLModel(model, POS, Mat4RotateZ(ROT), Vec3LitAll(1.0f));
        }

        // Test graphics info
        {
            LTexture tx = GetTexture(TX_BLUE_WIZARD);
            Sprite sprite = SpriteFromLTexture(tx, SBB_CLOSEST_FIT);
            LModel model = LModelFromAnimation(AnimationFromSprite(sprite), Vec3Lit(-200.0f, 200.0f, 0.f),
                                               Mat4RotateZ(Deg2Rad(45.0f)), Vec3LitAll(0.5f), LCLEAR);
            GraphicsInfo graphics = {0};
            GraphicsInfoMemset(&graphics);
            graphics.model = model;
            graphics.pos = POS;
            graphics.orient = Mat4RotateZ(ROT);
            DrawGraphicsInfo(&graphics);
        }

        // Test graphics info animation
        {
            static GraphicsInfo graphics = {0};
            static bool INIT = true;

            if (INIT) {
                INIT = false;
                GraphicsInfoMemset(&graphics);
                graphics.model = LModelFromAnimation(IDLE_ANIMATION, Vec3Lit(0.0f, 400.0f, 0.f),
                                                     Mat4RotateZ(Deg2Rad(45.0f)), Vec3LitAll(0.5f), LCLEAR);
            }

            graphics.elapsed = f32mod(graphics.elapsed + gstate->dt, graphics.model.animation.duration);
            graphics.frame_idx = (isize)((graphics.elapsed / graphics.model.animation.duration) *
                                         ((f32)graphics.model.animation.sprites.len));

            graphics.pos = POS;
            graphics.orient = Mat4RotateZ(ROT);
            DrawGraphicsInfo(&graphics);

            // graphics.frame_idx = (graphics.frame_idx + 1) % graphics.model.animation.sprites.len;
        }
    }

    if (false) {
        LTexture tx = TX_PACKED;
        // LTexture tx = GetTexture(TX_IMAGE_TEST);
        // LTexture tx = GetTexture(TX_IMAGE_TEST3);
        // LTexture tx = GetTexture(TX_IMAGE_TEST4);
        Sprite sprite = SpriteFromLTexture(tx, SBB_CLOSEST_FIT);
        LModel model = LModelFromSprite(sprite);
        DrawLModel(model, POS, Mat4RotateZ(ROT), Vec3LitAll(1.0f));
    }

    // Packed texture. I think leave this for now. Should figure out animations + consolidate the rendering
    // model portion. then maybe how to properly do the packing will make sense.
    if (false) {
        // Sprite sprite = SPRITE_WIZARD;
        // Sprite sprite = SPRITE_MM;
        Sprite sprite = SPRITE_WIZARD2;
        LModel model = LModelFromSprite(sprite);
        DrawLModel(model, POS, Mat4RotateZ(ROT), Vec3LitAll(1.0f));
    }

    // Animation.
    if (false) {
        // idle
        if (IDLE) {
            static isize IDX = 0;
            static f32 FRAME_DT = 0.0f;
            static f32 DT_PER_FRAME = 3.0f * (1 / 60.0f);

            isize cap = IDLE_ANIMATION.sprites.cap;
            FRAME_DT += gstate->dt;
            if (FRAME_DT >= DT_PER_FRAME) {
                FRAME_DT = 0.0f;
                IDX = (IDX + 1) % cap;
            }

            DrawSprite(ArrGet(&IDLE_ANIMATION.sprites, IDX), POS, Mat4RotateZ(ROT), Vec3LitAll(1.0f));
            // DrawLTexture(ANIMATION_TXS[IDX], Vec2Lit(POS.x, POS.y), 1.0f);
            // Sprite sprite = SpriteFromLTexture(tx, SBB_CLOSEST_FIT);
            // LModel model = LModelFromSprite(sprite);
            // DrawLModel(model, POS, Mat4RotateZ(ROT), Vec3LitAll(1.0f));
        }

        // walk
        if (WALK) {
            static isize IDX = 0;
            static f32 FRAME_DT = 0.0f;
            static f32 DT_PER_FRAME = 3.0f * (1 / 60.0f);

            isize cap = WALK_ANIMATION.sprites.cap;
            FRAME_DT += gstate->dt;
            if (FRAME_DT >= DT_PER_FRAME) {
                FRAME_DT = 0.0f;
                IDX = (IDX + 1) % cap;
            }

            DrawSprite(ArrGet(&WALK_ANIMATION.sprites, IDX), POS, Mat4RotateZ(ROT), Vec3LitAll(1.0f));
            // DrawLTexture(ANIMATION_TXS[IDX], Vec2Lit(POS.x, POS.y), 1.0f);
            // Sprite sprite = SpriteFromLTexture(tx, SBB_CLOSEST_FIT);
            // LModel model = LModelFromSprite(sprite);
            // DrawLModel(model, POS, Mat4RotateZ(ROT), Vec3LitAll(1.0f));
        }

        // jump
        if (JUMP) {
            static isize IDX = 0;
            static f32 FRAME_DT = 0.0f;
            static f32 DT_PER_FRAME = 3.0f * (1 / 60.0f);

            isize cap = JUMP_ANIMATION.sprites.cap;
            FRAME_DT += gstate->dt;
            if (FRAME_DT >= DT_PER_FRAME) {
                FRAME_DT = 0.0f;
                IDX = (IDX + 1) % cap;
            }

            DrawSprite(ArrGet(&JUMP_ANIMATION.sprites, IDX), POS, Mat4RotateZ(ROT), Vec3LitAll(1.0f));
            // DrawLTexture(ANIMATION_TXS[IDX], Vec2Lit(POS.x, POS.y), 1.0f);
            // Sprite sprite = SpriteFromLTexture(tx, SBB_CLOSEST_FIT);
            // LModel model = LModelFromSprite(sprite);
            // DrawLModel(model, POS, Mat4RotateZ(ROT), Vec3LitAll(1.0f));
        }
    }

    if (false) {
        ANIMATION_PCT += pct_delta;
        if (ANIMATION_PCT >= 1.0f) {
            if (ANIMATION_CYCLE || ANIMATION_IDX + 1 < ANIMATION.sprites.cap) {
                ANIMATION_PCT = 0.0f;
                ANIMATION_IDX = (ANIMATION_IDX + 1) % ANIMATION.sprites.cap;
            } else {
                // We aren't cycling the animation and are at the end of the last frame's completion pct, at this
                // point clamp the progress pct to 1 to avoid overflow if left alone forever.
                ANIMATION_PCT = 1.0f;
            }
        }

        DrawSprite(ArrGet(&ANIMATION.sprites, ANIMATION_IDX), POS, Mat4RotateZ(ROT), Vec3LitAll(1.0f));
    }

    EndShaderPass();

    EndRender();
}

////////////////////////////////////////////////////////////

static void TestL();

void GameInit() {
    GameState* gstate = &GState;
    TestL();

    memset(gstate, 0, sizeof(GameState));

    // Initialize dyanmic memory pool.
    isize cap = GB(4);
    byte* buffer = PlatformAllocate(cap);
    Arena perm = NewArena(buffer, cap);
    gstate->perm = perm;
    byte* temp_buffer = New(&gstate->perm, GB(1), byte);
    gstate->temp = NewArena(temp_buffer, GB(1));

    gstate->renderw = 1024.0f;
    gstate->renderh = 1024.0f;

    u32 scale = 1;
    PlatformInit(scale * gstate->renderw, scale * gstate->renderh, 60);

    isize sound_cap = GB(2);
    byte* sound_buffer = New(&gstate->perm, sound_cap, byte);

    PlatformInitAudio(sound_buffer, sound_cap);
    InitFonts();
    ParticleSystemInit();

    RenderStateInit(gstate->renderw, gstate->renderh);
    RenderDataInit();
    LoadSounds();

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
        gstate->scenefuncs[SCENE_NOISE] = SceneNoiseTick;
        gstate->scenefuncs[SCENE_LINE] = SceneLineTick;
        gstate->scenefuncs[SCENE_VECTOR] = SceneVectorTick;
        Assert(SCENE_COUNT < MAX_SCENES);
        gstate->scenefuncssize = idx;
    }
    GameStateLoadScene(gstate, SCENE_VECTOR);

    gstate->lecap = ArraySizeOf(gstate->loadable_entities);
    gstate->lelen = 0;

    gstate->fts_tail = 1;
    gstate->fts_head = 0;
}

void GameTick(f32 dt) {
    GameState* gstate = &GState;
    gstate->dt = dt;
    gstate->timer += dt;

    gstate->screenw = PlatformScreenWidth();
    gstate->screenh = PlatformScreenHeight();

    {
        static bool ENABLE_DEBUG_KEYMAP = true;

        if (ENABLE_DEBUG_KEYMAP) {
            bool debugkey = PlatformKeyDown(LKEY_LCONTROL) && PlatformKeyPressed(LKEY_1);
            if (debugkey) {
                gstate->debug = !gstate->debug;
                gstate->update = !gstate->debug;
                gstate->debug_ever = true;
                PlatformLog(LLOG_INFO, "Debug %s", gstate->debug ? "On" : "Off");
            }
            if (gstate->debug) {
                bool updatekey = PlatformKeyPressed(LKEY_U);
                if (updatekey) {
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

    gstate->init = false;
    ++gstate->nframe;

    // FPS count
    // TODO: Bug if spamming enter on first couple screens. head == tail.
    if (false) {
        Assert(gstate->fts_tail != gstate->fts_head);  // TODO: just like emit a warning log here or something.
        i32 len = ArraySizeOf(gstate->frame_times);
        i32 insert_idx = gstate->fts_tail;
        gstate->frame_times[insert_idx] = gstate->timer;
        ++gstate->fts_tail;
        gstate->fts_tail %= len;

        // The FPS is how many frames we've shown in the last second interval.
        // We only count frames who's timestamp is within a second of the most recent timestamp.
        // Could probably do this instead by storing delta times, but easier to think about in terms
        // of absolute timestamps (with dts I worry about off by one errors).
        i32 cur = insert_idx;
        u32 fps = 0;
        for (; ((cur + len - 1) % len) != gstate->fts_head && (gstate->timer - gstate->frame_times[cur]) <= 1.0f;) {
            cur = (cur + len - 1) % len;
            ++fps;
        }

        gstate->fts_head = cur;
        gstate->fps = fps;
        // PlatformLog(LLOG_INFO, "fps: %d", gstate->fps);
    }
}

////////////////////////////////////////////////////////////

static bool TestFail = false;

static void TAssert(bool result, const char* msg) {
    if (!result) {
        PlatformLog(LLOG_ERROR, "Failed: %s", msg);
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
            byte buffer[1] = {0};
            Arena a = NewArena(buffer, ArraySizeOf(buffer));
            TAssert(a.beg != NULL, "NewArena(1) beg not null");
            TAssert(a.end != a.beg, "NewArena(1) end != beg");
            TAssert(a.end == a.beg + 1, "NewArena(1) end = beg + 1");
            TAssert(a.end - a.beg == 1, "NewArena(1) end - beg == 1");
        }

        {
            byte buffer[1234] = {0};
            Arena a = NewArena(buffer, ArraySizeOf(buffer));
            TAssert(a.end == a.beg + 1234, "NewArena(1234) end = beg + 1234");
            TAssert(a.end - a.beg == 1234, "NewArena(1234) end - beg == 1234");
        }
    }

    // New
    {
        {
            byte buffer[1] = {0};
            Arena a = NewArena(buffer, ArraySizeOf(buffer));
            u8* begprev = a.beg;
            u8* b = New(&a, 1, u8);
            *b = 10;

            TAssert(b == begprev, "NewArena(1) + New(1, u8): b == begprev");
            TAssert(a.beg == a.end, "NewArena(1) + New(1, u8): beg == end");
            TAssert(*b == 10, "NewArena(1) + New(1, u8): *b == 10");
        }

        {
            byte buffer[10] = {0};
            Arena a = NewArena(buffer, ArraySizeOf(buffer));
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

    // PushS
    {
        {
            byte buffer[100] = {0};
            Arena a = NewArena(buffer, ArraySizeOf(buffer));

            struct {
                i32* data;
                isize len;
                isize cap;
            } ii = {0};
            i32* data = New(&a, 5, i32);
            InitArrS(&ii, data, 5);
            TAssert(ii.len == 0, "PushS(int): len == 0");

            *PushS(&ii) = 0;
            *PushS(&ii) = 10;
            *PushS(&ii) = 20;
            *PushS(&ii) = 30;
            *PushS(&ii) = 40;
            for (i32 i = 0; i < ii.len; ++i) {
                TAssert(ii.data[i] == i * 10, "PushS: data matches");
            }
            TAssert(ii.len == 5, "PushS(int): len == 5");
            TAssert(ii.cap == 5, "PushS(int): cap == 5");
        }
    }
}

static void TestRand() {
    // f32rand01
    {
        for (i32 i = 0; i < 1000; ++i) {
            f32 r = f32rand01();
            TAssert(r >= 0.0f && r <= 1.0f, "0.0f <= f32rand01() <= 1.0f");
        }
    }

    // f32randab
    {
        // no diff.
        {
            for (i32 i = 0; i < 1000; ++i) {
                f32 r = f32randab(1000.0f, 1000.0f);
                TAssert(f32eq(r, 1000.0f), "f32randab(1000, 1000) ~= 1000f");
            }
        }

        // small diff.
        {
            for (i32 i = 0; i < 1000; ++i) {
                f32 r = f32randab(100.0f, 101.0f);
                TAssert(r >= 100.0f && r <= 101.0f, "100.0f <= f32randab(100, 101) <= 101f");
            }
        }
    }
}

static void TestL() {
    PlatformLog(LLOG_INFO, "Running Tests");
    TestMath();
    TestMemory();
    TestRand();

    if (TestFail) {
        PlatformLog(LLOG_ERROR, "Failed tests!");
        exit(1);
    }

    PlatformLog(LLOG_INFO, "Tests Pass");
}
