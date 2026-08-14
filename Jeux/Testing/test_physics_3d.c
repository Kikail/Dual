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

#include <box3d/box3d.h>


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

    // On creer les input
    DUAL_InputManager* inputManager = NULL;
    DUAL_InputManager_Create(app, &inputManager);

    // On creer le renderer 3d
    DUAL_Renderer3D* renderer3D = NULL;
    result = DUAL_Renderer3D_Create(app, &renderer3D);
    DEBUG_DUAL_RESULT(result);

    // Mini test de box3d
    b3WorldDef worldDef = b3DefaultWorldDef();
    worldDef.gravity = (b3Vec3){0.0f, -9.81f, 0.0f};
    b3WorldId worldId = b3CreateWorld(&worldDef);
    if (b3World_IsValid(worldId)) {
        DUAL_Log(DUAL_LOG_INFO,"Box3D initialise avec succes dans CLion !");
    }
    b3DestroyWorld(worldId);

    // Boucle du jeu principal
    while (DUAL_ShouldRun(app)) {
        DUAL_BeginFrame(app);

        // On actualise les inputs
        DUAL_InputManager_Update(inputManager);

        DUAL_EndFrame(app);
    }

    // On ferme proprement l'application
    DUAL_ResourceManager_Destroy(resourceManager);
    DUAL_Renderer3D_Destroy(renderer3D);
    DUAL_Shutdown(app);

    return EXIT_SUCCESS;
}
