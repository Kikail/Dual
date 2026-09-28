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

    DUAL_PhysicsWorld* physicsWorld = DUAL_PhysicsWorld_Create((DUAL_Vec3){0.0,-9.81,0.0});
    DUAL_RigidBody body;
    body.masse = 1.0;
    body.est_statique = true;
    body.position = (DUAL_Vec3){-10.0, 0.0, 60.0};
    body.collider.type = DUAL_COLLIDER_AABB;
    body.vitesse = (DUAL_Vec3){15.0, 10.0, 0.0};
    body.collider.shape.aabb = (DUAL_AABB){(DUAL_Vec3){-5.0,-5.0,-5.0},(DUAL_Vec3){5.0,5.0,5.0}};

    DUAL_RigidBody body2;
    body2.masse = 1.0;
    body2.est_statique = false;
    body2.position = (DUAL_Vec3){10.0, 0.0, 60.0};
    body2.collider.type = DUAL_COLLIDER_SPHERE;
    body2.vitesse = (DUAL_Vec3){-5.0, 25.0, 0.0};
    body2.collider.shape.sphere = (DUAL_Sphere){(DUAL_Vec3){0.0,0.0,0.0}, 3.0};

    DUAL_PhysicsWorld_AddBody(physicsWorld, body);
    DUAL_PhysicsWorld_AddBody(physicsWorld, body2);

    for (int k = 0; k < physicsWorld->count_bodies; k++) {
        DUAL_Debug_PrintBody(&physicsWorld->bodies[k]);
    }

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
        DUAL_PhysicsWorld_Step(physicsWorld, DUAL_GetDeltaTime(app));

        // On selectionne l'ecran du haut
        DUAL_SetActiveScreen(app, DUAL_SCREEN_RIGHT);

        // On dessine nos images
        DUAL_Renderer3D_Begin(renderer3D);

        for (int k = 0; k < physicsWorld->count_bodies; k++) {
            switch (physicsWorld->bodies[k].collider.type) {
                case DUAL_COLLIDER_AABB:
                    DUAL_Debug_DrawAABB(renderer3D, DUAL_GetGlobalAABB(&physicsWorld->bodies[k]), DUAL_VEC3_COLOR_GREEN);
                    break;
                case DUAL_COLLIDER_SPHERE:
                    DUAL_Debug_DrawSphere(renderer3D, DUAL_GetGlobalSphere(&physicsWorld->bodies[k]), DUAL_VEC3_COLOR_GREEN);
                    break;
            }
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
