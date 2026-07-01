#include "dual_math.h"
#include <math.h>

/* ============================================================================
 * Opérations sur DUAL_Vec2
 * ========================================================================== */

DUAL_Vec2 DUAL_Vec2_Add(DUAL_Vec2 a, DUAL_Vec2 b) {
    return (DUAL_Vec2){ a.x + b.x, a.y + b.y };
}

DUAL_Vec2 DUAL_Vec2_Sub(DUAL_Vec2 a, DUAL_Vec2 b) {
    return (DUAL_Vec2){ a.x - b.x, a.y - b.y };
}

DUAL_Vec2 DUAL_Vec2_Scale(DUAL_Vec2 v, float scalaire) {
    return (DUAL_Vec2){ v.x * scalaire, v.y * scalaire };
}

float DUAL_Vec2_Length(DUAL_Vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

DUAL_Vec2 DUAL_Vec2_Normalize(DUAL_Vec2 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y);
    if (len == 0.0f) return (DUAL_Vec2){ 0.0f, 0.0f };
    return (DUAL_Vec2){ v.x / len, v.y / len };
}

float DUAL_Vec2_Dot(DUAL_Vec2 a, DUAL_Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

float DUAL_Vec2_Distance(DUAL_Vec2 a, DUAL_Vec2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

/* ============================================================================
 * Opérations sur DUAL_Vec3
 * ========================================================================== */

DUAL_Vec3 DUAL_Vec3_Add(DUAL_Vec3 a, DUAL_Vec3 b) {
    return (DUAL_Vec3){ a.x + b.x, a.y + b.y, a.z + b.z };
}

DUAL_Vec3 DUAL_Vec3_Sub(DUAL_Vec3 a, DUAL_Vec3 b) {
    return (DUAL_Vec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

DUAL_Vec3 DUAL_Vec3_Scale(DUAL_Vec3 v, float scalaire) {
    return (DUAL_Vec3){ v.x * scalaire, v.y * scalaire, v.z * scalaire };
}

DUAL_Vec3 DUAL_Vec3_Cross(DUAL_Vec3 a, DUAL_Vec3 b) {
    return (DUAL_Vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

float DUAL_Vec3_Dot(DUAL_Vec3 a, DUAL_Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float DUAL_Vec3_Length(DUAL_Vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

DUAL_Vec3 DUAL_Vec3_Normalize(DUAL_Vec3 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len == 0.0f) return (DUAL_Vec3){ 0.0f, 0.0f, 0.0f };
    return (DUAL_Vec3){ v.x / len, v.y / len, v.z / len };
}

/* ============================================================================
 * Opérations matricielles (DUAL_Mat4 - Column Major)
 * ========================================================================== */

DUAL_Mat4 DUAL_Mat4_Identity(void) {
    DUAL_Mat4 mat = {0};
    mat.m[0] = 1.0f; mat.m[5] = 1.0f; mat.m[10] = 1.0f; mat.m[15] = 1.0f;
    return mat;
}

DUAL_Mat4 DUAL_Mat4_Multiply(DUAL_Mat4 a, DUAL_Mat4 b) {
    DUAL_Mat4 out = {0};
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            out.m[col * 4 + row] = 
                a.m[0 * 4 + row] * b.m[col * 4 + 0] +
                a.m[1 * 4 + row] * b.m[col * 4 + 1] +
                a.m[2 * 4 + row] * b.m[col * 4 + 2] +
                a.m[3 * 4 + row] * b.m[col * 4 + 3];
        }
    }
    return out;
}

DUAL_Mat4 DUAL_Mat4_Translate(DUAL_Vec3 deplacement) {
    DUAL_Mat4 mat = DUAL_Mat4_Identity();
    mat.m[12] = deplacement.x;
    mat.m[13] = deplacement.y;
    mat.m[14] = deplacement.z;
    return mat;
}

DUAL_Mat4 DUAL_Mat4_Scale(DUAL_Vec3 echelle) {
    DUAL_Mat4 mat = DUAL_Mat4_Identity();
    mat.m[0] = echelle.x;
    mat.m[5] = echelle.y;
    mat.m[10] = echelle.z;
    return mat;
}

DUAL_Mat4 DUAL_Mat4_Rotate(DUAL_Vec3 axe, float angle_radians) {
    DUAL_Mat4 mat = {0};
    float c = cosf(angle_radians);
    float s = sinf(angle_radians);
    float t = 1.0f - c;

    DUAL_Vec3 axis = DUAL_Vec3_Normalize(axe);

    mat.m[0] = t * axis.x * axis.x + c;
    mat.m[1] = t * axis.x * axis.y + axis.z * s;
    mat.m[2] = t * axis.x * axis.z - axis.y * s;
    mat.m[3] = 0.0f;

    mat.m[4] = t * axis.x * axis.y - axis.z * s;
    mat.m[5] = t * axis.y * axis.y + c;
    mat.m[6] = t * axis.y * axis.z + axis.x * s;
    mat.m[7] = 0.0f;

    mat.m[8] = t * axis.x * axis.z + axis.y * s;
    mat.m[9] = t * axis.y * axis.z - axis.x * s;
    mat.m[10] = t * axis.z * axis.z + c;
    mat.m[11] = 0.0f;

    mat.m[12] = 0.0f; mat.m[13] = 0.0f; mat.m[14] = 0.0f; mat.m[15] = 1.0f;
    return mat;
}

DUAL_Mat4 DUAL_Mat4_Ortho(float gauche, float droite, float bas, float haut, float proche, float lointain) {
    DUAL_Mat4 mat = DUAL_Mat4_Identity();
    mat.m[0]  = 2.0f / (droite - gauche);
    mat.m[5]  = 2.0f / (haut - bas);
    mat.m[10] = -2.0f / (lointain - proche);
    
    mat.m[12] = -(droite + gauche) / (droite - gauche);
    mat.m[13] = -(haut + bas) / (haut - bas);
    mat.m[14] = -(lointain + proche) / (lointain - proche);
    return mat;
}

DUAL_Mat4 DUAL_Mat4_Perspective(float fov_radians, float ratio_aspect, float proche, float lointain) {
    DUAL_Mat4 mat = {0};
    float f = 1.0f / tanf(fov_radians / 2.0f);
    
    mat.m[0] = f / ratio_aspect;
    mat.m[5] = f;
    mat.m[10] = (lointain + proche) / (proche - lointain);
    mat.m[11] = -1.0f;
    mat.m[14] = (2.0f * lointain * proche) / (proche - lointain);
    return mat;
}

DUAL_Mat4 DUAL_Mat4_LookAt(DUAL_Vec3 position, DUAL_Vec3 cible, DUAL_Vec3 haut) {
    DUAL_Vec3 f = DUAL_Vec3_Normalize(DUAL_Vec3_Sub(cible, position));
    DUAL_Vec3 r = DUAL_Vec3_Normalize(DUAL_Vec3_Cross(f, haut));
    DUAL_Vec3 u = DUAL_Vec3_Cross(r, f);

    DUAL_Mat4 out = DUAL_Mat4_Identity();
    out.m[0] = r.x;  out.m[4] = r.y;  out.m[8] = r.z;
    out.m[1] = u.x;  out.m[5] = u.y;  out.m[9] = u.z;
    out.m[2] = -f.x; out.m[6] = -f.y; out.m[10] = -f.z;

    out.m[12] = -DUAL_Vec3_Dot(r, position);
    out.m[13] = -DUAL_Vec3_Dot(u, position);
    out.m[14] =  DUAL_Vec3_Dot(f, position);
    return out;
}

/* ============================================================================
 * Tests de collision
 * ========================================================================== */

bool DUAL_CollideRectRect(DUAL_Rect a, DUAL_Rect b) {
    return (a.x < b.x + b.largeur &&
            a.x + a.largeur > b.x &&
            a.y < b.y + b.hauteur &&
            a.y + a.hauteur > b.y);
}

bool DUAL_CollideCircleCircle(DUAL_Circle a, DUAL_Circle b) {
    float distSq = (a.centre.x - b.centre.x) * (a.centre.x - b.centre.x) +
                   (a.centre.y - b.centre.y) * (a.centre.y - b.centre.y);
    float radiusSum = a.rayon + b.rayon;
    return distSq < (radiusSum * radiusSum);
}

bool DUAL_CollidePointRect(DUAL_Vec2 point, DUAL_Rect rect) {
    return (point.x >= rect.x &&
            point.x <= rect.x + rect.largeur &&
            point.y >= rect.y &&
            point.y <= rect.y + rect.hauteur);
}

bool DUAL_CollideAABBAABB(DUAL_AABB a, DUAL_AABB b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

bool DUAL_CollideSphereSphere(DUAL_Sphere a, DUAL_Sphere b) {
    float distSq = (a.centre.x - b.centre.x) * (a.centre.x - b.centre.x) +
                   (a.centre.y - b.centre.y) * (a.centre.y - b.centre.y) +
                   (a.centre.z - b.centre.z) * (a.centre.z - b.centre.z);
    float radiusSum = a.rayon + b.rayon;
    return distSq < (radiusSum * radiusSum);
}

/* ============================================================================
 * Interpolation
 * ========================================================================== */

float DUAL_Lerp(float depart, float arrivee, float t) {
    return depart + t * (arrivee - depart);
}

DUAL_Vec2 DUAL_Vec2_Lerp(DUAL_Vec2 depart, DUAL_Vec2 arrivee, float t) {
    return (DUAL_Vec2){
        DUAL_Lerp(depart.x, arrivee.x, t),
        DUAL_Lerp(depart.y, arrivee.y, t)
    };
}

DUAL_Vec3 DUAL_Vec3_Lerp(DUAL_Vec3 depart, DUAL_Vec3 arrivee, float t) {
    return (DUAL_Vec3){
        DUAL_Lerp(depart.x, arrivee.x, t),
        DUAL_Lerp(depart.y, arrivee.y, t),
        DUAL_Lerp(depart.z, arrivee.z, t)
    };
}

float DUAL_EaseInOut(float t) {
    /* Équation Smoothstep cubique standard : 3t^2 - 2t^3 */
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

float DUAL_Clamp(float valeur, float min, float max) {
    if (valeur < min) return min;
    if (valeur > max) return max;
    return valeur;
}