//
// Created by killian on 9/29/26.
//

#include "../../SDK/include/DUAL_Editor/dual_scene_loader.h"
#include "../../SDK/include/DUAL_FS/dual_fs.h"
#include "../../SDK/include/dual_utils.h"

int main() {
    printf("\n");

    // On creer notre Scene
    DUAL_Editor_Scene scene;
    scene.entities = NULL;
    scene.numEntities = 0;

    DUAL_Editor_SceneEntity* entity_1 = createEntity(NONE, (DUAL_Transform3D){}, "Entity_1", NULL);
    DUAL_Editor_ModelData modelData; strcpy(modelData.name, "ModelData"); strcpy(modelData.path, "CHEMIN_VERS_MODEL");
    DUAL_Editor_SceneEntity* entity_2 = createEntity(MODEL, (DUAL_Transform3D){}, "Entity_2", &modelData);
    DUAL_Editor_CollisionData collisionData; collisionData.type = DUAL_COLLIDER_AABB; collisionData.CollisionDataInfo.aabb = (DUAL_AABB){(-5.0,-5.0,-5.0),(5.0,5.0,5.0)};
    DUAL_Editor_SceneEntity* entity_3 = createEntity(COLLISION, (DUAL_Transform3D){}, "Entity_3", &collisionData);
    DUAL_Editor_CollisionData collisionData2; collisionData2.type = DUAL_COLLIDER_SPHERE; collisionData2.CollisionDataInfo.sphere = (DUAL_Sphere){(DUAL_Vec3){2.0,4.0,6.0}, 5.0};
    DUAL_Editor_SceneEntity* entity_4 = createEntity(COLLISION, (DUAL_Transform3D){}, "Entity_4", &collisionData2);
    DUAL_Editor_StructData structData; strcpy(structData.targetname,"StructData_1"); strcpy(structData.script_noteworthy, "Script_Noteworthy"); strcpy(structData.script_string, "Script_String");
    DUAL_Editor_SceneEntity* entity_5 = createEntity(STRUCT, (DUAL_Transform3D){}, "Entity_5", &structData);

    printf("Entities:\n");
    afficher_entite(entity_1);
    afficher_entite(entity_2);
    afficher_entite(entity_3);
    afficher_entite(entity_4);
    afficher_entite(entity_5);

    printf("\n");

    // On ajoute toutes les entities a la scene
    addEntity(&scene, entity_1);
    addEntity(&scene, entity_2);
    addEntity(&scene, entity_3);
    addEntity(&scene, entity_4);
    addEntity(&scene, entity_5);

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
    DUAL_Editor_Scene scene2;
    unsigned int buffer_offset_scene = 0;
    charger_scene(&scene2, new_buffer, &buffer_offset_scene);

    DUAL_Editor_SceneEntity* target_entity = (DUAL_Editor_SceneEntity*) g_list_nth_data(scene2.entities, 0);
    removeEntity(&scene2, target_entity);

    afficher_scene(&scene2);

    return EXIT_SUCCESS;
}
