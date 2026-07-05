#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <dlfcn.h>

#include "dual_audio.h"
#include "dual_core.h"
#include "dual_fs.h"
#include "dual_math.h"
#include "dual_graphics_2d.h"
#include "dual_graphics_3d.h"
#include "dual_resources.h"
#include "game_api.h"
#include "dual_input.h"
#include <unistd.h>

#define ASSETS_PATH(file) ASSETS_DIR "/" file

bool EstCartoucheInseree(const char* chemin) {
    return (access(chemin, F_OK) == 0);
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    GameAPI game = game_api_create();

    const char* chemin_cartouche = "/media/killian/692B-8D17";
    bool carteLue = false;

    DUAL_App* app = NULL;
    DUAL_AppConfig config = {
        .titre_fenetre = "Test libdual - Double Ecran",
        .largeur_ecran = 400 * 2,
        .hauteur_ecran = 240 * 2,
        .plein_ecran = false,
        .vsync_actif = true,
        .fps_cible = 60
    };

    if (DUAL_Init(&config, &app) != DUAL_OK) {
        return -1;
    }


    DUAL_ResourceManager* resources = NULL;
    if (DUAL_ResourceManager_Create(app, &resources) != DUAL_OK) {
        DUAL_Log(DUAL_LOG_ERROR, "Impossible d'initialiser le Resource Manager.");
        DUAL_Shutdown(app);
        return -1;
    }
    // Ajustement optionnel des budgets memoire (ex: Limiter a 32Mo VRAM pour tester le debordement)
    DUAL_ResourceManager_SetVRAMBudget(resources, 32 * 1024 * 1024);
    DUAL_Renderer2D* renderer = NULL;
    DUAL_Renderer2D_Create(app, &renderer);
    DUAL_Renderer3D* renderer3D = NULL;
    DUAL_Renderer3D_Create(app, &renderer3D);


    // 1. Initialisation du moteur audio
    DUAL_AudioManager* audio = NULL;
    if (DUAL_AudioManager_Create(app, &audio) != 0) {
        printf("Erreur: Impossible d'initialiser l'audio.\n");
        return -1;
    }

    if (app == NULL) {
        DUAL_Log(DUAL_LOG_ERROR, "Application null");
        return -1;
    }

    while (DUAL_ShouldRun(app)) {
        DUAL_BeginFrame(app);

        if (glfwGetKey(DUAL_GetWindow(app), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            break;
        }

        if (EstCartoucheInseree(chemin_cartouche)) {
            if (!carteLue) {
                DUAL_Log(DUAL_LOG_INFO,"Carte inseree");
                carteLue = true;

                // 1. Chargement de la cartouche
                void* handle = dlopen("./game.so", RTLD_LAZY);
                if (handle == NULL) {
                    DUAL_Log(DUAL_LOG_ERROR, "Impossible d'initialiser le jeu.");
                }
                else {
                    void (*get_api)(GameAPI*) = dlsym(handle, "get_game_api");
                    get_api(&game);
                    if (game.initialized)
                        game.init(app, resources, renderer, renderer3D, audio);
                }
            }
        }
        else {
            if (carteLue) {
                DUAL_Log(DUAL_LOG_INFO,"Carte retiree");
                carteLue = false;
            }
        }

        if (game.initialized)
            game.update(DUAL_GetDeltaTime(app),app, resources, renderer, renderer3D, audio);

        DUAL_EndFrame(app);
    }

    if (game.initialized)
        game.shutdown(app, resources, renderer, renderer3D, audio);


    DUAL_ResourceManager_Destroy(resources);
    DUAL_Renderer2D_Destroy(renderer);
    DUAL_Renderer3D_Destroy(renderer3D);
    DUAL_Shutdown(app);


    return 0;
}