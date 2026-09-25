//
// Created by killian on 9/25/26.
//
#include "../../include/DUAL_Physics/dual_physics.h"

DUAL_CollisionInfo DUAL_Collide_SphereVSSphere(const DUAL_Sphere* a, const DUAL_Sphere* b) {
    DUAL_CollisionInfo info;
    DUAL_Vec3 ba = DUAL_Vec3_Sub(b->centre, a->centre);
    float length = DUAL_Vec3_Length(ba);
    info.collision = length < a->rayon + b->rayon;
    info.normal = DUAL_Vec3_Normalize(ba);
    info.penetration = length - (a->rayon + b->rayon);
    return info;
}

DUAL_CollisionInfo DUAL_Collide_SphereVSAABB(const DUAL_Sphere* sphere, const DUAL_AABB* aabb) {
    DUAL_CollisionInfo info;

    DUAL_Vec3 p;
    // Clamping sur X
    if (sphere->centre.x < aabb->min.x) p.x = aabb->min.x;
    else if (sphere->centre.x > aabb->max.x) p.x = aabb->max.x;
    else p.x = sphere->centre.x;

    if (sphere->centre.y < aabb->min.y)p.y = aabb->min.y;
    else if (sphere->centre.y > aabb->max.y)p.y = aabb->max.y;
    else p.y = sphere->centre.y;

    if (sphere->centre.z < aabb->min.z)p.z = aabb->min.z;
    else if (sphere->centre.z > aabb->max.z)p.z = aabb->max.z;
    else p.z = sphere->centre.z;

    float distance = DUAL_Vec3_Distance(p, sphere->centre);
    if (distance* distance <= sphere->rayon * sphere->rayon) {
        info.collision = true;
        info.normal = DUAL_Vec3_Normalize(DUAL_Vec3_Sub(sphere->centre, p));
        info.penetration = sphere->rayon - distance;
    }
    else {
        info.collision = false;
    }

    return info;
}


DUAL_CollisionInfo DUAL_Collide_AABBVSAABB(const DUAL_AABB* a, const DUAL_AABB* b) {
    DUAL_CollisionInfo info;

    float offsetx = DUAL_Min(a->max.x, b->max.x) - DUAL_Max(a->min.x, b->min.x);
    if (offsetx < 0) {
        info.collision = false;
        return info;
    }

    float offsety = DUAL_Min(a->max.y, b->max.y) - DUAL_Max(a->min.y, b->min.y);
    if (offsety < 0) {
        info.collision = false;
        return info;
    }

    float offsetz = DUAL_Min(a->max.z, b->max.z) - DUAL_Max(a->min.z, b->min.z);
    if (offsetz < 0) {
        info.collision = false;
        return info;
    }

    info.collision = true;
    if (offsetx < offsety && offsetx < offsetz) {
        info.normal = (DUAL_Vec3){1.0f, 0.0f, 0.0f};
        info.penetration = offsetx;
    }
    else if (offsety < offsetz && offsety < offsetx) {
        info.normal = (DUAL_Vec3){0.0f, 1.0f, 0.0f};
        info.penetration = offsety;
    }
    else if (offsetz < offsetx && offsetz < offsety) {
        info.normal = (DUAL_Vec3){0.0f, 0.0f, 1.0f};
        info.penetration = offsetz;
    }
    else {
        info.normal = (DUAL_Vec3){0.0f, 0.0f, 0.0f};
        info.penetration = 0.0;
    }

    return info;
}