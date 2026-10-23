//
// Created by killian on 9/25/26.
//
#include "../../include/DUAL_Physics/dual_physics.h"
#include <stdlib.h>
#include <math.h>
#include "DUAL_Core/dual_core.h"

DUAL_PhysicsWorld* DUAL_PhysicsWorld_Create(DUAL_Vec3 gravite) {
    DUAL_PhysicsWorld* world = malloc(sizeof(DUAL_PhysicsWorld));
    world->count_bodies = 0;
    world->gravite = gravite;
    world->terrain = NULL;
    return world;
}

void DUAL_PhysicsWorld_Destroy(DUAL_PhysicsWorld* world) {
    free(world);
}

void DUAL_internal_PhysicsBody_Step(DUAL_RigidBody* body, DUAL_Vec3 gravity, float dt) {
    if (body->est_statique || !body->actif) return;

    DUAL_Vec3 acceleration = DUAL_Vec3_Scale(body->forces, body->masse_inverse);
    acceleration = DUAL_Vec3_Add(acceleration, gravity);

    body->vitesse.x += acceleration.x * dt;
    body->vitesse.y += acceleration.y * dt;
    body->vitesse.z += acceleration.z * dt;

    body->position.x += body->vitesse.x * dt;
    body->position.y += body->vitesse.y * dt;
    body->position.z += body->vitesse.z * dt;

    body->forces = (DUAL_Vec3){0.0f, 0.0f, 0.0f};
}

DUAL_AABB DUAL_GetGlobalAABB(DUAL_RigidBody* body) {
    DUAL_AABB new_AABB = body->collider.shape.aabb;
    new_AABB.min = DUAL_Vec3_Add(new_AABB.min, body->position);
    new_AABB.max = DUAL_Vec3_Add(new_AABB.max, body->position);
    return new_AABB;
}

DUAL_Sphere DUAL_GetGlobalSphere(DUAL_RigidBody* body) {
    DUAL_Sphere sphere = body->collider.shape.sphere;
    sphere.centre = DUAL_Vec3_Add(sphere.centre, body->position);
    return sphere;
}

void DUAL_PhysicsWorld_Step(DUAL_PhysicsWorld* world, float dt) {
    // 1. Intégration du mouvement
    for (int i = 0; i < world->count_bodies; i++) {
        DUAL_RigidBody* body = &world->bodies[i];
        DUAL_internal_PhysicsBody_Step(body, world->gravite, dt);
    }

    // 2. Détection et résolution des collisions
    for (int i = 0; i < world->count_bodies; i++) {
        for (int j = i + 1; j < world->count_bodies; j++) {
            DUAL_RigidBody* body1 = &world->bodies[i];
            DUAL_RigidBody* body2 = &world->bodies[j];

            if (!body1->actif || !body2->actif) continue;
            if (body1->est_statique && body2->est_statique) continue;

            DUAL_CollisionInfo info = {0};

            // AABB vs AABB
            if (body1->collider.type == DUAL_COLLIDER_AABB && body2->collider.type == DUAL_COLLIDER_AABB) {
                DUAL_AABB box1 = DUAL_GetGlobalAABB(body1);
                DUAL_AABB box2 = DUAL_GetGlobalAABB(body2);
                info = DUAL_Collide_AABBVSAABB(&box1, &box2);
            }
            // AABB1 vs SPHERE2
            else if (body1->collider.type == DUAL_COLLIDER_AABB && body2->collider.type == DUAL_COLLIDER_SPHERE) {
                DUAL_AABB box1 = DUAL_GetGlobalAABB(body1);
                DUAL_Sphere sphere2 = DUAL_GetGlobalSphere(body2);
                info = DUAL_Collide_SphereVSAABB(&sphere2, &box1);
            }
            // SPHERE1 vs AABB2
            else if (body1->collider.type == DUAL_COLLIDER_SPHERE && body2->collider.type == DUAL_COLLIDER_AABB) {
                DUAL_Sphere sphere1 = DUAL_GetGlobalSphere(body1);
                DUAL_AABB box2 = DUAL_GetGlobalAABB(body2);
                info = DUAL_Collide_SphereVSAABB(&sphere1, &box2);
                info.normal = DUAL_Vec3_Scale(info.normal, -1.0f); // Inversion pour garder le bon sens body1 -> body2
            }
            // SPHERE vs SPHERE
            else if (body1->collider.type == DUAL_COLLIDER_SPHERE && body2->collider.type == DUAL_COLLIDER_SPHERE) {
                DUAL_Sphere sphere1 = DUAL_GetGlobalSphere(body1);
                DUAL_Sphere sphere2 = DUAL_GetGlobalSphere(body2);
                info = DUAL_Collide_SphereVSSphere(&sphere1, &sphere2);
            }

            if (info.collision && info.penetration > 0.0001f) {
                float inv_mass_a = body1->est_statique ? 0.0f : body1->masse_inverse;
                float inv_mass_b = body2->est_statique ? 0.0f : body2->masse_inverse;
                float total_inv_mass = inv_mass_a + inv_mass_b;

                if (total_inv_mass == 0.0f) continue;

                DUAL_Vec3 separation = DUAL_Vec3_Scale(info.normal, info.penetration);
                if (!body1->est_statique) {
                    body1->position = DUAL_Vec3_Sub(body1->position, DUAL_Vec3_Scale(separation, inv_mass_a / total_inv_mass));
                }
                if (!body2->est_statique) {
                    body2->position = DUAL_Vec3_Add(body2->position, DUAL_Vec3_Scale(separation, inv_mass_b / total_inv_mass));
                }

                DUAL_Vec3 relative_velocity = DUAL_Vec3_Sub(body2->vitesse, body1->vitesse);
                float vel_along_normal = DUAL_Vec3_Dot(relative_velocity, info.normal);

                // Ne pas résoudre si les objets s'éloignent déjà
                if (vel_along_normal > 0.0f) continue;

                float e = 0.5f; // Coefficient de restitution
                float j = -(1.0f + e) * vel_along_normal;
                j /= total_inv_mass;

                DUAL_Vec3 impulse = DUAL_Vec3_Scale(info.normal, j);

                if (!body1->est_statique) {
                    body1->vitesse = DUAL_Vec3_Sub(body1->vitesse, DUAL_Vec3_Scale(impulse, inv_mass_a));
                }
                if (!body2->est_statique) {
                    body2->vitesse = DUAL_Vec3_Add(body2->vitesse, DUAL_Vec3_Scale(impulse, inv_mass_b));
                }
            }
        }
    }
}

int32_t DUAL_PhysicsWorld_AddBody(DUAL_PhysicsWorld* world, DUAL_RigidBody body) {
    if (body.est_statique) {
        body.masse_inverse = 0.0f;
    } else {
        body.masse_inverse = (body.masse > 0.0f) ? (1.0f / body.masse) : 0.0f;
    }
    body.forces = (DUAL_Vec3){0.0f, 0.0f, 0.0f};
    body.actif = true;

    if (world->count_bodies < DUAL_PHYSICS_MAX_BODIES) {
        world->bodies[world->count_bodies] = body;
        return world->count_bodies++;
    } else {
        for (int i = 0; i < DUAL_PHYSICS_MAX_BODIES; i++) {
            if (!world->bodies[i].actif) {
                world->bodies[i] = body;
                return i;
            }
        }
    }
    return -1;
}

void DUAL_PhysicsWorld_RemoveBody(DUAL_PhysicsWorld* world, int32_t body_id) {
    if (body_id >= 0 && body_id < DUAL_PHYSICS_MAX_BODIES) {
        world->bodies[body_id].actif = false;
    }
}

void DUAL_PhysicsWorld_SetHeightmap(DUAL_PhysicsWorld* world, DUAL_Heightmap* terrain) {
    world->terrain = terrain;
}

DUAL_CollisionInfo DUAL_Collide_SphereVSSphere(const DUAL_Sphere* a, const DUAL_Sphere* b) {
    DUAL_CollisionInfo info = {0};
    DUAL_Vec3 ab = DUAL_Vec3_Sub(b->centre, a->centre);
    float dist_sq = DUAL_Vec3_Dot(ab, ab);
    float radius_sum = a->rayon + b->rayon;

    if (dist_sq < radius_sum * radius_sum) {
        float distance = sqrtf(dist_sq);
        info.collision = true;
        if (distance > 0.0001f) {
            info.normal = DUAL_Vec3_Scale(ab, 1.0f / distance);
        } else {
            info.normal = (DUAL_Vec3){0.0f, 1.0f, 0.0f};
        }
        info.penetration = radius_sum - distance;
    }
    return info;
}

DUAL_CollisionInfo DUAL_Collide_SphereVSAABB(const DUAL_Sphere* sphere, const DUAL_AABB* aabb) {
    DUAL_CollisionInfo info = {0};

    DUAL_Vec3 p;
    p.x = DUAL_Clamp(sphere->centre.x, aabb->min.x, aabb->max.x);
    p.y = DUAL_Clamp(sphere->centre.y, aabb->min.y, aabb->max.y);
    p.z = DUAL_Clamp(sphere->centre.z, aabb->min.z, aabb->max.z);

    DUAL_Vec3 diff = DUAL_Vec3_Sub(sphere->centre, p);
    float dist_sq = DUAL_Vec3_Dot(diff, diff);

    if (dist_sq <= sphere->rayon * sphere->rayon) {
        float distance = sqrtf(dist_sq);
        info.collision = true;
        info.penetration = sphere->rayon - distance;
        if (distance > 0.0001f) {
            info.normal = DUAL_Vec3_Scale(diff, 1.0f / distance);
        } else {
            info.normal = (DUAL_Vec3){0.0f, 1.0f, 0.0f};
        }
    }
    return info;
}

DUAL_CollisionInfo DUAL_Collide_AABBVSAABB(const DUAL_AABB* a, const DUAL_AABB* b) {
    DUAL_CollisionInfo info = {0};

    float overlap_x = DUAL_Min(a->max.x, b->max.x) - DUAL_Max(a->min.x, b->min.x);
    if (overlap_x <= 0.0f) return info;

    float overlap_y = DUAL_Min(a->max.y, b->max.y) - DUAL_Max(a->min.y, b->min.y);
    if (overlap_y <= 0.0f) return info;

    float overlap_z = DUAL_Min(a->max.z, b->max.z) - DUAL_Max(a->min.z, b->min.z);
    if (overlap_z <= 0.0f) return info;

    info.collision = true;

    // Trouver l'axe de plus petite pénétration (axe de séparation minimal)
    DUAL_Vec3 center_a = DUAL_Vec3_Scale(DUAL_Vec3_Add(a->min, a->max), 0.5f);
    DUAL_Vec3 center_b = DUAL_Vec3_Scale(DUAL_Vec3_Add(b->min, b->max), 0.5f);
    DUAL_Vec3 dir = DUAL_Vec3_Sub(center_b, center_a);

    if (overlap_x < overlap_y && overlap_x < overlap_z) {
        info.penetration = overlap_x;
        info.normal = (dir.x < 0.0f) ? (DUAL_Vec3){-1.0f, 0.0f, 0.0f} : (DUAL_Vec3){1.0f, 0.0f, 0.0f};
    } else if (overlap_y < overlap_z) {
        info.penetration = overlap_y;
        info.normal = (dir.y < 0.0f) ? (DUAL_Vec3){0.0f, -1.0f, 0.0f} : (DUAL_Vec3){0.0f, 1.0f, 0.0f};
    } else {
        info.penetration = overlap_z;
        info.normal = (dir.z < 0.0f) ? (DUAL_Vec3){0.0f, 0.0f, -1.0f} : (DUAL_Vec3){0.0f, 0.0f, 1.0f};
    }

    return info;
}

void DUAL_Debug_PrintAABB(DUAL_AABB aabb) {
    DUAL_Log(DUAL_LOG_DEBUG, "min:(%f %f %f), max:(%f %f %f)", aabb.min.x, aabb.min.y, aabb.min.z, aabb.max.x, aabb.max.y, aabb.max.z);
}

void DUAL_Debug_PrintBody(DUAL_RigidBody* body) {
    DUAL_Log(DUAL_LOG_DEBUG, "position:(%f %f %f), vitesse(%f %f %f), forces(%f %f %f)", body->position.x, body->position.y, body->position.z, body->vitesse.x, body->vitesse.y, body->vitesse.z, body->forces.x, body->forces.y, body->forces.z);
}