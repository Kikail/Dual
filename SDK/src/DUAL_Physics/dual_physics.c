//
// Created by killian on 9/25/26.
//
#include "../../include/DUAL_Physics/dual_physics.h"

#include <stdlib.h>

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
    DUAL_Vec3 acceleration = DUAL_Vec3_Scale(body->forces, 1.0 / body->masse);
    acceleration = DUAL_Vec3_Add(acceleration, gravity);
    body->vitesse.x += acceleration.x * dt;
    body->vitesse.y += acceleration.y * dt;
    body->vitesse.z += acceleration.z * dt;
    body->position.x += body->vitesse.x * dt;
    body->position.y += body->vitesse.y * dt;
    body->position.z += body->vitesse.z * dt;
    body->forces = (DUAL_Vec3){0.0,0.0,0.0};
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
    // On calcule toutes les positions
    for (int i = 0; i < world->count_bodies; i++) {
        DUAL_RigidBody* body = &world->bodies[i];
        if (body->est_statique) continue;
        DUAL_internal_PhysicsBody_Step(body, world->gravite, dt);
    }

    // On resout les collisions
    for (int i = 0; i < world->count_bodies; i++) {
        for (int j = i + 1; j < world->count_bodies; j++) {
            DUAL_RigidBody* body1 = &world->bodies[i];
            DUAL_RigidBody* body2 = &world->bodies[j];

            DUAL_CollisionInfo info;
            if (body1->collider.type == DUAL_COLLIDER_AABB && body2->collider.type == DUAL_COLLIDER_AABB) {
                DUAL_AABB new_AABB_1 = body1->collider.shape.aabb;
                new_AABB_1.min = DUAL_Vec3_Add(new_AABB_1.min, body1->position);
                new_AABB_1.max = DUAL_Vec3_Add(new_AABB_1.max, body1->position);

                DUAL_AABB new_AABB_2 = body2->collider.shape.aabb;
                new_AABB_2.min = DUAL_Vec3_Add(new_AABB_2.min, body2->position);
                new_AABB_2.max = DUAL_Vec3_Add(new_AABB_2.max, body2->position);

                info = DUAL_Collide_AABBVSAABB(&new_AABB_1, &new_AABB_2);
            }
            else if (body1->collider.type == DUAL_COLLIDER_AABB && body2->collider.type == DUAL_COLLIDER_SPHERE) {
                DUAL_AABB new_AABB_1 = body1->collider.shape.aabb;
                new_AABB_1.min = DUAL_Vec3_Add(new_AABB_1.min, body1->position);
                new_AABB_1.max = DUAL_Vec3_Add(new_AABB_1.max, body1->position);

                DUAL_Sphere new_Sphere_1 = body2->collider.shape.sphere;
                new_Sphere_1.centre = DUAL_Vec3_Add(new_Sphere_1.centre, body2->position);

                info = DUAL_Collide_SphereVSAABB(&new_Sphere_1, &new_AABB_1);
            }
            else if (body2->collider.type == DUAL_COLLIDER_AABB && body1->collider.type == DUAL_COLLIDER_SPHERE) {
                DUAL_AABB new_AABB_1 = body2->collider.shape.aabb;
                new_AABB_1.min = DUAL_Vec3_Add(new_AABB_1.min, body2->position);
                new_AABB_1.max = DUAL_Vec3_Add(new_AABB_1.max, body2->position);

                DUAL_Sphere new_Sphere_1 = body1->collider.shape.sphere;
                new_Sphere_1.centre = DUAL_Vec3_Add(new_Sphere_1.centre, body1->position);

                info = DUAL_Collide_SphereVSAABB(&new_Sphere_1, &new_AABB_1);
            }
            else if (body1->collider.type == DUAL_COLLIDER_SPHERE && body2->collider.type == DUAL_COLLIDER_SPHERE) {
                DUAL_Sphere new_Sphere_1 = body1->collider.shape.sphere;
                new_Sphere_1.centre = DUAL_Vec3_Add(new_Sphere_1.centre, body1->position);

                DUAL_Sphere new_Sphere_2 = body2->collider.shape.sphere;
                new_Sphere_2.centre = DUAL_Vec3_Add(new_Sphere_2.centre, body2->position);

                info = DUAL_Collide_SphereVSSphere(&new_Sphere_1, &new_Sphere_2);
            }
            else {
                // Pas defini pour le moment on garde ces states pour les capsules et height map
            }
            // Si on trouve une collision on les repousse
            if (info.collision) {
                // On les empeche de se chevaucher
                float total_mass = body1->masse + body2->masse;
                if (total_mass == 0.0f) return;
                float w_a = body2->masse / total_mass;
                float w_b = body1->masse / total_mass;
                body1->position = DUAL_Vec3_Sub(body1->position, DUAL_Vec3_Scale(info.normal, info.penetration * w_a));
                body2->position = DUAL_Vec3_Add(body2->position, DUAL_Vec3_Scale(info.normal, info.penetration * w_b));

                // On "inverse" la vitesse pour les repousser selon une restitution
                DUAL_Vec3 relative_velocity = DUAL_Vec3_Sub(body2->vitesse, body1->vitesse);
                float vel_along_normal = DUAL_Vec3_Dot(relative_velocity, info.normal);
                if (vel_along_normal == 0.0f) return;
                float e = 1.0; // restitution
                float inv_mass_sum = body1->masse_inverse + body2->masse_inverse;
                if (inv_mass_sum == 0.0f) return;
                float j = -(1.0f + e) * vel_along_normal;
                j /= inv_mass_sum;
                DUAL_Vec3 impulse = DUAL_Vec3_Scale(info.normal, j);

                if (!body1->est_statique) {
                    body1->vitesse = DUAL_Vec3_Sub(body1->vitesse, DUAL_Vec3_Scale(impulse, body1->masse_inverse));
                }
                if (!body2->est_statique) {
                    body2->vitesse = DUAL_Vec3_Add(body2->vitesse, DUAL_Vec3_Scale(impulse, body2->masse_inverse));
                }
            }
        }
    }
}
int32_t DUAL_PhysicsWorld_AddBody(DUAL_PhysicsWorld* world, DUAL_RigidBody body) {
    if (world->count_bodies < DUAL_PHYSICS_MAX_BODIES) {
        DUAL_RigidBody* world_body = &world->bodies[world->count_bodies];
        *world_body = body;
        world_body->masse_inverse = 1.0 / body.masse;
        world_body->forces = (DUAL_Vec3){0.0, 0.0, 0.0};
        world_body->actif = true;
        return world->count_bodies++;
    }
    else {
        for (int i = 0; i < DUAL_PHYSICS_MAX_BODIES; i++) {
            if (!world->bodies[i].actif) {
                world->bodies[i] = body;
                world->bodies[i].actif = true;
                world->bodies[i].masse_inverse = 1.0 / world->bodies[i].masse;
                return i;
            }
        }
    }
}
void DUAL_PhysicsWorld_RemoveBody(DUAL_PhysicsWorld* world, int32_t body_id) {
    world->bodies[body_id].actif = false;
}
void DUAL_PhysicsWorld_SetHeightmap(DUAL_PhysicsWorld* world, DUAL_Heightmap* terrain) {
    world->terrain = terrain;
}

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

void DUAL_Debug_PrintAABB(DUAL_AABB aabb) {
    DUAL_Log(DUAL_LOG_DEBUG, "min:(%f %f %f), max:(%f %f %f)", aabb.min.x, aabb.min.y, aabb.min.z, aabb.max.x, aabb.max.y, aabb.max.z);
}
void DUAL_Debug_PrintBody(DUAL_RigidBody* body) {
    DUAL_Log(DUAL_LOG_DEBUG, "position:(%f %f %f), vitesse(%f %f %f), forces(%f %f %f)", body->position.x, body->position.y, body->position.z, body->vitesse.x,body->vitesse.y,body->vitesse.z, body->forces.x,body->forces.y,body->forces.z);
}