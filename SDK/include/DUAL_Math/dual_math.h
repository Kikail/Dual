/**
 * @file dual_math.h
 * @brief Module mathématique de libdual : vecteurs 2D/3D, matrices, quaternions.
 */

#ifndef DUAL_MATH_H
#define DUAL_MATH_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DUAL_RAD(deg) ((deg) * 0.01745329251f)

typedef struct DUAL_Vec2 { float x, y; } DUAL_Vec2;
typedef struct DUAL_Vec3 { float x, y, z; } DUAL_Vec3;
typedef struct DUAL_Vec4 { float x, y, z, w; } DUAL_Vec4;

/**
 * @brief Quaternion pour représenter des rotations 3D de manière sûre (évite le Gimbal Lock).
 */
typedef struct DUAL_Quat {
    float x, y, z, w;
} DUAL_Quat;

typedef struct DUAL_Mat4 { float m[16]; } DUAL_Mat4;

typedef struct DUAL_Rect { float x, y, largeur, hauteur; } DUAL_Rect;
typedef struct DUAL_Circle { DUAL_Vec2 centre; float rayon; } DUAL_Circle;
typedef struct DUAL_AABB { DUAL_Vec3 min; DUAL_Vec3 max; } DUAL_AABB;
typedef struct DUAL_Sphere { DUAL_Vec3 centre; float rayon; } DUAL_Sphere;

/* Opérations sur DUAL_Vec2 */
DUAL_Vec2 DUAL_Vec2_Add(DUAL_Vec2 a, DUAL_Vec2 b);
DUAL_Vec2 DUAL_Vec2_Sub(DUAL_Vec2 a, DUAL_Vec2 b);
DUAL_Vec2 DUAL_Vec2_Scale(DUAL_Vec2 v, float scalaire);
float     DUAL_Vec2_Length(DUAL_Vec2 v);
DUAL_Vec2 DUAL_Vec2_Normalize(DUAL_Vec2 v);
float     DUAL_Vec2_Dot(DUAL_Vec2 a, DUAL_Vec2 b);
float     DUAL_Vec2_Distance(DUAL_Vec2 a, DUAL_Vec2 b);
void      DUAL_Vec2_Log(DUAL_Vec2 v);

/* Opérations sur DUAL_Vec3 */
DUAL_Vec3 DUAL_Vec3_Add(DUAL_Vec3 a, DUAL_Vec3 b);
DUAL_Vec3 DUAL_Vec3_Sub(DUAL_Vec3 a, DUAL_Vec3 b);
DUAL_Vec3 DUAL_Vec3_Scale(DUAL_Vec3 v, float scalaire);
DUAL_Vec3 DUAL_Vec3_Cross(DUAL_Vec3 a, DUAL_Vec3 b);
float     DUAL_Vec3_Dot(DUAL_Vec3 a, DUAL_Vec3 b);
float     DUAL_Vec3_Length(DUAL_Vec3 v);
DUAL_Vec3 DUAL_Vec3_Normalize(DUAL_Vec3 v);
float     DUAL_Vec3_Distance(DUAL_Vec3 a, DUAL_Vec3 b);
void      DUAL_Vec3_Log(DUAL_Vec3 v);

/* Opérations sur DUAL_Vec4 */
void      DUAL_Vec4_Log(DUAL_Vec4 v);

/* Opérations sur DUAL_Quat (ANIMATION) */
DUAL_Quat DUAL_Quat_Normalize(DUAL_Quat q);
DUAL_Quat DUAL_Quat_Slerp(DUAL_Quat q1, DUAL_Quat q2, float t);
DUAL_Mat4 DUAL_Quat_ToMat4(DUAL_Quat q);

/* Opérations matricielles (DUAL_Mat4) */
DUAL_Mat4 DUAL_Mat4_Identity(void);
DUAL_Mat4 DUAL_Mat4_Multiply(DUAL_Mat4 a, DUAL_Mat4 b);
DUAL_Mat4 DUAL_Mat4_Translate(DUAL_Vec3 deplacement);
DUAL_Mat4 DUAL_Mat4_Scale(DUAL_Vec3 echelle);
DUAL_Mat4 DUAL_Mat4_Rotate(DUAL_Vec3 axe, float angle_radians);
DUAL_Mat4 DUAL_Mat4_Ortho(float gauche, float droite, float bas, float haut, float proche, float lointain);
DUAL_Mat4 DUAL_Mat4_Perspective(float fov_radians, float ratio_aspect, float proche, float lointain);
DUAL_Mat4 DUAL_Mat4_LookAt(DUAL_Vec3 position, DUAL_Vec3 cible, DUAL_Vec3 haut);
void      DUAL_Mat4_Log(DUAL_Mat4 m);

/* Tests de collision */
bool DUAL_CollideRectRect(DUAL_Rect a, DUAL_Rect b);
bool DUAL_CollideCircleCircle(DUAL_Circle a, DUAL_Circle b);
bool DUAL_CollidePointRect(DUAL_Vec2 point, DUAL_Rect rect);
bool DUAL_CollideAABBAABB(DUAL_AABB a, DUAL_AABB b);
bool DUAL_CollideSphereSphere(DUAL_Sphere a, DUAL_Sphere b);

/* Interpolation */
float     DUAL_Lerp(float depart, float arrivee, float t);
DUAL_Vec2 DUAL_Vec2_Lerp(DUAL_Vec2 depart, DUAL_Vec2 arrivee, float t);
DUAL_Vec3 DUAL_Vec3_Lerp(DUAL_Vec3 depart, DUAL_Vec3 arrivee, float t);
DUAL_Vec3 DUAL_Vec3_Mix(DUAL_Vec3 depart, DUAL_Vec3 arrivee, float t); // Alias utilisé par LearnOpenGL
float     DUAL_EaseInOut(float t);
float     DUAL_Clamp(float valeur, float min, float max);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_MATH_H */