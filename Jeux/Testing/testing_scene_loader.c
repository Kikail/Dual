//
// Created by killian on 9/29/26.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../SDK/include/DUAL_Math/dual_math.h"
#include "../../SDK/include/dual_utils.h"
#include "../../SDK/include/DUAL_Physics/dual_physics.h"
#include "DUAL_FS/dual_fs.h"
#include "DUAL_Graphics/dual_graphics_3d.h"

void print_buffer_hex(const char* buffer, unsigned int size) {
    printf("Buffer hex (%u octets) : \t", size);
    for (unsigned int i = 0; i < size; i++) {
        printf("%02X ", (unsigned char)buffer[i]);
    }
    printf("\n");
}

#define SCENE_MAX_ENTITIES 16

#define PRINT_SIZE(x)\
printf("sizeof(");\
printf(#x);\
printf(") = %ld\n", sizeof(x))

#define WRITE_DATA_BUFFER(data, type, buffer, buffer_offset)\
memcpy(buffer + buffer_offset, &data, sizeof(type));\
buffer_offset += sizeof(type);

#define GET_DATA_BUFFER(data, type, buffer, buffer_offset)\
memcpy(data, buffer + buffer_offset, sizeof(type));\
buffer_offset += sizeof(type);

typedef enum EntityType {
    NONE,
    MODEL,
    COLLISION,
    STRUCT
}EntityType;
void afficher_type(EntityType type) {
    switch (type) {
        case NONE:
            printf("NONE");
            break;
        case MODEL:
            printf("MODEL");
            break;
        case COLLISION:
            printf("COLLISION");
            break;
        case STRUCT:
            printf("STRUCT");
            break;
    }
}

typedef struct ModelData {
    char name[16];
    char path[128];
}ModelData;

void afficher_ModelData(ModelData data) {
    printf("\t\tModelData:{name:%s, path:%s}\n", data.name, data.path);
}

typedef struct CollisionData {
    DUAL_ColliderType type;
    union{
        DUAL_AABB aabb;
        DUAL_Sphere sphere;
    }CollisionDataInfo;
}CollisionData;

void afficher_CollisionData(CollisionData data) {
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

typedef struct StructData {
    char targetname[32];
    char script_string[32];
    char script_noteworthy[32];
}StructData;

void afficher_StructData(StructData data) {
    printf("\t\tStructData:{targetname:%s, script_string:%s, script_noteworthy:%s}\n", data.targetname, data.script_string, data.script_noteworthy);
}

typedef struct SceneEntity {
    EntityType type;
    DUAL_Transform3D transform;
    char name[64];
    unsigned int id;

    unsigned int blockSize;
    char* block;
}SceneEntity;

void sauvegarder_entite(SceneEntity* entity, char* buffer, unsigned int* buffer_offset) {
    if (entity == NULL) {
        DUAL_Log(DUAL_LOG_ERROR, "Entity is null");
        return;
    }
    // Important de sauvegarder dans l'ordre pour ne pas se perdre plus tard
    memcpy(buffer + *buffer_offset, &entity->type, sizeof(EntityType)); *buffer_offset += sizeof(EntityType);
    memcpy(buffer + *buffer_offset, &entity->transform, sizeof(DUAL_Transform3D)); *buffer_offset += sizeof(DUAL_Transform3D);
    memcpy(buffer + *buffer_offset, entity->name, 64); *buffer_offset += 64;
    memcpy(buffer + *buffer_offset, &entity->id, sizeof(unsigned int)); *buffer_offset += sizeof(unsigned int);
    memcpy(buffer + *buffer_offset, &entity->blockSize, sizeof(unsigned int)); *buffer_offset += sizeof(unsigned int);
    memcpy(buffer + *buffer_offset, entity->block, entity->blockSize); *buffer_offset += entity->blockSize;
}

SceneEntity* charger_entite(char* buffer, unsigned int* buffer_offset) {
    SceneEntity* entity = malloc(sizeof(SceneEntity));
    memcpy(&entity->type, buffer + *buffer_offset, sizeof(EntityType)); *buffer_offset += sizeof(EntityType);
    memcpy(&entity->transform, buffer + *buffer_offset, sizeof(DUAL_Transform3D)); *buffer_offset += sizeof(DUAL_Transform3D);
    memcpy(entity->name, buffer + *buffer_offset, 64); *buffer_offset += 64;
    memcpy(&entity->id, buffer + *buffer_offset, sizeof(unsigned int)); *buffer_offset += sizeof(unsigned int);
    memcpy(&entity->blockSize, buffer + *buffer_offset, sizeof(unsigned int)); *buffer_offset += sizeof(unsigned int);
    entity->block = malloc(entity->blockSize);
    memcpy(entity->block, buffer + *buffer_offset, entity->blockSize); *buffer_offset += entity->blockSize;
    return entity;
}

typedef struct Scene {
    SceneEntity** entities;
    unsigned int numEntities;
}Scene;

void sauvegarder_scene(Scene* scene, char* buffer, unsigned int* buffer_offset) {
    print_buffer_hex(buffer, *buffer_offset);
    memcpy(buffer, &scene->numEntities, sizeof(unsigned int)); *buffer_offset += 4;
    print_buffer_hex(buffer, *buffer_offset);
    for (unsigned int i = 0; i < scene->numEntities; i++) {
        sauvegarder_entite(scene->entities[i], buffer, buffer_offset);
        print_buffer_hex(buffer, *buffer_offset);
    }
}

void charger_scene(Scene* scene, char* buffer, unsigned int* buffer_offset) {
    if (scene == NULL || buffer == NULL || buffer_offset == NULL) return;

    memcpy(&scene->numEntities, buffer + *buffer_offset, sizeof(unsigned int)); *buffer_offset += sizeof(unsigned int);
    scene->entities = malloc(sizeof(SceneEntity*) * SCENE_MAX_ENTITIES);

    for (unsigned int i = 0; i < scene->numEntities; i++) {
        scene->entities[i] = charger_entite(buffer, buffer_offset);
    }
}

void afficher_entite(SceneEntity* entity) {
    printf("\t-Entity{type:");
    afficher_type(entity->type);
    printf(", position:(%f, %f, %f), name:%s, id:%u, blocksize:%u}\n", entity->transform.position.x, entity->transform.position.y, entity->transform.position.z, entity->name, entity->id, entity->blockSize);
    switch (entity->type) {
        case MODEL:
            ModelData model; unsigned int offset = 0;
            GET_DATA_BUFFER(&model, ModelData, entity->block, offset);
            afficher_ModelData(model);
            break;
        case COLLISION:
            CollisionData collision; unsigned int offsetcol = 0;
            GET_DATA_BUFFER(&collision, CollisionData, entity->block, offsetcol);
            afficher_CollisionData(collision);
            break;
        case STRUCT:
            StructData structData; unsigned int offsetstruct = 0;
            GET_DATA_BUFFER(&structData, StructData, entity->block, offsetstruct);
            afficher_StructData(structData);
            break;
        case NONE:
            break;
    }
}

void afficher_scene(Scene* scene) {
    if (scene == NULL) return;
    printf("Scene(%u entities):\n",scene->numEntities);
    for (unsigned int i = 0; i < scene->numEntities; i++) {
        if (scene->entities[i] == NULL) continue;
        afficher_entite(scene->entities[i]);
    }
}
void addEntity(Scene* scene,SceneEntity* entity) {
    scene->entities[scene->numEntities] = entity;
    scene->numEntities += 1;
}

SceneEntity createEntity(EntityType type, DUAL_Transform3D transform3_d, const char* name, void* blockData) {
    SceneEntity entity;
    entity.type = type;
    entity.transform = transform3_d;
    strcpy(entity.name, name);
    entity.blockSize = 0;
    switch (type) {
        case NONE:
            entity.block = NULL;
            break;
        case MODEL:
            ModelData* model_data = (ModelData*)blockData;
            entity.block = malloc(sizeof(ModelData));
            WRITE_DATA_BUFFER(*model_data, ModelData, entity.block, entity.blockSize);
            break;
        case COLLISION:
            CollisionData* col_data = (CollisionData*)blockData;
            entity.block = malloc(sizeof(CollisionData));
            WRITE_DATA_BUFFER(*col_data, CollisionData, entity.block, entity.blockSize);
            break;
        case STRUCT:
            StructData* struct_data = (StructData*)blockData;
            entity.block = malloc(sizeof(StructData));
            WRITE_DATA_BUFFER(*struct_data, StructData, entity.block, entity.blockSize);
            break;
    }
    return entity;
}

size_t entity_getTotalSize(SceneEntity* entity) {
    size_t size = 0;
    size += sizeof(EntityType);
    size += sizeof(DUAL_Transform3D);
    size += 64; // le nom fait 64 caracteres et 1 caractere = 1 octet
    size += sizeof(unsigned int);
    size += sizeof(unsigned int);
    size += entity->blockSize;
    return size;
}

size_t scene_getTotalSize(Scene* scene) {
    size_t total = sizeof(unsigned int);
    for (unsigned int i = 0; i < scene->numEntities; i++) {
        total += entity_getTotalSize(scene->entities[i]);
    }
    return total;
}

int main() {
    /*
    printf("\n");

    // On creer notre Scene
    Scene scene; scene.entities = malloc(sizeof(SceneEntity) * SCENE_MAX_ENTITIES); scene.numEntities = 0;

    SceneEntity entity_1 = createEntity(NONE, (DUAL_Transform3D){}, "Entity_1", NULL);
    ModelData modelData; strcpy(modelData.name, "ModelData"); strcpy(modelData.path, "CHEMIN_VERS_MODEL");
    SceneEntity entity_2 = createEntity(MODEL, (DUAL_Transform3D){}, "Entity_2", &modelData);
    CollisionData collisionData; collisionData.type = DUAL_COLLIDER_AABB; collisionData.CollisionDataInfo.aabb = (DUAL_AABB){(-5.0,-5.0,-5.0),(5.0,5.0,5.0)};
    SceneEntity entity_3 = createEntity(COLLISION, (DUAL_Transform3D){}, "Entity_3", &collisionData);
    CollisionData collisionData2; collisionData2.type = DUAL_COLLIDER_SPHERE; collisionData2.CollisionDataInfo.sphere = (DUAL_Sphere){(DUAL_Vec3){2.0,4.0,6.0}, 5.0};
    SceneEntity entity_4 = createEntity(COLLISION, (DUAL_Transform3D){}, "Entity_4", &collisionData2);
    StructData structData; strcpy(structData.targetname,"StructData_1"); strcpy(structData.script_noteworthy, "Script_Noteworthy"); strcpy(structData.script_string, "Script_String");
    SceneEntity entity_5 = createEntity(STRUCT, (DUAL_Transform3D){}, "Entity_5", &structData);

    printf("Entities:\n");
    afficher_entite(&entity_1);
    afficher_entite(&entity_2);
    afficher_entite(&entity_3);
    afficher_entite(&entity_4);
    afficher_entite(&entity_5);

    printf("\n");

    // On ajoute toutes les entities a la scene
    addEntity(&scene, &entity_1);
    addEntity(&scene, &entity_2);
    addEntity(&scene, &entity_3);
    addEntity(&scene, &entity_4);
    addEntity(&scene, &entity_5);

    // On affiche notre scene
    afficher_scene(&scene);

    // On creer un buffer de la scene complete
    size_t total_size = scene_getTotalSize(&scene);
    printf("scene_size: %d\n",total_size);
    char buffer[total_size];

    // On initialise tout le buffer a 0
    memset(buffer, 0, total_size);
    unsigned int buffer_offset = 0;

    // On sauvegarde la scene dans le buffer
    sauvegarder_scene(&scene, buffer, &buffer_offset);

    // On sauvegarde dans un fichier
    DUAL_FS_SetSaveDirectory("/home/killian/Projects/C/Dual/Jeux/Testing/");
    DUAL_Result result;
    DUAL_SaveFile* save = NULL;
    result = DUAL_SaveFile_Open(0,DUAL_SAVE_MODE_ECRITURE, &save);
    result = DUAL_SaveFile_Write(save, buffer, total_size);
    result = DUAL_SaveFile_Close(save);


    */
    DUAL_FS_SetSaveDirectory("/home/killian/Projects/C/Dual/Jeux/Testing/");
    DUAL_Result result;
    DUAL_SaveFile* save = NULL;

    // On charge la scene depuis le fichier creer
    result = DUAL_SaveFile_Open(0, DUAL_SAVE_MODE_LECTURE, &save);
    DEBUG_DUAL_RESULT(result);
    long unsigned int filesize = 0;
    result = DUAL_SaveFile_GetSize(save, &filesize);
    DEBUG_DUAL_RESULT(result);
    char new_buffer[filesize];
    memset(new_buffer, 0, filesize);
    result = DUAL_SaveFile_Read(save, new_buffer, filesize, NULL);
    DEBUG_DUAL_RESULT(result);

    // On va creer une deuxieme scene qui va etre chargee depuis le buffer
    Scene scene2;
    unsigned int buffer_offset_scene = 0;
    charger_scene(&scene2, new_buffer, &buffer_offset_scene);
    afficher_scene(&scene2);


    return EXIT_SUCCESS;
}