//
// Created by killian on 7/9/26.
//
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "dual_core.h"
#include "dual_graphics_3d.h"
#include "dual_graphics_2d.h"
#include "dual_input.h"
#include "dual_resources.h"
#include "dual_utils.h"


int main() {
    DUAL_Log(DUAL_LOG_INFO, "Test Graphics 2D started !");

    // On initialise l'application
    DUAL_App* app = NULL;
    DUAL_AppConfig config = {
        .largeur_ecran = 800,
        .hauteur_ecran = 480,
        .plein_ecran = false,
        .fps_cible = 60,
        .titre_fenetre = "DUAL Core Testing",
        .vsync_actif = false
    };
    DUAL_Result result = DUAL_Init(&config, &app);
    DEBUG_DUAL_RESULT(result);

    // On creer le resource manager
    DUAL_ResourceManager* resourceManager = NULL;
    DUAL_ResourceManager_Create(app, &resourceManager);

    // On charge des ressources
    DUAL_Model* ambulanceModel = NULL;
    DUAL_Texture* diffuseTexture = NULL;
    result = DUAL_Model_LoadFromOBJ(resourceManager, "/home/killian/CLionProjects/Dual/Jeux/resources/3D/cars/OBJ format/ambulance.obj", &ambulanceModel);
    DEBUG_DUAL_RESULT(result);
    result = DUAL_Texture_LoadFromFile(resourceManager, "/home/killian/CLionProjects/Dual/Jeux/resources/3D/cars/OBJ format/Textures/colormap.png", DUAL_FILTER_LINEAR, &diffuseTexture);
    DEBUG_DUAL_RESULT(result);
    DUAL_Material* ambulanceMaterial = NULL;
    result = DUAL_Material_Create(resourceManager, diffuseTexture, &ambulanceMaterial);
    DEBUG_DUAL_RESULT(result);

    // Position 3D de notre ambulance
    DUAL_Transform3D ambulanceTransform3D = {
        .position = {0.0,-2.0,-5.0},
        .echelle = {1.0,1.0,1.0},
        .rotation_euler_radians = {0.0,0.0,0.0},
    };

    // On affiche les stats de notre resource manager
    DUAL_ResourceManager_Log(resourceManager);

    // On creer notre 3d renderer
    DUAL_Renderer3D* renderer3D = NULL;
    result = DUAL_Renderer3D_Create(app, &renderer3D);
    DEBUG_DUAL_RESULT(result);

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

    // On creer les input
    DUAL_InputManager* inputManager = NULL;
    DUAL_InputManager_Create(app, &inputManager);

    int projectionMode = 0;
    int rendererMode = 0;

    // Boucle du jeu principal
    while (DUAL_ShouldRun(app)) {
        DUAL_BeginFrame(app);

        // On actualise les inputs
        DUAL_InputManager_Update(inputManager);

        // On change la projection si on appuie sur la touche du haut
        if (DUAL_IsButtonPressed(inputManager, DUAL_BUTTON_UP)) {
            if (projectionMode == 0) {
                projectionMode = 1;
                DUAL_Renderer3D_SetProjectionMode(renderer3D, projectionMode);
            }
            else {
                projectionMode = 0;
                DUAL_Renderer3D_SetProjectionMode(renderer3D, projectionMode);
            }
        }
        // On change le mode de rendu
        if (DUAL_IsButtonPressed(inputManager, DUAL_BUTTON_RIGHT)) {
            rendererMode += 1;
            if (rendererMode > 2)
                rendererMode = 0;
            DUAL_Renderer3D_SetRenderMode(renderer3D, rendererMode);
        }

        // Actualisation de nos structures
        ambulanceTransform3D.rotation_euler_radians = (DUAL_Vec3){0.0, DUAL_GetTime(app),  0.0};

        // On selectionne l'ecran du bas
        DUAL_SetActiveScreen(app, DUAL_SCREEN_BOTTOM);

        // On dessine nos images


        // On selectionne l'ecran du haut
        DUAL_SetActiveScreen(app, DUAL_SCREEN_TOP);

        // On dessine nos images
        DUAL_Renderer3D_Begin(renderer3D);
        DUAL_DrawModel(renderer3D, ambulanceModel, ambulanceMaterial, ambulanceTransform3D);
        DUAL_Renderer3D_End(renderer3D);

        DUAL_EndFrame(app);
    }

    // On ferme proprement l'application
    DUAL_ResourceManager_Destroy(resourceManager);
    DUAL_Renderer3D_Destroy(renderer3D);
    DUAL_Shutdown(app);

    return EXIT_SUCCESS;
}
