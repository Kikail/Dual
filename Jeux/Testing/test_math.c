//
// Created by killian on 7/9/26.
//
#include <stdio.h>
#include <stdlib.h>

#include "dual_core.h"
#include "dual_math.h"
#include "dual_utils.h"

#define DEBUG_VEC2(v) printf(#v);\
printf("=");\
DUAL_Vec2_Log(v);\
printf("\n");
#define DEBUG_VEC3(v) printf(#v);\
printf("=");\
DUAL_Vec3_Log(v);\
printf("\n");
#define DEBUG_FLOAT(v) printf(#v);\
printf("=");\
printf("%f",v);\
printf("\n");
#define DEBUG_BOOL(v) printf(#v);\
printf("=");\
printf("%s", (v) ? "true" : "false");\
printf("\n");

void test_vec3() {
    // On creer nos vecteurs
    DUAL_Vec3 v0 = {0.0f, 0.0f, 0.0f};
    DUAL_Vec3 v1 = {1.0f, 1.0f, 1.0f};
    DUAL_Vec3 v2 = {2.0f, 2.0f, 2.0f};
    DUAL_Vec3 v3 = {64.0f, 128.0f, 256.0f};
    DUAL_Vec3 v4 = {-32.0f, -64.0f, -128.0f};

    // On affiche nos vecteurs
    DEBUG_VEC3(v0)
    DEBUG_VEC3(v1)
    DEBUG_VEC3(v2)
    DEBUG_VEC3(v3)
    DEBUG_VEC3(v4)

    // ADDITIONS
    DEBUG_VEC3(DUAL_Vec3_Add(v0, v1))
    DEBUG_VEC3(DUAL_Vec3_Add(v0, v2))
    DEBUG_VEC3(DUAL_Vec3_Add(v0, v3))
    DEBUG_VEC3(DUAL_Vec3_Add(v0, v4))
    DEBUG_VEC3(DUAL_Vec3_Add(v1, v2))
    DEBUG_VEC3(DUAL_Vec3_Add(v1, v3))
    DEBUG_VEC3(DUAL_Vec3_Add(v1, v4))
    DEBUG_VEC3(DUAL_Vec3_Add(v2, v3))
    DEBUG_VEC3(DUAL_Vec3_Add(v2, v4))
    DEBUG_VEC3(DUAL_Vec3_Add(v3, v4))

    // SOUSTRACTIONS
    DEBUG_VEC3(DUAL_Vec3_Sub(v0, v1))
    DEBUG_VEC3(DUAL_Vec3_Sub(v0, v2))
    DEBUG_VEC3(DUAL_Vec3_Sub(v0, v3))
    DEBUG_VEC3(DUAL_Vec3_Sub(v0, v4))
    DEBUG_VEC3(DUAL_Vec3_Sub(v1, v2))
    DEBUG_VEC3(DUAL_Vec3_Sub(v1, v3))
    DEBUG_VEC3(DUAL_Vec3_Sub(v1, v4))
    DEBUG_VEC3(DUAL_Vec3_Sub(v2, v3))
    DEBUG_VEC3(DUAL_Vec3_Sub(v2, v4))
    DEBUG_VEC3(DUAL_Vec3_Sub(v3, v4))

    // SCALE
    DEBUG_VEC3(DUAL_Vec3_Scale(v0, 10.0))
    DEBUG_VEC3(DUAL_Vec3_Scale(v1, 10.0))
    DEBUG_VEC3(DUAL_Vec3_Scale(v2, 10.0))
    DEBUG_VEC3(DUAL_Vec3_Scale(v3, 10.0))
    DEBUG_VEC3(DUAL_Vec3_Scale(v4, 10.0))
    DEBUG_VEC3(DUAL_Vec3_Scale(v0, -10.0))
    DEBUG_VEC3(DUAL_Vec3_Scale(v1, -10.0))
    DEBUG_VEC3(DUAL_Vec3_Scale(v2, -10.0))
    DEBUG_VEC3(DUAL_Vec3_Scale(v3, -10.0))
    DEBUG_VEC3(DUAL_Vec3_Scale(v4, -10.0))

    // CROSS
    DEBUG_VEC3(DUAL_Vec3_Cross(v0, v1))
    DEBUG_VEC3(DUAL_Vec3_Cross(v0, v2))
    DEBUG_VEC3(DUAL_Vec3_Cross(v0, v3))
    DEBUG_VEC3(DUAL_Vec3_Cross(v0, v4))
    DEBUG_VEC3(DUAL_Vec3_Cross(v1, v2))
    DEBUG_VEC3(DUAL_Vec3_Cross(v1, v3))
    DEBUG_VEC3(DUAL_Vec3_Cross(v1, v4))
    DEBUG_VEC3(DUAL_Vec3_Cross(v2, v3))
    DEBUG_VEC3(DUAL_Vec3_Cross(v2, v4))
    DEBUG_VEC3(DUAL_Vec3_Cross(v3, v4))

    // LENGTH
    DEBUG_FLOAT(DUAL_Vec3_Length(v0))
    DEBUG_FLOAT(DUAL_Vec3_Length(v1))
    DEBUG_FLOAT(DUAL_Vec3_Length(v2))
    DEBUG_FLOAT(DUAL_Vec3_Length(v3))
    DEBUG_FLOAT(DUAL_Vec3_Length(v4))

    // NORMALIZE
    DEBUG_VEC3(DUAL_Vec3_Normalize(v0))
    DEBUG_VEC3(DUAL_Vec3_Normalize(v1))
    DEBUG_VEC3(DUAL_Vec3_Normalize(v2))
    DEBUG_VEC3(DUAL_Vec3_Normalize(v3))
    DEBUG_VEC3(DUAL_Vec3_Normalize(v4))

    // DOT
    DEBUG_FLOAT(DUAL_Vec3_Dot(v0, v1))
    DEBUG_FLOAT(DUAL_Vec3_Dot(v0, v2))
    DEBUG_FLOAT(DUAL_Vec3_Dot(v0, v3))
    DEBUG_FLOAT(DUAL_Vec3_Dot(v0, v4))
    DEBUG_FLOAT(DUAL_Vec3_Dot(v1, v2))
    DEBUG_FLOAT(DUAL_Vec3_Dot(v1, v3))
    DEBUG_FLOAT(DUAL_Vec3_Dot(v1, v4))
    DEBUG_FLOAT(DUAL_Vec3_Dot(v2, v3))
    DEBUG_FLOAT(DUAL_Vec3_Dot(v2, v4))
    DEBUG_FLOAT(DUAL_Vec3_Dot(v3, v4))

    // DOT
    DEBUG_FLOAT(DUAL_Vec3_Distance(v0, v1))
    DEBUG_FLOAT(DUAL_Vec3_Distance(v0, v2))
    DEBUG_FLOAT(DUAL_Vec3_Distance(v0, v3))
    DEBUG_FLOAT(DUAL_Vec3_Distance(v0, v4))
    DEBUG_FLOAT(DUAL_Vec3_Distance(v1, v2))
    DEBUG_FLOAT(DUAL_Vec3_Distance(v1, v3))
    DEBUG_FLOAT(DUAL_Vec3_Distance(v1, v4))
    DEBUG_FLOAT(DUAL_Vec3_Distance(v2, v3))
    DEBUG_FLOAT(DUAL_Vec3_Distance(v2, v4))
    DEBUG_FLOAT(DUAL_Vec3_Distance(v3, v4))
}

void test_vec2() {
    // On creer nos vecteurs
    DUAL_Vec2 v0 = {0.0f, 0.0f};
    DUAL_Vec2 v1 = {1.0f, 1.0f};
    DUAL_Vec2 v2 = {2.0f, 2.0f};
    DUAL_Vec2 v3 = {64.0f, 128.0f};
    DUAL_Vec2 v4 = {-32.0f, -64.0f};

    // On affiche nos vecteurs
    DEBUG_VEC2(v0)
    DEBUG_VEC2(v1)
    DEBUG_VEC2(v2)
    DEBUG_VEC2(v3)
    DEBUG_VEC2(v4)

    // ADDITIONS
    DEBUG_VEC2(DUAL_Vec2_Add(v0, v1))
    DEBUG_VEC2(DUAL_Vec2_Add(v0, v2))
    DEBUG_VEC2(DUAL_Vec2_Add(v0, v3))
    DEBUG_VEC2(DUAL_Vec2_Add(v0, v4))
    DEBUG_VEC2(DUAL_Vec2_Add(v1, v2))
    DEBUG_VEC2(DUAL_Vec2_Add(v1, v3))
    DEBUG_VEC2(DUAL_Vec2_Add(v1, v4))
    DEBUG_VEC2(DUAL_Vec2_Add(v2, v3))
    DEBUG_VEC2(DUAL_Vec2_Add(v2, v4))
    DEBUG_VEC2(DUAL_Vec2_Add(v3, v4))

    // SOUSTRACTIONS
    DEBUG_VEC2(DUAL_Vec2_Sub(v0, v1))
    DEBUG_VEC2(DUAL_Vec2_Sub(v0, v2))
    DEBUG_VEC2(DUAL_Vec2_Sub(v0, v3))
    DEBUG_VEC2(DUAL_Vec2_Sub(v0, v4))
    DEBUG_VEC2(DUAL_Vec2_Sub(v1, v2))
    DEBUG_VEC2(DUAL_Vec2_Sub(v1, v3))
    DEBUG_VEC2(DUAL_Vec2_Sub(v1, v4))
    DEBUG_VEC2(DUAL_Vec2_Sub(v2, v3))
    DEBUG_VEC2(DUAL_Vec2_Sub(v2, v4))
    DEBUG_VEC2(DUAL_Vec2_Sub(v3, v4))

    // SCALE
    DEBUG_VEC2(DUAL_Vec2_Scale(v0, 10.0))
    DEBUG_VEC2(DUAL_Vec2_Scale(v1, 10.0))
    DEBUG_VEC2(DUAL_Vec2_Scale(v2, 10.0))
    DEBUG_VEC2(DUAL_Vec2_Scale(v3, 10.0))
    DEBUG_VEC2(DUAL_Vec2_Scale(v4, 10.0))
    DEBUG_VEC2(DUAL_Vec2_Scale(v0, -10.0))
    DEBUG_VEC2(DUAL_Vec2_Scale(v1, -10.0))
    DEBUG_VEC2(DUAL_Vec2_Scale(v2, -10.0))
    DEBUG_VEC2(DUAL_Vec2_Scale(v3, -10.0))
    DEBUG_VEC2(DUAL_Vec2_Scale(v4, -10.0))

    // LENGTH
    DEBUG_FLOAT(DUAL_Vec2_Length(v0))
    DEBUG_FLOAT(DUAL_Vec2_Length(v1))
    DEBUG_FLOAT(DUAL_Vec2_Length(v2))
    DEBUG_FLOAT(DUAL_Vec2_Length(v3))
    DEBUG_FLOAT(DUAL_Vec2_Length(v4))

    // NORMALIZE
    DEBUG_VEC2(DUAL_Vec2_Normalize(v0))
    DEBUG_VEC2(DUAL_Vec2_Normalize(v1))
    DEBUG_VEC2(DUAL_Vec2_Normalize(v2))
    DEBUG_VEC2(DUAL_Vec2_Normalize(v3))
    DEBUG_VEC2(DUAL_Vec2_Normalize(v4))

    // DOT
    DEBUG_FLOAT(DUAL_Vec2_Dot(v0, v1))
    DEBUG_FLOAT(DUAL_Vec2_Dot(v0, v2))
    DEBUG_FLOAT(DUAL_Vec2_Dot(v0, v3))
    DEBUG_FLOAT(DUAL_Vec2_Dot(v0, v4))
    DEBUG_FLOAT(DUAL_Vec2_Dot(v1, v2))
    DEBUG_FLOAT(DUAL_Vec2_Dot(v1, v3))
    DEBUG_FLOAT(DUAL_Vec2_Dot(v1, v4))
    DEBUG_FLOAT(DUAL_Vec2_Dot(v2, v3))
    DEBUG_FLOAT(DUAL_Vec2_Dot(v2, v4))
    DEBUG_FLOAT(DUAL_Vec2_Dot(v3, v4))

    // DOT
    DEBUG_FLOAT(DUAL_Vec2_Distance(v0, v1))
    DEBUG_FLOAT(DUAL_Vec2_Distance(v0, v2))
    DEBUG_FLOAT(DUAL_Vec2_Distance(v0, v3))
    DEBUG_FLOAT(DUAL_Vec2_Distance(v0, v4))
    DEBUG_FLOAT(DUAL_Vec2_Distance(v1, v2))
    DEBUG_FLOAT(DUAL_Vec2_Distance(v1, v3))
    DEBUG_FLOAT(DUAL_Vec2_Distance(v1, v4))
    DEBUG_FLOAT(DUAL_Vec2_Distance(v2, v3))
    DEBUG_FLOAT(DUAL_Vec2_Distance(v2, v4))
    DEBUG_FLOAT(DUAL_Vec2_Distance(v3, v4))
}

void test_collisions() {
       // RECT / RECT (2D)
       DUAL_Rect rectA = { 0.0f, 0.0f, 10.0f, 10.0f };
       DUAL_Rect rectB_touch = { 5.0f, 5.0f, 10.0f, 10.0f };
       DUAL_Rect rectC_miss = { 20.0f, 20.0f, 5.0f, 5.0f };

       DEBUG_BOOL(DUAL_CollideRectRect(rectA, rectB_touch))
       DEBUG_BOOL(DUAL_CollideRectRect(rectA, rectC_miss))

       // CIRCLE / CIRCLE (2D)
       DUAL_Circle circA = { { 0.0f, 0.0f }, 5.0f };
       DUAL_Circle circB_touch = { { 6.0f, 0.0f }, 3.0f };
       DUAL_Circle circC_miss = { { 10.0f, 10.0f }, 2.0f };

       DEBUG_BOOL(DUAL_CollideCircleCircle(circA, circB_touch))
       DEBUG_BOOL(DUAL_CollideCircleCircle(circA, circC_miss))

       // POINT / RECT (2D)
       DUAL_Rect targetRect = { 10.0f, 10.0f, 50.0f, 30.0f };
       DUAL_Vec2 pt_inside = { 25.0f, 20.0f };
       DUAL_Vec2 pt_outside = { 5.0f, 12.0f };

       DEBUG_BOOL(DUAL_CollidePointRect(pt_inside, targetRect))
       DEBUG_BOOL(DUAL_CollidePointRect(pt_outside, targetRect))

       // AABB / AABB (3D)
       DUAL_AABB aabbA = { { 0.0f, 0.0f, 0.0f }, { 2.0f, 2.0f, 2.0f } };
       DUAL_AABB aabbB_touch = { { 1.0f, 1.0f, 1.0f }, { 3.0f, 3.0f, 3.0f } };
       DUAL_AABB aabbC_miss = { { 5.0f, 0.0f, 0.0f }, { 7.0f, 2.0f, 2.0f } };

       DEBUG_BOOL(DUAL_CollideAABBAABB(aabbA, aabbB_touch))
       DEBUG_BOOL(DUAL_CollideAABBAABB(aabbA, aabbC_miss))

       // SPHERE / SPHERE (3D)
       DUAL_Sphere sphA = { { 0.0f, 0.0f, 0.0f }, 4.0f };
       DUAL_Sphere sphB_touch = { { 3.0f, 0.0f, 4.0f }, 2.0f };
       DUAL_Sphere sphC_miss = { { 10.0f, 0.0f, 0.0f }, 1.0f };

       DEBUG_BOOL(DUAL_CollideSphereSphere(sphA, sphB_touch))
       DEBUG_BOOL(DUAL_CollideSphereSphere(sphA, sphC_miss))
   }

int main() {
    DUAL_Log(DUAL_LOG_INFO, "Test Math started !");

    test_vec2();
    test_vec3();
    test_collisions();

    return EXIT_SUCCESS;
}
