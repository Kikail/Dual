//
// Created by killian on 9/29/26.
//

#include "../../SDK/include/DUAL_Editor/dual_scene_loader.h"
#include "../../SDK/include/DUAL_FS/dual_fs.h"
#include "../../SDK/include/DUAL_Resources/dual_resources.h"
#include "../../SDK/include/dual_utils.h"
#include "DUAL_Input/dual_input.h"

void game_charger_scene(char* filepath, DUAL_Editor_Scene* output) {
    long unsigned int filesize = 0;
    DUAL_GetFileSize(filepath, &filesize);
    char new_buffer[filesize]; memset(new_buffer, 0, filesize);
    DUAL_LoadFile(filepath, new_buffer, filesize);
    unsigned int buffer_offset_scene = 0;
    charger_scene(output, new_buffer, &buffer_offset_scene);
    //afficher_scene(output);
}

DUAL_RigidBody create_collision(DUAL_Vec3 position, DUAL_AABB aabb) {
    DUAL_RigidBody body;
    body.masse = 1.0f;
    body.masse_inverse = 1.0f;
    body.est_statique = true;
    body.actif = true;
    body.amortissement = 0.98f;
    body.position = position;
    body.vitesse = (DUAL_Vec3){0.0f, 0.0f, 0.0f};
    body.forces = (DUAL_Vec3){0.0f, 0.0f, 0.0f};
    body.collider.type = DUAL_COLLIDER_AABB;
    body.collider.shape.aabb = aabb;
    return body;
}
DUAL_RigidBody create_player_body(DUAL_Vec3 position) {
    DUAL_RigidBody body;
    body.masse = 1.0f;
    body.masse_inverse = 1.0f;
    body.est_statique = false;
    body.actif = true;
    body.amortissement = 0.98f;
    body.position = position;
    body.vitesse = (DUAL_Vec3){0.0f, 0.0f, 0.0f};
    body.forces = (DUAL_Vec3){0.0f, 0.0f, 0.0f};
    body.collider.type = DUAL_COLLIDER_AABB;
    body.collider.shape.aabb = (DUAL_AABB){(DUAL_Vec3){-0.3f, 0.0f, -0.3f}, (DUAL_Vec3){0.3f, 1.7f, 0.3f}};
    return body;
}

void game_charger_collisions(DUAL_PhysicsWorld* physicsWorld, DUAL_Editor_Scene* scene) {
    for (int i = 0; i < scene->numEntities; i++) {
        DUAL_Editor_SceneEntity* entity = getEntity(scene, i);
        if (entity->type == COLLISION) {
            DUAL_Editor_CollisionData collision; unsigned int offsetcol = 0;
            GET_DATA_BUFFER(&collision, DUAL_Editor_CollisionData, entity->block, offsetcol);
            //afficher_CollisionData(collision);
            DUAL_PhysicsWorld_AddBody(physicsWorld, create_collision(entity->transform.position, collision.CollisionDataInfo.aabb));
        }
    }
}

DUAL_Editor_SceneEntity structs_entities[16]; unsigned int current_structs = 0;
DUAL_Editor_SceneEntity* game_get_struct_entity(char* targetname, DUAL_Editor_StructData* out_struct_data) {
    for (int i = 0; i < current_structs; i++) {
        if (structs_entities[i].type == STRUCT) {
            DUAL_Editor_StructData structData; unsigned int offsetstruct = 0;
            GET_DATA_BUFFER(&structData, DUAL_Editor_StructData, structs_entities[i].block, offsetstruct);
            if (strcmp(structData.targetname, targetname) == 0) {
                *out_struct_data = structData;
                return &structs_entities[i];
            }
        }
    }
    return NULL;
}

void game_charger_ressources(DUAL_ResourceManager* resource_manager, DUAL_Editor_Scene* scene) {
    for (int i = 0; i < scene->numEntities; i++) {
        DUAL_Editor_SceneEntity* entity = getEntity(scene, i);
        if (entity->type == MODEL) {
            DUAL_Editor_ModelData model; unsigned int offset = 0;
            GET_DATA_BUFFER(&model, DUAL_Editor_ModelData, entity->block, offset);
            DUAL_Log(DUAL_LOG_INFO, "%d - loading: %s ...", i, model.path);

            DUAL_Model* model3D; unsigned int nb_mats = 0; DUAL_Material** mats;

            DUAL_Model_Load(resource_manager, model.path, &model3D, &nb_mats, &mats);
        }
        if (entity->type == STRUCT) {
            if (current_structs < 16) {
                structs_entities[current_structs] = *entity;
                current_structs++;
            } else {
                DUAL_Log(DUAL_LOG_ERROR, "Trop de structures dans la scene ! Limite atteinte (16 max).");
            }
        }
    }
}

#define GAME_VITESSE_JOUEUR 4.0
#define GAME_JUMP_HEIGHT 4.2
#define SHOW_DEBUG_AABB false

void game_deplacer_joueur(DUAL_InputManager* inputManager, DUAL_RigidBody* playerBody, float dt, DUAL_Camera3D* cam) {
    float vitesse_y_actuelle = playerBody->vitesse.y;

    DUAL_Vec3 nouvelle_vitesse = { 0.0f, vitesse_y_actuelle, 0.0f };
    DUAL_Vec3 front_flat = { cam->front.x, 0.0f, cam->front.z };
    front_flat = DUAL_Vec3_Normalize(front_flat);
    DUAL_Vec3 right_flat = { cam->right.x, 0.0f, cam->right.z };
    right_flat = DUAL_Vec3_Normalize(right_flat);

    if (DUAL_IsButtonDown(inputManager, DUAL_BUTTON_KEYBOARD_W)) {
        nouvelle_vitesse = DUAL_Vec3_Add(nouvelle_vitesse, DUAL_Vec3_Scale(front_flat, GAME_VITESSE_JOUEUR));
    }
    if (DUAL_IsButtonDown(inputManager, DUAL_BUTTON_KEYBOARD_S)) {
        nouvelle_vitesse = DUAL_Vec3_Sub(nouvelle_vitesse, DUAL_Vec3_Scale(front_flat, GAME_VITESSE_JOUEUR));
    }
    if (DUAL_IsButtonDown(inputManager, DUAL_BUTTON_KEYBOARD_A)) {
        nouvelle_vitesse = DUAL_Vec3_Sub(nouvelle_vitesse, DUAL_Vec3_Scale(right_flat, GAME_VITESSE_JOUEUR));
    }
    if (DUAL_IsButtonDown(inputManager, DUAL_BUTTON_KEYBOARD_D)) {
        nouvelle_vitesse = DUAL_Vec3_Add(nouvelle_vitesse, DUAL_Vec3_Scale(right_flat, GAME_VITESSE_JOUEUR));
    }
    if (DUAL_IsButtonPressed(inputManager, DUAL_BUTTON_KEYBOARD_SPACE)) {
        nouvelle_vitesse.y = GAME_JUMP_HEIGHT;
    }

    cam->position = DUAL_Vec3_Add(playerBody->position, (DUAL_Vec3){0.0, 1.5, 0.0});

    playerBody->vitesse = nouvelle_vitesse;
}

int main() {
    char* filepath = "/home/killian/Scenes/scene1.dual";
    DUAL_Editor_Scene scene;
    game_charger_scene(filepath, &scene);

    DUAL_App* app = NULL;
    DUAL_AppConfig config = {
        .largeur_ecran = 1920,
        .hauteur_ecran = 1080,
        .plein_ecran = false,
        .fps_cible = 60,
        .titre_fenetre = "DUAL_FPS_Game",
        .vsync_actif = false,
        .screenLayout = DUAL_LAYOUT_NO_SPLIT
    };
    DUAL_Init(&config, &app);
    glfwSetInputMode(DUAL_GetWindow(app), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    DUAL_Renderer3D* renderer3D = NULL;
    DUAL_Renderer3D_Create(app, &renderer3D);

    DUAL_Renderer3D_SetAmbientLight(renderer3D, (DUAL_Vec3){0.3,0.3,0.45});
    DUAL_Light light = {
        .type = DUAL_LIGHT_POINT,
        .position = {3.0,0.0,-2.5},
        .couleur = {1.0,0.3,0.3},
        .direction = {0.0,0.0,0.0},
        .intensite = 1.0,
        10
    };
    DUAL_Light sun = {
        .type = DUAL_LIGHT_DIRECTIONAL,
        .position = {3.0,0.0,-2.5},
        .couleur = {0.9,0.9,0.9},
        .direction = {0.0,-1.0,-0.3},
        .intensite = 0.6
    };
    DUAL_Renderer3D_SetLight(renderer3D, 0, light);
    DUAL_Renderer3D_SetLight(renderer3D, 1, sun);
    DUAL_Renderer3D_SetCullMode(renderer3D, DUAL_CULL_BACK);

    DUAL_ResourceManager* resourceManager = NULL;
    DUAL_ResourceManager_Create(app, &resourceManager);

    game_charger_ressources(resourceManager, &scene);

    DUAL_Animated_Model* animatedModel = NULL;
    DUAL_AnimatedModel_Create(resourceManager, "/home/killian/Scenes/pistol.fbx", &animatedModel);
    DUAL_AnimatedModel_PlayAnimation(animatedModel, 1);

    DUAL_PhysicsWorld* physicsWorld = DUAL_PhysicsWorld_Create((DUAL_Vec3){0.0, -9.81, 0.0});

    DUAL_Vec3 spawn_pos = {0.0,0.0,0.0};
    DUAL_Editor_StructData player_spawn_struct;
    DUAL_Editor_SceneEntity* player_spawn_entity = game_get_struct_entity("player_spawn",&player_spawn_struct);
    if (player_spawn_entity != NULL) {
        spawn_pos = player_spawn_entity->transform.position;
    }
    unsigned int player_body_index = DUAL_PhysicsWorld_AddBody(physicsWorld, create_player_body(spawn_pos));
    game_charger_collisions(physicsWorld, &scene);

    DUAL_InputManager* inputManager = NULL;
    DUAL_InputManager_Create(app, &inputManager);
    double oldx = 0, oldy = 0;
    bool firstMouse = false;


    DUAL_SetScreenClearColor(app, DUAL_SCREEN_MAIN, DUAL_COLOR_BLACK);

    while (DUAL_ShouldRun(app)) {
        DUAL_BeginFrame(app);

        DUAL_InputManager_Update(inputManager);

        double x, y;
        glfwGetCursorPos(DUAL_GetWindow(app), &x, &y);
        if (!firstMouse) {
            oldx = x;
            oldy = y;
            firstMouse = true;
        }
        double xoffset = x - oldx;
        double yoffset = y - oldy;
        oldx = x;
        oldy = y;
        DUAL_Camera3D* cam = DUAL_Renderer3D_GetCamera(renderer3D);
        DUAL_Camera3D_ProcessMouseMovement(cam, xoffset, yoffset);

        if (DUAL_IsButtonPressed(inputManager, DUAL_BUTTON_KEYBOARD_ESCAPE)) {
            DUAL_RequestExit(app);
        }

        DUAL_PhysicsWorld_Step(physicsWorld, DUAL_GetDeltaTime(app));

        game_deplacer_joueur(inputManager, &physicsWorld->bodies[player_body_index], DUAL_GetDeltaTime(app), DUAL_Renderer3D_GetCamera(renderer3D));

        DUAL_Renderer3D_Begin(renderer3D);

        for (int i = 0; i < scene.numEntities; i++) {
            DUAL_Editor_SceneEntity* entity = getEntity(&scene, i);
            if (entity->type == MODEL) {
                DUAL_Editor_ModelData modelData; unsigned int offset = 0;
                GET_DATA_BUFFER(&modelData, DUAL_Editor_ModelData, entity->block, offset);

                char model_resource_name[64];
                DUAL_GetFilenameWithoutExtension(modelData.path, model_resource_name, sizeof(model_resource_name));

                DUAL_Model* model = DUAL_ResourceManager_GetModel(resourceManager, model_resource_name);
                if (!model) {
                    DUAL_Log(DUAL_LOG_ERROR, "trying to use a NULL model");
                    continue;
                }
                DUAL_DrawModel(renderer3D, model, model->materials, entity->transform);
            }
        }

        if (SHOW_DEBUG_AABB) {
            DUAL_Debug_DrawAABB(renderer3D, DUAL_GetGlobalAABB(&physicsWorld->bodies[player_body_index]), DUAL_VEC3_COLOR_RED);
            for (int i = 0; i < physicsWorld->count_bodies; i++) {
                if (!physicsWorld->bodies[i].est_statique) continue;
                DUAL_Debug_DrawAABB(renderer3D, DUAL_GetGlobalAABB(&physicsWorld->bodies[i]), DUAL_VEC3_COLOR_MAGENTA);
            }
        }




        DUAL_Vec3 wpn_pos = cam->position;
        wpn_pos = DUAL_Vec3_Add(wpn_pos, DUAL_Vec3_Scale(cam->right, 0.3f));
        wpn_pos = DUAL_Vec3_Add(wpn_pos, DUAL_Vec3_Scale(cam->up,   -0.2f));
        wpn_pos = DUAL_Vec3_Add(wpn_pos, DUAL_Vec3_Scale(cam->front,  0.5f));

        DUAL_Transform3D wpn_transform;
        wpn_transform.position = wpn_pos;
        wpn_transform.rotation_euler_radians = (DUAL_Vec3){
            atan2f(cam->up.z, cam->right.z),
            -asinf(DUAL_Clamp(cam->front.z, -1.0f, 1.0f)),
            atan2f(cam->front.y, cam->front.x)
        };
        wpn_transform.echelle = (DUAL_Vec3){ 0.01f, 0.01f, 0.01f };

        DUAL_Renderer3D_UseShader(renderer3D, SHADER3D_SKELETAL_LIT);

        if (DUAL_IsButtonPressed(inputManager, DUAL_BUTTON_MOUSE_LEFT )) {
            DUAL_AnimatedModel_PlayAnimation(animatedModel, 0);
            animatedModel->animator.m_Loop = false;
        }
        DUAL_AnimatedModel_Update(animatedModel, DUAL_GetDeltaTime(app));
        if (animatedModel->animator.m_Finished) {
            DUAL_AnimatedModel_PlayAnimation(animatedModel, 1);
            animatedModel->animator.m_Loop = true;
        }

        DUAL_DrawAnimatedModel(renderer3D, animatedModel->model, animatedModel->model->materials, wpn_transform, &animatedModel->animator);


        DUAL_Renderer3D_End(renderer3D);

        DUAL_EndFrame(app);
    }

    return EXIT_SUCCESS;
}