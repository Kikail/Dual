//
// Created by killian on 9/29/26.
//

#ifndef DUAL_DUAL_SCENE_LOADER_H
#define DUAL_DUAL_SCENE_LOADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "../DUAL_Physics/dual_physics.h"
#include "../DUAL_Math/dual_math.h"
#include "../DUAL_Graphics/dual_graphics_3d.h"

// Ajouter du contenu dans cette enum pour ajouter des nouveaux composants pour les entity
// Une entity peut etre que de un seul type pour faciliter la manipulation
typedef enum DUAL_Editor_EntityType {
    NONE,
    MODEL,
    COLLISION,
    STRUCT
}DUAL_Editor_EntityType;

// Cette structure contient les infos necessaire au chargement d'un modele 3D dans la scene
typedef struct DUAL_Editor_ModelData {
    char name[16];
    char path[128];
}DUAL_Editor_ModelData;

// Cette structure contient les infos necessaire a la manipulation de collision dans la scene
typedef struct DUAL_Editor_CollisionData {
    DUAL_ColliderType type;
    union{
        DUAL_AABB aabb;
        DUAL_Sphere sphere;
    }CollisionDataInfo;
}DUAL_Editor_CollisionData;

// Cette structure va permettre d'indiquer au moteur de jeu ou sont les spawners des enemies ou autre
// Cette structure permet de donner des infos diverses et variees
typedef struct DUAL_Editor_StructData {
    char targetname[32];
    char script_string[32];
    char script_noteworthy[32];
}DUAL_Editor_StructData;

// Structure principale de la scene, notre scene va etre composee de plein de Entity avec des types differents
// pour s'y retrouver facilement une fois dans le moteur
typedef struct DUAL_Editor_SceneEntity {
    DUAL_Editor_EntityType type;
    DUAL_Transform3D transform;
    char name[64];
    unsigned int id;

    // Partie qui concerne les infos sur le composant, nous stockons tout dans un tableau d'octets pour faciliter
    // la sauvegarde et chargement sur des fichiers
    unsigned int blockSize;
    char* block;
    void* data_ptr;
}DUAL_Editor_SceneEntity;

// Structure tres simple ou nous stockons uniquement des Entity
typedef struct DUAL_Editor_Scene {
    GList* entities;
    unsigned int numEntities;
}DUAL_Editor_Scene;


// MACROS
#define WRITE_DATA_BUFFER(data, type, buffer, buffer_offset)\
memcpy(buffer + buffer_offset, &data, sizeof(type));\
buffer_offset += sizeof(type);

#define GET_DATA_BUFFER(data, type, buffer, buffer_offset)\
memcpy(data, buffer + buffer_offset, sizeof(type));\
buffer_offset += sizeof(type);

#define SCENE_MAX_ENTITIES 16

// TODO Corriger les fuites memoire

// FONCTIONS
// Debug
void print_buffer_hex(const char* buffer, unsigned int size);
void afficher_type(DUAL_Editor_EntityType type);
void afficher_ModelData(DUAL_Editor_ModelData data);
void afficher_CollisionData(DUAL_Editor_CollisionData data);
void afficher_StructData(DUAL_Editor_StructData data);
void afficher_entite(DUAL_Editor_SceneEntity* entity);
void afficher_scene(DUAL_Editor_Scene* scene);


// Sauvegarde et Chargement
void sauvegarder_entite(DUAL_Editor_SceneEntity* entity, char* buffer, unsigned int* buffer_offset);
void sauvegarder_scene(DUAL_Editor_Scene* scene, char* buffer, unsigned int* buffer_offset);
DUAL_Editor_SceneEntity* charger_entite(char* buffer, unsigned int* buffer_offset);
void charger_scene(DUAL_Editor_Scene* scene, char* buffer, unsigned int* buffer_offset);
size_t entity_getTotalSize(DUAL_Editor_SceneEntity* entity);
size_t scene_getTotalSize(DUAL_Editor_Scene* scene);

// Creation de Entity pour la Scene
DUAL_Editor_SceneEntity* createEntity(DUAL_Editor_EntityType type, DUAL_Transform3D transform3_d, const char* name, void* blockData);
void addEntity(DUAL_Editor_Scene* scene,DUAL_Editor_SceneEntity* entity);
void removeEntity(DUAL_Editor_Scene* scene, DUAL_Editor_SceneEntity* entity);
























#endif //DUAL_DUAL_SCENE_LOADER_H
