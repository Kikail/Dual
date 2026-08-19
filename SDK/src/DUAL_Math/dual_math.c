#include "../../include/DUAL_Math/dual_math.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VECTEURS 2D & 3D
 * ========================================================================== */

DUAL_Vec2 DUAL_Vec2_Add(DUAL_Vec2 a, DUAL_Vec2 b) { return (DUAL_Vec2){a.x + b.x, a.y + b.y}; }
DUAL_Vec2 DUAL_Vec2_Sub(DUAL_Vec2 a, DUAL_Vec2 b) { return (DUAL_Vec2){a.x - b.x, a.y - b.y}; }
DUAL_Vec2 DUAL_Vec2_Scale(DUAL_Vec2 v, float s)   { return (DUAL_Vec2){v.x * s, v.y * s}; }
float DUAL_Vec2_Length(DUAL_Vec2 v)               { return sqrtf(v.x * v.x + v.y * v.y); }
float DUAL_Vec2_Dot(DUAL_Vec2 a, DUAL_Vec2 b)     { return a.x * b.x + a.y * b.y; }
float DUAL_Vec2_Distance(DUAL_Vec2 a, DUAL_Vec2 b){ return DUAL_Vec2_Length(DUAL_Vec2_Sub(a, b)); }
void DUAL_Vec2_Log(DUAL_Vec2 v)                   { printf("Vec2(%.3f, %.3f)\n", v.x, v.y); }

DUAL_Vec2 DUAL_Vec2_Normalize(DUAL_Vec2 v) {
    float len = DUAL_Vec2_Length(v);
    if (len > 0.00001f) return (DUAL_Vec2){v.x / len, v.y / len};
    return (DUAL_Vec2){0.0f, 0.0f};
}

DUAL_Vec3 DUAL_Vec3_Add(DUAL_Vec3 a, DUAL_Vec3 b) { return (DUAL_Vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
DUAL_Vec3 DUAL_Vec3_Sub(DUAL_Vec3 a, DUAL_Vec3 b) { return (DUAL_Vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
DUAL_Vec3 DUAL_Vec3_Scale(DUAL_Vec3 v, float s)   { return (DUAL_Vec3){v.x * s, v.y * s, v.z * s}; }
float DUAL_Vec3_Dot(DUAL_Vec3 a, DUAL_Vec3 b)     { return a.x * b.x + a.y * b.y + a.z * b.z; }
float DUAL_Vec3_Length(DUAL_Vec3 v)               { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }
float DUAL_Vec3_Distance(DUAL_Vec3 a, DUAL_Vec3 b){ return DUAL_Vec3_Length(DUAL_Vec3_Sub(a, b)); }
void DUAL_Vec3_Log(DUAL_Vec3 v)                   { printf("Vec3(%.3f, %.3f, %.3f)\n", v.x, v.y, v.z); }
void DUAL_Vec4_Log(DUAL_Vec4 v)                   { printf("Vec4(%.3f, %.3f, %.3f, %.3f)\n", v.x, v.y, v.z, v.w); }

DUAL_Vec3 DUAL_Vec3_Cross(DUAL_Vec3 a, DUAL_Vec3 b) {
    return (DUAL_Vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

DUAL_Vec3 DUAL_Vec3_Normalize(DUAL_Vec3 v) {
    float len = DUAL_Vec3_Length(v);
    if (len > 0.00001f) return (DUAL_Vec3){v.x / len, v.y / len, v.z / len};
    return (DUAL_Vec3){0.0f, 0.0f, 0.0f};
}

/* ============================================================================
 * QUATERNIONS & ANIMATION
 * ========================================================================== */

DUAL_Quat DUAL_Quat_Normalize(DUAL_Quat q) {
    float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len > 0.00001f) return (DUAL_Quat){q.x / len, q.y / len, q.z / len, q.w / len};
    return (DUAL_Quat){0.0f, 0.0f, 0.0f, 1.0f};
}

DUAL_Quat DUAL_Quat_Slerp(DUAL_Quat q1, DUAL_Quat q2, float t) {
    DUAL_Quat res;
    float cosHalfTheta = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;

    // Prendre le chemin le plus court
    if (cosHalfTheta < 0.0f) {
        q2.x = -q2.x; q2.y = -q2.y; q2.z = -q2.z; q2.w = -q2.w;
        cosHalfTheta = -cosHalfTheta;
    }

    // Si les quaternions sont très proches, utiliser Lerp pour éviter la division par zéro
    if (cosHalfTheta >= 0.999f) {
        res.x = q1.x + t * (q2.x - q1.x);
        res.y = q1.y + t * (q2.y - q1.y);
        res.z = q1.z + t * (q2.z - q1.z);
        res.w = q1.w + t * (q2.w - q1.w);
        return DUAL_Quat_Normalize(res);
    }

    float halfTheta = acosf(cosHalfTheta);
    float sinHalfTheta = sqrtf(1.0f - cosHalfTheta * cosHalfTheta);

    float ratioA = sinf((1.0f - t) * halfTheta) / sinHalfTheta;
    float ratioB = sinf(t * halfTheta) / sinHalfTheta;

    res.x = (q1.x * ratioA) + (q2.x * ratioB);
    res.y = (q1.y * ratioA) + (q2.y * ratioB);
    res.z = (q1.z * ratioA) + (q2.z * ratioB);
    res.w = (q1.w * ratioA) + (q2.w * ratioB);

    return res;
}

DUAL_Mat4 DUAL_Quat_ToMat4(DUAL_Quat q) {
    DUAL_Mat4 res = DUAL_Mat4_Identity();
    float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

    res.m[0] = 1.0f - 2.0f * (yy + zz);
    res.m[1] = 2.0f * (xy + wz);
    res.m[2] = 2.0f * (xz - wy);

    res.m[4] = 2.0f * (xy - wz);
    res.m[5] = 1.0f - 2.0f * (xx + zz);
    res.m[6] = 2.0f * (yz + wx);

    res.m[8] = 2.0f * (xz + wy);
    res.m[9] = 2.0f * (yz - wx);
    res.m[10] = 1.0f - 2.0f * (xx + yy);

    return res;
}

/* ============================================================================
 * MATRICES 4X4 (Column-Major)
 * ========================================================================== */

DUAL_Mat4 DUAL_Mat4_Identity(void) {
    DUAL_Mat4 res;
    memset(res.m, 0, sizeof(res.m));
    res.m[0] = 1.0f; res.m[5] = 1.0f; res.m[10] = 1.0f; res.m[15] = 1.0f;
    return res;
}

DUAL_Mat4 DUAL_Mat4_Multiply(DUAL_Mat4 a, DUAL_Mat4 b) {
    DUAL_Mat4 res;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            res.m[col * 4 + row] =
                a.m[0 * 4 + row] * b.m[col * 4 + 0] +
                a.m[1 * 4 + row] * b.m[col * 4 + 1] +
                a.m[2 * 4 + row] * b.m[col * 4 + 2] +
                a.m[3 * 4 + row] * b.m[col * 4 + 3];
        }
    }
    return res;
}

DUAL_Mat4 DUAL_Mat4_Translate(DUAL_Vec3 d) {
    DUAL_Mat4 res = DUAL_Mat4_Identity();
    res.m[12] = d.x;
    res.m[13] = d.y;
    res.m[14] = d.z;
    return res;
}

DUAL_Mat4 DUAL_Mat4_Scale(DUAL_Vec3 s) {
    DUAL_Mat4 res = DUAL_Mat4_Identity();
    res.m[0] = s.x;
    res.m[5] = s.y;
    res.m[10] = s.z;
    return res;
}

DUAL_Mat4 DUAL_Mat4_Rotate(DUAL_Vec3 axe, float angle) {
    DUAL_Mat4 res = DUAL_Mat4_Identity();
    DUAL_Vec3 n = DUAL_Vec3_Normalize(axe);
    float c = cosf(angle);
    float s = sinf(angle);
    float t = 1.0f - c;

    res.m[0] = t * n.x * n.x + c;
    res.m[1] = t * n.x * n.y + s * n.z;
    res.m[2] = t * n.x * n.z - s * n.y;

    res.m[4] = t * n.x * n.y - s * n.z;
    res.m[5] = t * n.y * n.y + c;
    res.m[6] = t * n.y * n.z + s * n.x;

    res.m[8]  = t * n.x * n.z + s * n.y;
    res.m[9]  = t * n.y * n.z - s * n.x;
    res.m[10] = t * n.z * n.z + c;

    return res;
}

DUAL_Mat4 DUAL_Mat4_Ortho(float left, float right, float bottom, float top, float near, float far) {
    DUAL_Mat4 res;
    memset(res.m, 0, sizeof(res.m));
    res.m[0]  = 2.0f / (right - left);
    res.m[5]  = 2.0f / (top - bottom);
    res.m[10] = -2.0f / (far - near);
    res.m[12] = -(right + left) / (right - left);
    res.m[13] = -(top + bottom) / (top - bottom);
    res.m[14] = -(far + near) / (far - near);
    res.m[15] = 1.0f;
    return res;
}

DUAL_Mat4 DUAL_Mat4_Perspective(float fov, float aspect, float near, float far) {
    DUAL_Mat4 res;
    memset(res.m, 0, sizeof(res.m));
    float tanHalfFov = tanf(fov / 2.0f);
    res.m[0]  = 1.0f / (aspect * tanHalfFov);
    res.m[5]  = 1.0f / tanHalfFov;
    res.m[10] = -(far + near) / (far - near);
    res.m[11] = -1.0f;
    res.m[14] = -(2.0f * far * near) / (far - near);
    return res;
}

DUAL_Mat4 DUAL_Mat4_LookAt(DUAL_Vec3 pos, DUAL_Vec3 target, DUAL_Vec3 up) {
    DUAL_Vec3 f = DUAL_Vec3_Normalize(DUAL_Vec3_Sub(target, pos));
    DUAL_Vec3 s = DUAL_Vec3_Normalize(DUAL_Vec3_Cross(f, up));
    DUAL_Vec3 u = DUAL_Vec3_Cross(s, f);

    DUAL_Mat4 res = DUAL_Mat4_Identity();
    res.m[0] = s.x; res.m[4] = s.y; res.m[8]  = s.z;
    res.m[1] = u.x; res.m[5] = u.y; res.m[9]  = u.z;
    res.m[2] = -f.x;res.m[6] = -f.y;res.m[10] = -f.z;

    res.m[12] = -DUAL_Vec3_Dot(s, pos);
    res.m[13] = -DUAL_Vec3_Dot(u, pos);
    res.m[14] = DUAL_Vec3_Dot(f, pos);
    return res;
}

void DUAL_Mat4_Log(DUAL_Mat4 m) {
    printf("Mat4:\n");
    for(int i = 0; i < 4; i++) {
        printf("[%.2f, %.2f, %.2f, %.2f]\n", m.m[i], m.m[i+4], m.m[i+8], m.m[i+12]);
    }
}

/* ============================================================================
 * INTERPOLATIONS
 * ========================================================================== */

float DUAL_Clamp(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

float DUAL_Lerp(float a, float b, float t) {
    return a + t * (b - a);
}

DUAL_Vec2 DUAL_Vec2_Lerp(DUAL_Vec2 a, DUAL_Vec2 b, float t) {
    return (DUAL_Vec2){ a.x + t * (b.x - a.x), a.y + t * (b.y - a.y) };
}

DUAL_Vec3 DUAL_Vec3_Lerp(DUAL_Vec3 a, DUAL_Vec3 b, float t) {
    return (DUAL_Vec3){ a.x + t * (b.x - a.x), a.y + t * (b.y - a.y), a.z + t * (b.z - a.z) };
}

DUAL_Vec3 DUAL_Vec3_Mix(DUAL_Vec3 a, DUAL_Vec3 b, float t) {
    return DUAL_Vec3_Lerp(a, b, t);
}

float DUAL_EaseInOut(float t) {
    t = DUAL_Clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/* ============================================================================
 * COLLISIONS (Primitives de base)
 * ========================================================================== */

bool DUAL_CollideRectRect(DUAL_Rect a, DUAL_Rect b) {
    return (a.x < b.x + b.largeur && a.x + a.largeur > b.x &&
            a.y < b.y + b.hauteur && a.y + a.hauteur > b.y);
}

bool DUAL_CollideCircleCircle(DUAL_Circle a, DUAL_Circle b) {
    float dist = DUAL_Vec2_Distance(a.centre, b.centre);
    return dist < (a.rayon + b.rayon);
}

bool DUAL_CollidePointRect(DUAL_Vec2 p, DUAL_Rect r) {
    return (p.x >= r.x && p.x <= r.x + r.largeur &&
            p.y >= r.y && p.y <= r.y + r.hauteur);
}

bool DUAL_CollideAABBAABB(DUAL_AABB a, DUAL_AABB b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x &&
            a.min.y <= b.max.y && a.max.y >= b.min.y &&
            a.min.z <= b.max.z && a.max.z >= b.min.z);
}

bool DUAL_CollideSphereSphere(DUAL_Sphere a, DUAL_Sphere b) {
    float dist = DUAL_Vec3_Distance(a.centre, b.centre);
    return dist < (a.rayon + b.rayon);
}