#ifndef DUAL_PHYSICS_H
#define DUAL_PHYSICS_H

#include <stdbool.h>
#include <stdint.h>
#include "../DUAL_Math/dual_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    DUAL_Vec3 bas;
    DUAL_Vec3 haut;
    float rayon;
} DUAL_Capsule;

// Heightmap pour les terrains de l'environnement
typedef struct {
    float* data_hauteurs;  // Tableau 1D de dimensions (largeur * profondeur)
    uint32_t largeur;      // Nombre de points sur l'axe X
    uint32_t profondeur;   // Nombre de points sur l'axe Z
    DUAL_Vec3 echelle;     // Échelle du terrain (ex: 1.0m entre chaque point, hauteur max)
    DUAL_Vec3 origine;     // Position du coin bas-gauche du terrain dans le monde
} DUAL_Heightmap;

// Types de colliders gérés par le moteur
typedef enum {
    DUAL_COLLIDER_NONE = 0,
    DUAL_COLLIDER_AABB,
    DUAL_COLLIDER_SPHERE,
    DUAL_COLLIDER_CAPSULE
} DUAL_ColliderType;

typedef struct {
    DUAL_ColliderType type;
    union {
        DUAL_AABB aabb;
        DUAL_Sphere sphere;
        DUAL_Capsule capsule;
    } shape;
} DUAL_Collider;


typedef struct {
    DUAL_Vec3 position;
    DUAL_Vec3 vitesse;
    DUAL_Vec3 forces;
    
    float masse;         // 0.0f = Objet statique (masse infinie)
    float masse_inverse; // 1.0f / masse (précalculé pour éviter les divisions)
    float amortissement; // Damping (ex: 0.98f pour simuler la friction de l'air)
    
    DUAL_Collider collider;
    bool est_statique;
    bool actif;
} DUAL_RigidBody;

// Informations renvoyées lors d'une collision
typedef struct {
    bool collision;
    DUAL_Vec3 normal;    // Direction de repousse
    float penetration;   // Profondeur du chevauchement
    DUAL_Vec3 point;     // Point d'impact dans le monde
} DUAL_CollisionInfo;

typedef struct {
    DUAL_RigidBody* bodies;
    uint32_t max_bodies;
    uint32_t count_bodies;

    DUAL_Heightmap* terrain; // Heightmap globale de l'environnement (optionnelle)
    DUAL_Vec3 gravite;       // Ex: {0.0f, -9.81f, 0.0f}
} DUAL_PhysicsWorld;


DUAL_PhysicsWorld* DUAL_PhysicsWorld_Create(uint32_t max_bodies, DUAL_Vec3 gravite);
void DUAL_PhysicsWorld_Destroy(DUAL_PhysicsWorld* world);

// Avancement de la simulation (Intégration d'Euler / Verlet rapide)
void DUAL_PhysicsWorld_Step(DUAL_PhysicsWorld* world, float dt);

// Attribuer une Heightmap au monde
void DUAL_PhysicsWorld_SetHeightmap(DUAL_PhysicsWorld* world, DUAL_Heightmap* terrain);

// Ajout/Suppression de bodies dans le monde
int32_t DUAL_PhysicsWorld_AddBody(DUAL_PhysicsWorld* world, DUAL_RigidBody body);
void DUAL_PhysicsWorld_RemoveBody(DUAL_PhysicsWorld* world, int32_t body_id);

// Intersections de primitives de base
DUAL_CollisionInfo DUAL_Collide_SphereVSSphere(const DUAL_Sphere* a, const DUAL_Sphere* b);
DUAL_CollisionInfo DUAL_Collide_AABBVSAABB(const DUAL_AABB* a, const DUAL_AABB* b);
DUAL_CollisionInfo DUAL_Collide_SphereVSAABB(const DUAL_Sphere* sphere, const DUAL_AABB* aabb);
DUAL_CollisionInfo DUAL_Collide_CapsuleVSSphere(const DUAL_Capsule* capsule, const DUAL_Sphere* sphere);
DUAL_CollisionInfo DUAL_Collide_CapsuleVSAABB(const DUAL_Capsule* capsule, const DUAL_AABB* aabb);

// Heightmap : Test d'altitude instantané (O(1))
float DUAL_Heightmap_GetHeightAt(const DUAL_Heightmap* terrain, float world_x, float world_z);
DUAL_Vec3 DUAL_Heightmap_GetNormalAt(const DUAL_Heightmap* terrain, float world_x, float world_z);
DUAL_CollisionInfo DUAL_Collide_SphereVSHeightmap(const DUAL_Sphere* sphere, const DUAL_Heightmap* terrain);

typedef struct {
    DUAL_Vec3 origine;
    DUAL_Vec3 direction; // Normée
    float longueur_max;
} DUAL_Ray;

typedef struct {
    bool touche;
    float distance;
    DUAL_Vec3 point;
    DUAL_Vec3 normal;
    int32_t body_id;
} DUAL_RaycastHit;

DUAL_RaycastHit DUAL_Raycast_Sphere(const DUAL_Ray* ray, const DUAL_Sphere* sphere);
DUAL_RaycastHit DUAL_Raycast_AABB(const DUAL_Ray* ray, const DUAL_AABB* aabb);
DUAL_RaycastHit DUAL_Raycast_Capsule(const DUAL_Ray* ray, const DUAL_Capsule* capsule);
DUAL_RaycastHit DUAL_Raycast_Heightmap(const DUAL_Ray* ray, const DUAL_Heightmap* terrain);

#ifdef __cplusplus
}
#endif

#endif // DUAL_PHYSICS_H