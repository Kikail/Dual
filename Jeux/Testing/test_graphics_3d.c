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

#define MODEL_AMBULANCE_PATH "/home/killian/Projects/C/Dual/Jeux/resources/3D/cars/OBJ format/ambulance.obj"
#define MODEL_FIRETRUCK_PATH "/home/killian/Projects/C/Dual/Jeux/resources/3D/cars/OBJ format/firetruck.obj"
#define MODEL_CHARACTER_PATH "/home/killian/Projects/C/Dual/Jeux/resources/3D/skeletalModels/Remy.dae"
#define TEXTURE_DEFAULT_PATH "/home/killian/Projects/C/Dual/Jeux/resources/3D/skeletalModels/default_texture.png"
#define ANIMATION_DANCE_PATH "/home/killian/Projects/C/Dual/Jeux/resources/3D/skeletalModels/Capoeira.dae"
#define ANIMATION_DANCE_2_PATH "/home/killian/Projects/C/Dual/Jeux/resources/3D/skeletalModels/Hip Hop Dancing.dae"
#define TEST_PATH "/home/killian/Projects/C/Dual/Jeux/resources/2D/Characters/man.png"

float random_value(float min, float max) {
    float scale = (float)rand() / (float)RAND_MAX;
    return min + scale * (max - min);
}

float current_rotation;
DUAL_Mat4* rotation;

DUAL_Vec3 init_position_function(void){
    return (DUAL_Vec3){0.0, 0.0, 5.0};
}
DUAL_Vec3 init_velocity_function(void){
    DUAL_Vec3 velocity = {random_value(-2.0, 2.0), random_value(10.0,30.0), random_value(-2.0, 2.0)};
    return DUAL_Mat4_MultiplyVector(*rotation, velocity);
}
DUAL_Vec3 init_scale_function(void){
    return (DUAL_Vec3){0.0, 0.0, 0.0};
}
DUAL_Vec3 init_color_function(void){
    return (DUAL_Vec3){0.0, 0.0, 0.0};
}
float init_lifetime(void){
    return (float)random_value(1.0, 3.0);
}

void particle_over_time(struct ParticleEmitter* emitter, Particle* particles, unsigned int nbParticles, float deltaTime) {
    current_rotation += deltaTime * 120.0;
    if (current_rotation >= 360.0)current_rotation = 0.0;
    DUAL_Mat4 rotx = DUAL_Mat4_Rotate((DUAL_Vec3){1.0,0.0,0.0}, DUAL_RAD(45.0));
    DUAL_Mat4 roty = DUAL_Mat4_Rotate((DUAL_Vec3){0.0,1.0,0.0}, DUAL_RAD(current_rotation));
    *rotation = DUAL_Mat4_Multiply(roty, rotx);

    for (unsigned int i = 0; i < nbParticles; i++) {
        particles[i].velocity.y += -9.81 * deltaTime;
        particles[i].position.x += particles[i].velocity.x * deltaTime;
        particles[i].position.y += particles[i].velocity.y * deltaTime;
        particles[i].position.z += particles[i].velocity.z * deltaTime;

        particles[i].size.x = 1-(particles[i].lifespan / particles[i].lifetime);
        particles[i].size.y = 1-(particles[i].lifespan / particles[i].lifetime);
        particles[i].size.z = 1-(particles[i].lifespan / particles[i].lifetime);

        particles[i].color.x = particles[i].lifespan / particles[i].lifetime;
        particles[i].color.y = particles[i].lifespan / particles[i].lifetime;
        particles[i].color.z = particles[i].lifespan / particles[i].lifetime;
    }
}

#define PARTICLE_NUMBER 20000

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

    // On initialise les ressources
    DUAL_Model* ambulanceModel = NULL;
    DUAL_Material** materials = NULL;
    unsigned int materialCount = 0;
    result = DUAL_Model_Load(resourceManager, MODEL_FIRETRUCK_PATH, &ambulanceModel, &materialCount, &materials);
    DEBUG_DUAL_RESULT(result);

    // On charge une texture de test
    DUAL_Texture* texture = NULL;
    result = DUAL_Texture_LoadFromFile(resourceManager, TEXTURE_DEFAULT_PATH, DUAL_FILTER_NEAREST, &texture);
    DEBUG_DUAL_RESULT(result);
    DUAL_Texture* texture2 = NULL;
    result = DUAL_Texture_LoadFromFile(resourceManager, TEST_PATH, DUAL_FILTER_NEAREST, &texture2);
    DEBUG_DUAL_RESULT(result);

    // Position de l'ambulance
    DUAL_Transform3D ambulanceTransform3D = {
        .position = {0.0, -2.0, 5.0},
        .echelle = {1.0, 1.0, 1.0}, // Ajustez selon la taille réelle du modèle
        .rotation_euler_radians = {0.0, M_PI, 0.0},
    };

    current_rotation = 0.0;
    DUAL_Mat4 mat = DUAL_Mat4_Identity();
    rotation = &mat;

    Particle particles[PARTICLE_NUMBER];
    ParticleEmitter* emitter = ParticleEmitter_Init(
        init_position_function,
        init_scale_function,
        init_color_function,
        init_velocity_function,
        init_lifetime,
        particle_over_time,
        particles,
        PARTICLE_NUMBER
    );
    ParticleSystem particle_system;
    ParticleSystem_Create(PARTICLE_NUMBER, particles, emitter, &particle_system);

    // Position du billboard
    DUAL_Transform3D billboard_transform = ambulanceTransform3D;
    billboard_transform.position.y += 2.0;

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

    DUAL_Renderer3D_SetCullMode(renderer3D, DUAL_CULL_BACK);

    // On creer les input
    DUAL_InputManager* inputManager = NULL;
    DUAL_InputManager_Create(app, &inputManager);

    int projectionMode = 0;
    int rendererMode = 0;

    // On utilise notre shader perso DUAL_Renderer3D_LoadShader(renderer3D, vertex_shader_skeleton_lit_src_main, fragment_shader_lit_src_main, &shader);
    DUAL_Shader shader;
    DUAL_Shader_load_VS_FS(&shader, "/home/killian/Projects/C/Dual/SDK/internal_resources/shaders/i_shader_3d_skeletal_base.vs", "/home/killian/Projects/C/Dual/SDK/internal_resources/shaders/i_shader_3d_unlit_base.fs", DUAL_SHADER_UNLIT);
    //DUAL_Renderer3D_UseCustomShader(renderer3D, &shader);
    DUAL_Renderer3D_UseShader(renderer3D, SHADER3D_LIT);

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
        ParticleSystem_Update(&particle_system, DUAL_GetDeltaTime(app));


        // On selectionne l'ecran du bas
        DUAL_SetActiveScreen(app, DUAL_SCREEN_LEFT);

        // On dessine nos images


        // On selectionne l'ecran du haut
        DUAL_SetActiveScreen(app, DUAL_SCREEN_RIGHT);

        // On dessine nos images
        DUAL_Renderer3D_Begin(renderer3D);
        DUAL_Debug_Draw_Model_BoundingBox(renderer3D, DUAL_Model_GetBoundingBox(ambulanceModel), ambulanceTransform3D ,DUAL_VEC3_COLOR_BLUE);
        DUAL_DrawModel(renderer3D, ambulanceModel, materials, ambulanceTransform3D);

        for (int i = 0; i < PARTICLE_NUMBER; i++) {
            if (particles[i].lifespan > 0) {
                DUAL_Transform3D tmp;
                tmp.position = particles[i].position;
                tmp.echelle = particles[i].size;
                DUAL_DrawBillboard(renderer3D, texture, tmp, particles[i].color);
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
