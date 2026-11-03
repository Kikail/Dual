//
// Created by killian on 10/5/26.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/DUAL_Editor/dual_scene_loader.h"

//////////////////////////////////////////////////////////////
/// Debuging
//////////////////////////////////////////////////////////////

void DUAL_Editor_Print_Buffer_Hex(const char* buffer, unsigned int size) {
    printf("Buffer hex (%u octets) : \t", size);
    for (unsigned int i = 0; i < size; i++) {
        printf("%02X ", (unsigned char)buffer[i]);
    }
    printf("\n");
}

void DUAL_Editor_Afficher_Type(DUAL_Editor_EntityType type) {
    switch (type) {
        case DUAL_ENTITY_TYPE_NONE:
            printf("NONE");
            break;
        case DUAL_ENTITY_TYPE_MODEL:
            printf("MODEL");
            break;
        case DUAL_ENTITY_TYPE_COLLISION:
            printf("COLLISION");
            break;
        case DUAL_ENTITY_TYPE_STRUCT:
            printf("STRUCT");
            break;
    }
}

void DUAL_Editor_Afficher_ModelData(DUAL_Editor_ModelData data) {
    printf("\t\tModelData:{name:%s, path:%s}\n", data.name, data.path);
}

void DUAL_Editor_Afficher_CollisionData(DUAL_Editor_CollisionData data) {
    printf("\t\t");
    switch (data.type) {
        case DUAL_COLLIDER_AABB:
            printf("AABB:{min:(%f, %f, %f),max:(%f, %f, %f)}\n", data.CollisionDataInfo.aabb.min.x, data.CollisionDataInfo.aabb.min.y,data.CollisionDataInfo.aabb.min.z, data.CollisionDataInfo.aabb.max.x, data.CollisionDataInfo.aabb.max.y, data.CollisionDataInfo.aabb.max.z);
            break;
        case DUAL_COLLIDER_SPHERE:
            printf("Sphere:{center(%f, %f, %f), radius:%f)\n", data.CollisionDataInfo.sphere.centre.x, data.CollisionDataInfo.sphere.centre.y, data.CollisionDataInfo.sphere.centre.z, data.CollisionDataInfo.sphere.rayon);
            break;
        case DUAL_COLLIDER_NONE:
            break;
    }
}

void DUAL_Editor_Afficher_StructData(DUAL_Editor_StructData data) {
    printf("\t\tStructData:{targetname:%s, script_string:%s, script_noteworthy:%s}\n", data.targetname, data.script_string, data.script_noteworthy);
}

void DUAL_Editor_Afficher_Entity(DUAL_Editor_SceneEntity* entity) {
    printf("\t-Entity{type:");
    DUAL_Editor_Afficher_Type(entity->type);
    printf(", position:(%f, %f, %f), name:%s, id:%u, blocksize:%u}\n", entity->transform.position.x, entity->transform.position.y, entity->transform.position.z, entity->name, entity->id, entity->blockSize);
    switch (entity->type) {
        case DUAL_ENTITY_TYPE_MODEL:
            DUAL_Editor_ModelData model; unsigned int offset = 0;
            GET_DATA_BUFFER(&model, DUAL_Editor_ModelData, entity->block, offset);
            DUAL_Editor_Afficher_ModelData(model);
            break;
        case DUAL_ENTITY_TYPE_COLLISION:
            DUAL_Editor_CollisionData collision; unsigned int offsetcol = 0;
            GET_DATA_BUFFER(&collision, DUAL_Editor_CollisionData, entity->block, offsetcol);
            DUAL_Editor_Afficher_CollisionData(collision);
            break;
        case DUAL_ENTITY_TYPE_STRUCT:
            DUAL_Editor_StructData structData; unsigned int offsetstruct = 0;
            GET_DATA_BUFFER(&structData, DUAL_Editor_StructData, entity->block, offsetstruct);
            DUAL_Editor_Afficher_StructData(structData);
            break;
        case DUAL_ENTITY_TYPE_NONE:
            break;
    }
}

void DUAL_Editor_Afficher_Scene(DUAL_Editor_Scene* scene) {
    if (scene == NULL) return;
    printf("Scene(%u entities):\n",scene->numEntities);
    for (int i = 0; i < scene->entities->size; i++) {
        DUAL_Editor_SceneEntity* entity = DynamicArray_Get(scene->entities, i);
        if (entity == NULL) continue;
        DUAL_Editor_Afficher_Entity(entity);
    }
}

//////////////////////////////////////////////////////////////
/// Modification de la scene et des entities
//////////////////////////////////////////////////////////////


DUAL_Editor_SceneEntity* DUAL_Editor_CreateEntity(DUAL_Editor_EntityType type, DUAL_Transform3D transform3_d, const char* name, void* blockData) {
    // Allocation de l'entité sur le tas (heap)
    DUAL_Editor_SceneEntity* entity = malloc(sizeof(DUAL_Editor_SceneEntity));
    if (!entity) return NULL;

    entity->type = type;
    entity->transform = transform3_d;
    strcpy(entity->name, name);
    entity->id = 0; // Ou un ID unique si tu en gères un
    entity->blockSize = 0;

    switch (type) {
        case DUAL_ENTITY_TYPE_NONE:
            entity->block = NULL;
            entity->data_ptr = NULL;
            break;
        case DUAL_ENTITY_TYPE_MODEL: {
            DUAL_Editor_ModelData* model_data = (DUAL_Editor_ModelData*)blockData;
            entity->block = malloc(sizeof(DUAL_Editor_ModelData));
            WRITE_DATA_BUFFER(*model_data, DUAL_Editor_ModelData, entity->block, entity->blockSize);
            entity->data_ptr = blockData;
            break;
        }
        case DUAL_ENTITY_TYPE_COLLISION: {
            DUAL_Editor_CollisionData* col_data = (DUAL_Editor_CollisionData*)blockData;
            entity->block = malloc(sizeof(DUAL_Editor_CollisionData));
            WRITE_DATA_BUFFER(*col_data, DUAL_Editor_CollisionData, entity->block, entity->blockSize);
            entity->data_ptr = blockData;
            break;
        }
        case DUAL_ENTITY_TYPE_STRUCT: {
            DUAL_Editor_StructData* struct_data = (DUAL_Editor_StructData*)blockData;
            entity->block = malloc(sizeof(DUAL_Editor_StructData));
            WRITE_DATA_BUFFER(*struct_data, DUAL_Editor_StructData, entity->block, entity->blockSize);
            entity->data_ptr = blockData;
            break;
        }
    }
    return entity;
}

DUAL_Editor_SceneEntity* DUAL_Editor_GetEntity(DUAL_Editor_Scene* scene, unsigned int index) {
    return DynamicArray_Get(scene->entities, index);
}


void DUAL_Editor_AddEntity(DUAL_Editor_Scene* scene,DUAL_Editor_SceneEntity* entity) {
    DynamicArray_Put(scene->entities, entity);
    scene->numEntities += 1;
}

//////////////////////////////////////////////////////////////
/// Sauvegarde de la scene et des entities
//////////////////////////////////////////////////////////////

void DUAL_Editor_Sauvegarder_Entity(DUAL_Editor_SceneEntity* entity, char* buffer, unsigned int* buffer_offset) {
    if (entity == NULL) {
        DUAL_Log(DUAL_LOG_ERROR, "Entity is null");
        return;
    }
    // Important de sauvegarder dans l'ordre pour ne pas se perdre plus tard
    memcpy(buffer + *buffer_offset, &entity->type, sizeof(DUAL_Editor_EntityType)); *buffer_offset += sizeof(DUAL_Editor_EntityType);
    memcpy(buffer + *buffer_offset, &entity->transform, sizeof(DUAL_Transform3D)); *buffer_offset += sizeof(DUAL_Transform3D);
    memcpy(buffer + *buffer_offset, entity->name, 64); *buffer_offset += 64;
    memcpy(buffer + *buffer_offset, &entity->id, sizeof(unsigned int)); *buffer_offset += sizeof(unsigned int);
    memcpy(buffer + *buffer_offset, &entity->blockSize, sizeof(unsigned int)); *buffer_offset += sizeof(unsigned int);
    memcpy(buffer + *buffer_offset, entity->block, entity->blockSize); *buffer_offset += entity->blockSize;
}

void DUAL_Editor_Sauvegarder_Scene(DUAL_Editor_Scene* scene, char* buffer, unsigned int* buffer_offset) {
    DUAL_Editor_Print_Buffer_Hex(buffer, *buffer_offset);
    memcpy(buffer, &scene->numEntities, sizeof(unsigned int)); *buffer_offset += 4;
    DUAL_Editor_Print_Buffer_Hex(buffer, *buffer_offset);
    for (int i = 0; i < scene->entities->size; i++) {
        DUAL_Editor_SceneEntity* entity = DynamicArray_Get(scene->entities, i);
        DUAL_Editor_Sauvegarder_Entity(entity, buffer, buffer_offset);
        DUAL_Editor_Print_Buffer_Hex(buffer, *buffer_offset);
    }
}

//////////////////////////////////////////////////////////////
/// Chargement de la scene et des entities
//////////////////////////////////////////////////////////////

DUAL_Editor_SceneEntity* DUAL_Editor_Charger_Entity(char* buffer, unsigned int* buffer_offset) {
    DUAL_Editor_SceneEntity* entity = malloc(sizeof(DUAL_Editor_SceneEntity));
    memcpy(&entity->type, buffer + *buffer_offset, sizeof(DUAL_Editor_EntityType)); *buffer_offset += sizeof(DUAL_Editor_EntityType);
    memcpy(&entity->transform, buffer + *buffer_offset, sizeof(DUAL_Transform3D)); *buffer_offset += sizeof(DUAL_Transform3D);
    memcpy(entity->name, buffer + *buffer_offset, 64); *buffer_offset += 64;
    memcpy(&entity->id, buffer + *buffer_offset, sizeof(unsigned int)); *buffer_offset += sizeof(unsigned int);
    memcpy(&entity->blockSize, buffer + *buffer_offset, sizeof(unsigned int)); *buffer_offset += sizeof(unsigned int);
    entity->block = malloc(entity->blockSize);
    memcpy(entity->block, buffer + *buffer_offset, entity->blockSize); *buffer_offset += entity->blockSize;
    return entity;
}

void DUAL_Editor_Charger_Scene(DUAL_Editor_Scene* scene, char* buffer, unsigned int* buffer_offset) {
    if (scene == NULL || buffer == NULL || buffer_offset == NULL) return;

    memcpy(&scene->numEntities, buffer + *buffer_offset, sizeof(unsigned int)); *buffer_offset += sizeof(unsigned int);
    scene->entities = DynamicArray_CreateEmptyWithCapacity(64);

    for (unsigned int i = 0; i < scene->numEntities; i++) {
        DynamicArray_Put(scene->entities, DUAL_Editor_Charger_Entity(buffer, buffer_offset));
    }
}

//////////////////////////////////////////////////////////////
/// Fonctions utilitaires pour la scene et les entities
//////////////////////////////////////////////////////////////


size_t DUAL_Editor_Entity_GetTotalSize(DUAL_Editor_SceneEntity* entity) {
    size_t size = 0;
    size += sizeof(DUAL_Editor_EntityType);
    size += sizeof(DUAL_Transform3D);
    size += 64; // le nom fait 64 caracteres et 1 caractere = 1 octet
    size += sizeof(unsigned int);
    size += sizeof(unsigned int);
    size += entity->blockSize;
    return size;
}

size_t DUAL_Editor_Scene_GetTotalSize(DUAL_Editor_Scene* scene) {
    size_t total = sizeof(unsigned int);
    for (int i = 0; i < scene->entities->size; i++) {
        DUAL_Editor_SceneEntity* entity = DynamicArray_Get(scene->entities, i);
        total += DUAL_Editor_Entity_GetTotalSize(entity);
    }
    return total;
}
void DUAL_Editor_RemoveEntity(DUAL_Editor_Scene* scene, DUAL_Editor_SceneEntity* entity) {
    if (scene == NULL || entity == NULL) return;

    unsigned int i = 0;
    while (i < scene->entities->size) {
        if (entity == DynamicArray_Get(scene->entities, i)) {
            DynamicArray_RemoveAt(scene->entities, i);
        }
    }

    if (scene->numEntities > 0) {
        scene->numEntities--;
    }
    if (entity->block != NULL) {
        free(entity->block);
    }
    free(entity);
}

