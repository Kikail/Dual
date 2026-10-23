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

#define MODEL_FIRETRUCK_PATH "/home/killian/Projects/C/Dual/Jeux/resources/3D/cars/OBJ format/firetruck.obj"
#define TEXTURE_DEFAULT_PATH "/home/killian/Projects/C/Dual/Jeux/resources/3D/skeletalModels/default_texture.png"

float random_value(float min, float max) {
    float scale = (float)rand() / (float)RAND_MAX;
    return min + scale * (max - min);
}

DUAL_RigidBody add_rigid_body() {
    DUAL_RigidBody body;
    body.masse = 1.0f;
    body.masse_inverse = 1.0f;
    body.est_statique = false;
    body.actif = true;
    body.amortissement = 0.98f;
    body.position = (DUAL_Vec3){random_value(-50.0f, 50.0f), random_value(10.0f, 30.0f), random_value(-50.0f, 50.0f)};
    body.vitesse = (DUAL_Vec3){0.0f, 0.0f, 0.0f};
    body.forces = (DUAL_Vec3){0.0f, 0.0f, 0.0f};
    body.collider.type = DUAL_COLLIDER_AABB;
    body.collider.shape.aabb = (DUAL_AABB){(DUAL_Vec3){-1.0,-1.0,-1.0}, (DUAL_Vec3){1.0,1.0,1.0}};
    return body;
}

int main() {
    DUAL_Log(DUAL_LOG_INFO, "Test Graphics 2D started !");

    srand(time(NULL));

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

    DUAL_ResourceManager* resourceManager = NULL;
    DUAL_ResourceManager_Create(app, &resourceManager);

    DUAL_Renderer3D* renderer3D = NULL;
    result = DUAL_Renderer3D_Create(app, &renderer3D);
    DEBUG_DUAL_RESULT(result);
    DUAL_Renderer3D_SetCullMode(renderer3D, DUAL_CULL_BACK);

    DUAL_InputManager* inputManager = NULL;
    DUAL_InputManager_Create(app, &inputManager);

    int projectionMode = 0;
    int rendererMode = 0;

    DUAL_Model* ambulanceModel = NULL;
    DUAL_Material** materials = NULL;
    unsigned int materialCount = 0;
    result = DUAL_Model_Load(resourceManager, MODEL_FIRETRUCK_PATH, &ambulanceModel, &materialCount, &materials);
    DEBUG_DUAL_RESULT(result);

    DUAL_Texture* texture = NULL;
    result = DUAL_Texture_LoadFromFile(resourceManager, TEXTURE_DEFAULT_PATH, DUAL_FILTER_NEAREST, &texture);
    DEBUG_DUAL_RESULT(result);

    DUAL_PhysicsWorld* physicsWorld = DUAL_PhysicsWorld_Create((DUAL_Vec3){0.0, -9.81, 0.0});


    DUAL_RigidBody sol;
    sol.masse = 0.0f;
    sol.masse_inverse = 0.0f;
    sol.est_statique = true;
    sol.actif = true;
    sol.position = (DUAL_Vec3){0.0f, -5.0f, 0.0f};
    sol.vitesse = (DUAL_Vec3){0.0f, 0.0f, 0.0f};
    sol.forces = (DUAL_Vec3){0.0f, 0.0f, 0.0f};
    sol.collider.type = DUAL_COLLIDER_AABB;
    sol.collider.shape.aabb = (DUAL_AABB){(DUAL_Vec3){-60.0f, -5.5f, -60.0f}, (DUAL_Vec3){60.0f, -4.5f, 60.0f}};

    DUAL_PhysicsWorld_AddBody(physicsWorld, sol);

    double oldx = 0, oldy = 0;
    bool firstMouse = false;

    DUAL_Renderer3D_SetRenderMode(renderer3D, DUAL_RENDER_UNLIT);
    DUAL_Renderer3D_SetAmbientLight(renderer3D, (DUAL_Vec3){0.3f, 0.3f, 0.45f});
    DUAL_Light light = {
        .type = DUAL_LIGHT_POINT,
        .position = {3.0f, 0.0f, -2.5f},
        .couleur = {1.0f, 0.3f, 0.3f},
        .direction = {0.0f, 0.0f, 0.0f},
        .intensite = 0.5f,
        10
    };
    DUAL_Light sun = {
        .type = DUAL_LIGHT_DIRECTIONAL,
        .position = {3.0f, 0.0f, -2.5f},
        .couleur = {0.9f, 0.9f, 0.9f},
        .direction = {0.0f, -1.0f, -0.3f},
        .intensite = 0.6f
    };
    DUAL_Renderer3D_SetLight(renderer3D, 0, light);
    DUAL_Renderer3D_SetLight(renderer3D, 1, sun);

    DUAL_Renderer3D_SetCullMode(renderer3D, DUAL_CULL_BACK);

    while (DUAL_ShouldRun(app)) {
        DUAL_BeginFrame(app);

        DUAL_InputManager_Update(inputManager);

        if (DUAL_IsTouching(inputManager)) {
            double x, y;
            glfwGetCursorPos(DUAL_GetWindow(app), &x, &y);

            if (!firstMouse) {
                oldx = x;
                oldy = y;
                firstMouse = true;
            }

            DUAL_Camera3D_ProcessMouseMovement(DUAL_Renderer3D_GetCamera(renderer3D), x - oldx, y - oldy);
            oldx = x;
            oldy = y;
        } else {
            firstMouse = false;
        }

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

        if (DUAL_IsButtonDown(inputManager, DUAL_BUTTON_X)) {   // Touche V du clavier
            DUAL_PhysicsWorld_AddBody(physicsWorld, add_rigid_body());
        }

        if (DUAL_IsButtonPressed(inputManager, DUAL_BUTTON_A)) {    // Touche X du clavier
            rendererMode += 1;
            if (rendererMode > 2)
                rendererMode = 0;
            DUAL_Renderer3D_SetRenderMode(renderer3D, rendererMode);
        }

        DUAL_PhysicsWorld_Step(physicsWorld, DUAL_GetDeltaTime(app));

        DUAL_SetActiveScreen(app, DUAL_SCREEN_LEFT);

        DUAL_Renderer3D_Begin(renderer3D);

        for (int k = 0; k < physicsWorld->count_bodies; k++) {
            if (physicsWorld->bodies[k].est_statique) continue;

            DUAL_Transform3D transform;
            transform.position = physicsWorld->bodies[k].position;
            transform.rotation_euler_radians = (DUAL_Vec3){0.0f, M_PI, 0.0f};
            float taille = 3.0f;
            transform.echelle = (DUAL_Vec3){taille, taille, taille};
            DUAL_Debug_DrawAABB(renderer3D, DUAL_GetGlobalAABB(&physicsWorld->bodies[k]), DUAL_VEC3_COLOR_GREEN);
        }

        DUAL_Debug_DrawAABB(renderer3D, sol.collider.shape.aabb, DUAL_VEC3_COLOR_BLUE);

        DUAL_Renderer3D_End(renderer3D);

        DUAL_EndFrame(app);
    }

    DUAL_ResourceManager_Destroy(resourceManager);
    DUAL_Renderer3D_Destroy(renderer3D);
    DUAL_Shutdown(app);

    return EXIT_SUCCESS;
}