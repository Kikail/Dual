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
    afficher_scene(output);
}

void game_charger_ressources(struct DUAL_ResourceManager* resource_manager, DUAL_Editor_Scene* scene) {
    for (int i = 0; i < scene->numEntities; i++) {
        DUAL_Editor_SceneEntity* entity = getEntity(scene, i);
        if (entity->type == MODEL) {
            DUAL_Editor_ModelData model; unsigned int offset = 0;
            GET_DATA_BUFFER(&model, DUAL_Editor_ModelData, entity->block, offset);
            DUAL_Log(DUAL_LOG_INFO, "%d - loading: %s ...", i, model.path);

            DUAL_Model* model3D; unsigned int nb_mats = 0; DUAL_Material** mats;

            DUAL_Model_Load(resource_manager, model.path, &model3D, &nb_mats, &mats);
        }
    }
}

int main() {
    // On charge notre scene
    char* filepath = "/home/killian/Documents/myScene.dual";
    DUAL_Editor_Scene scene;
    game_charger_scene(filepath, &scene);

    // On creer notre application
    DUAL_App* app = NULL;
    DUAL_AppConfig config = {
        .largeur_ecran = 1200,
        .hauteur_ecran = 1200,
        .plein_ecran = false,
        .fps_cible = 60,
        .titre_fenetre = "DUAL_FPS_Game",
        .vsync_actif = false,
        .screenLayout = DUAL_LAYOUT_NO_SPLIT
    };
    DUAL_Init(&config, &app);

    // On creer notre Renderer
    DUAL_Renderer3D* renderer3D = NULL;
    DUAL_Renderer3D_Create(app, &renderer3D);

    // On change la couleur ambiante
    DUAL_Renderer3D_SetAmbientLight(renderer3D, (DUAL_Vec3){0.3,0.3,0.45});
    DUAL_Light light = {
        .type = DUAL_LIGHT_POINT,
        .position = {3.0,0.0,-2.5},
        .couleur = {1.0,0.3,0.3},
        .direction = {0.0,0.0,0.0},
        .intensite = 0.5,
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

    // On creer le resource manager
    DUAL_ResourceManager* resourceManager = NULL;
    DUAL_ResourceManager_Create(app, &resourceManager);

    game_charger_ressources(resourceManager, &scene);

    // On creer notre handler d'Inputs
    DUAL_InputManager* inputManager = NULL;
    DUAL_InputManager_Create(app, &inputManager);

    DUAL_SetScreenClearColor(app, DUAL_SCREEN_MAIN, DUAL_COLOR_BLUE);

    while (DUAL_ShouldRun(app)) {
        DUAL_BeginFrame(app);


        // On actualise les inputs
        DUAL_InputManager_Update(inputManager);
        if (DUAL_IsButtonDown(inputManager, DUAL_BUTTON_UP)) {
            DUAL_Camera3D* cam = DUAL_Renderer3D_GetCamera(renderer3D);
            DUAL_Camera3D_ProcessKeyboard(cam, FORWARD, DUAL_GetDeltaTime(app));
        }
        if (DUAL_IsButtonDown(inputManager, DUAL_BUTTON_DOWN)) {
            DUAL_Camera3D* cam = DUAL_Renderer3D_GetCamera(renderer3D);
            DUAL_Camera3D_ProcessKeyboard(cam, BACKWARD, DUAL_GetDeltaTime(app));
        }
        if (DUAL_IsButtonDown(inputManager, DUAL_BUTTON_LEFT)) {
            DUAL_Camera3D* cam = DUAL_Renderer3D_GetCamera(renderer3D);
            DUAL_Camera3D_ProcessKeyboard(cam, LEFT, DUAL_GetDeltaTime(app));
        }
        if (DUAL_IsButtonDown(inputManager, DUAL_BUTTON_RIGHT)) {
            DUAL_Camera3D* cam = DUAL_Renderer3D_GetCamera(renderer3D);
            DUAL_Camera3D_ProcessKeyboard(cam, RIGHT, DUAL_GetDeltaTime(app));
        }


        // On actualise les donnees du monde ici



        // On dessine nos images
        DUAL_Renderer3D_Begin(renderer3D);

        // Code pour dessiner dans notre renderer
        for (int i = 0; i < scene.numEntities; i++) {
            DUAL_Editor_SceneEntity* entity = getEntity(&scene, i);
            if (entity->type == MODEL) {
                DUAL_Editor_ModelData modelData; unsigned int offset = 0;
                GET_DATA_BUFFER(&modelData, DUAL_Editor_ModelData, entity->block, offset);

                DUAL_Model* model = DUAL_ResourceManager_GetModel(resourceManager, modelData.name);
                if (!model) {
                    DUAL_Log(DUAL_LOG_ERROR, "trying to use a NULL model");
                    continue;
                }
                DUAL_DrawModel(renderer3D, model, model->materials, entity->transform);
            }
        }

        DUAL_Renderer3D_End(renderer3D);


        DUAL_EndFrame(app);
    }

    return EXIT_SUCCESS;
}
