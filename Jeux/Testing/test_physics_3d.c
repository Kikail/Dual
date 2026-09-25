//
// Created by killian on 7/9/26.
//
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../../SDK/include/DUAL_Core/dual_core.h"
#include "../../SDK/include/DUAL_Graphics/dual_graphics_3d.h"
#include "../../SDK/include/DUAL_Graphics/dual_graphics_2d.h"
#include "../../SDK/include/DUAL_Input/dual_input.h"
#include "../../SDK/include/DUAL_Resources/dual_resources.h"
#include "dual_utils.h"
#include "../../SDK/include/DUAL_Graphics/shader.h"
#include "DUAL_Graphics/camera3d.h"
#include "DUAL_Graphics/particle_system.h"
#include "DUAL_Physics/dual_physics.h"


int main() {
    DUAL_Log(DUAL_LOG_INFO, "Test Graphics 2D started !");

    srand(time(NULL));

    // On initialise l'application
    DUAL_App* app = NULL;
    DUAL_AppConfig config = {
        .largeur_ecran = 1200,
        .hauteur_ecran = 1200,
        .plein_ecran = false,
        .fps_cible = 60,
        .titre_fenetre = "DUAL Core Testing",
        .vsync_actif = false,
        .screenLayout = DUAL_LAYOUT_NO_SPLIT
    };
    DUAL_Result result = DUAL_Init(&config, &app);
    DEBUG_DUAL_RESULT(result);

    // On creer le resource manager
    DUAL_ResourceManager* resourceManager = NULL;
    DUAL_ResourceManager_Create(app, &resourceManager);

    // On creer notre 3d renderer
    DUAL_Renderer3D* renderer3D = NULL;
    result = DUAL_Renderer3D_Create(app, &renderer3D);
    DEBUG_DUAL_RESULT(result);
    DUAL_Renderer3D_SetCullMode(renderer3D, DUAL_CULL_BACK);

    // On creer les input
    DUAL_InputManager* inputManager = NULL;
    DUAL_InputManager_Create(app, &inputManager);

    int projectionMode = 0;
    int rendererMode = 0;

    DUAL_AABB box;
    box.min = (DUAL_Vec3){-20.0, 0.0, 40.0};
    box.max = (DUAL_Vec3){-10.0, 10.0, 50.0};

    DUAL_Sphere sphere;
    sphere.centre = (DUAL_Vec3){15.0, -0.1, 45.0};
    sphere.rayon = 6.0;

    double oldx = 0, oldy = 0;
    bool firstMouse = false;

    // Boucle du jeu principal
    while (DUAL_ShouldRun(app)) {
        DUAL_BeginFrame(app);

        // On actualise les inputs
        DUAL_InputManager_Update(inputManager);

        if (DUAL_IsTouching(inputManager)) {
            double x,y;
            glfwGetCursorPos(DUAL_GetWindow(app), &x, &y);

            if (!firstMouse) {
                oldx = x;
                oldy = y;
                firstMouse = true;
            }

            DUAL_Camera3D_ProcessMouseMovement(DUAL_Renderer3D_GetCamera(renderer3D), x-oldx, y-oldy);
            oldx = x;
            oldy = y;
        }
        else {
            firstMouse = false;
        }

        // On change la projection si on appuie sur la touche du haut
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
        // On change le mode de rendu
        if (DUAL_IsButtonPressed(inputManager, DUAL_BUTTON_A)) {
            rendererMode += 1;
            if (rendererMode > 2)
                rendererMode = 0;
            DUAL_Renderer3D_SetRenderMode(renderer3D, rendererMode);
        }

        box.max.x += sin(DUAL_GetTime(app)) * DUAL_GetDeltaTime(app) * 12;
        box.min.x += sin(DUAL_GetTime(app)) * DUAL_GetDeltaTime(app) * 12;
        sphere.centre.x += sin(DUAL_GetTime(app)) * DUAL_GetDeltaTime(app) * -12;


        // On selectionne l'ecran du haut
        DUAL_SetActiveScreen(app, DUAL_SCREEN_RIGHT);

        // On dessine nos images
        DUAL_Renderer3D_Begin(renderer3D);

        DUAL_CollisionInfo collisionInfo = DUAL_Collide_SphereVSAABB(&sphere, &box);

        if (collisionInfo.collision) {
            sphere.centre = DUAL_Vec3_Add(sphere.centre, DUAL_Vec3_Scale(collisionInfo.normal, collisionInfo.penetration));
            DUAL_Debug_DrawAABB(renderer3D, box, DUAL_VEC3_COLOR_GREEN);
            DUAL_Debug_DrawSphere(renderer3D, sphere, DUAL_VEC3_COLOR_GREEN);
        }
        else {
            DUAL_Debug_DrawAABB(renderer3D, box, DUAL_VEC3_COLOR_BLUE);
            DUAL_Debug_DrawSphere(renderer3D, sphere, DUAL_VEC3_COLOR_RED);
        }

        DUAL_Renderer3D_End(renderer3D);

        DUAL_EndFrame(app);
    }

    // On ferme proprement l'application
    //ParticleSystem_Clean(&particle_system);
    DUAL_ResourceManager_Destroy(resourceManager);
    DUAL_Renderer3D_Destroy(renderer3D);
    DUAL_Shutdown(app);

    return EXIT_SUCCESS;
}
