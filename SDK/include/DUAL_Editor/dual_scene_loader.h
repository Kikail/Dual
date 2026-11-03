//
// Created by killian on 9/29/26.
//

#ifndef DUAL_DUAL_SCENE_LOADER_H
#define DUAL_DUAL_SCENE_LOADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../DUAL_Physics/dual_physics.h"
#include "../DUAL_Math/dual_math.h"
#include "../DUAL_Graphics/3d/dual_graphics_3d.h"
#include "../DUAL_Core/dual_enums.h"
#include "dynamic-array/dynamic-array.h"

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
    DynamicArray* entities;
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
void DUAL_Editor_Print_Buffer_Hex(const char* buffer, unsigned int size);
void DUAL_Editor_Afficher_Type(DUAL_Editor_EntityType type);
void DUAL_Editor_Afficher_ModelData(DUAL_Editor_ModelData data);
void DUAL_Editor_Afficher_CollisionData(DUAL_Editor_CollisionData data);
void DUAL_Editor_Afficher_StructData(DUAL_Editor_StructData data);
void DUAL_Editor_Afficher_Entity(DUAL_Editor_SceneEntity* entity);
void DUAL_Editor_Afficher_Scene(DUAL_Editor_Scene* scene);


// Sauvegarde et Chargement
void DUAL_Editor_Sauvegarder_Entity(DUAL_Editor_SceneEntity* entity, char* buffer, unsigned int* buffer_offset);
void DUAL_Editor_Sauvegarder_Scene(DUAL_Editor_Scene* scene, char* buffer, unsigned int* buffer_offset);
DUAL_Editor_SceneEntity* DUAL_Editor_Charger_Entity(char* buffer, unsigned int* buffer_offset);
void DUAL_Editor_Charger_Scene(DUAL_Editor_Scene* scene, char* buffer, unsigned int* buffer_offset);
size_t DUAL_Editor_Entity_GetTotalSize(DUAL_Editor_SceneEntity* entity);
size_t DUAL_Editor_Scene_GetTotalSize(DUAL_Editor_Scene* scene);

// Creation de Entity pour la Scene
DUAL_Editor_SceneEntity* DUAL_Editor_CreateEntity(DUAL_Editor_EntityType type, DUAL_Transform3D transform3_d, const char* name, void* blockData);
DUAL_Editor_SceneEntity* DUAL_Editor_GetEntity(DUAL_Editor_Scene* scene, unsigned int index);
void DUAL_Editor_AddEntity(DUAL_Editor_Scene* scene,DUAL_Editor_SceneEntity* entity);
void DUAL_Editor_RemoveEntity(DUAL_Editor_Scene* scene, DUAL_Editor_SceneEntity* entity);
























#endif //DUAL_DUAL_SCENE_LOADER_H
