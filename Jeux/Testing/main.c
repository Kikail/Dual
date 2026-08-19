#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <dlfcn.h>

#include "../../SDK/include/DUAL_Audio/dual_audio.h"
#include "../../SDK/include/DUAL_Core/dual_core.h"
#include "../../SDK/include/DUAL_FS/dual_fs.h"
#include "../../SDK/include/DUAL_Math/dual_math.h"
#include "../../SDK/include/DUAL_Graphics/dual_graphics_2d.h"
#include "../../SDK/include/DUAL_Graphics/dual_graphics_3d.h"
#include "../../SDK/include/DUAL_Resources/dual_resources.h"
#include "game_api.h"
#include "../../SDK/include/DUAL_Input/dual_input.h"
#include <unistd.h>

#define ASSETS_PATH(file) ASSETS_DIR "/" file

bool EstCartoucheInseree(const char* chemin) {
    return (access(chemin, F_OK) == 0);
}

/*
 *
 * Fonctionnalites a ajouter:
 *  - Shaders perso OK sauf lecture de shader via fichier + il faut fournir des shaders d exemple aux utilisateurs
 *  - Attenuation du son avec la distance 2D ou 3D
 *  - Systeme de particules
 *  - Simples collisions 2D/3D ainsi que des Raycast
 *  - Des tilemaps
 *  - Transparence sur les shaders
 *  - Frustum culling ( ne pas afficher les modeles hors du champs de vision )
 *  - Systeme de LOD
 *  - Billboards ( Sprite dans un environnement 3D, regarde toujours la camera )
 *  - Overlay de debug qui affiche les stats CPU et GPU
 *
 *  - Systeme de build de game, construit un fichier contenant toutes les informations
 *  d'un jeu comme son executable et ses ressources. Tout comme un fichier .nds
 *  Dual lira alors ce fichier la et pourra interpreter ce fichier comme un jeu.
 *
*/

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    GameAPI game = game_api_create();

    //const char* chemin_cartouche = "/media/killian/692B-8D17";
    const char* chemin_cartouche = "/home/killian/Projects/C/DUAL_Games/Game01";
    bool carteLue = false;

    DUAL_App* app = NULL;
    DUAL_AppConfig config = {
        .titre_fenetre = "Test libdual - Double Ecran",
        .largeur_ecran = 400 * 2,
        .hauteur_ecran = 240 * 2,
        .plein_ecran = true,
        .vsync_actif = true,
        .fps_cible = 60
    };

    if (DUAL_Init(&config, &app) != DUAL_OK) {
        return -1;
    }

    DUAL_InputManager* inputManager = NULL;
    if (DUAL_InputManager_Create(app, &inputManager) != DUAL_OK) {
        DUAL_Log(DUAL_LOG_ERROR, "Impossible d'initialiser l'inputManager.");
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
        DUAL_Log(DUAL_LOG_ERROR, "Impossible d'initialiser l'audio.");
        return -1;
    }

    if (app == NULL) {
        DUAL_Log(DUAL_LOG_ERROR, "Application null");
        return -1;
    }

    void* handle = NULL;

    while (DUAL_ShouldRun(app)) {
        DUAL_BeginFrame(app);
        DUAL_InputManager_Update(inputManager);

        if (glfwGetKey(DUAL_GetWindow(app), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            break;
        }

        if (EstCartoucheInseree(chemin_cartouche)) {
            if (!carteLue) {
                DUAL_Log(DUAL_LOG_INFO, "Carte inseree !");
                carteLue = true;

                // A. On configure le SDK pour chercher les fichiers sur la clé USB
                // Au lieu de "cartridge_data/", on lui donne le chemin de la clé
                DUAL_FS_SetCartridgeRoot(chemin_cartouche);

                // B. On construit le chemin dynamique vers le fichier game.so sur la clé
                char chemin_so[512];
                snprintf(chemin_so, sizeof(chemin_so), "%s/game.so", chemin_cartouche);

                // C. On charge le .so depuis la clé USB
                handle = dlopen(chemin_so, RTLD_LAZY);
                if (handle == NULL) {
                    DUAL_Log(DUAL_LOG_ERROR, "Impossible de charger le jeu depuis la cle USB.");
                    DUAL_Log(DUAL_LOG_ERROR, dlerror()); // Affiche l'erreur système exacte si ça rate
                }
                else {
                    void (*get_api)(GameAPI*) = dlsym(handle, "get_game_api");
                    get_api(&game);
                    if (game.initialized)
                        game.init(app, resources, renderer, renderer3D, audio, inputManager);
                    DUAL_MemoryStats stats;
                    DUAL_ResourceManager_GetStats(resources, &stats);
                    DUAL_Log(DUAL_LOG_INFO, "Etat de la memoire: [%u/%u]", stats.vram_utilisee_octets, stats.vram_totale_octets);
                }
            }
        }
        else {
            if (carteLue) {
                DUAL_Log(DUAL_LOG_INFO,"Carte retiree");

                if (game.initialized) {
                    game.shutdown(app, resources, renderer, renderer3D, audio, inputManager);
                }

                game.initialized = false;
                carteLue = false;
                DUAL_SetScreenClearColor(app, DUAL_SCREEN_TOP, (DUAL_Color){0.1f, 0.6f, 0.2f, 1.0f});
                DUAL_SetScreenClearColor(app, DUAL_SCREEN_BOTTOM, (DUAL_Color){0.1f, 0.3f, 0.6f, 1.0f});

                DUAL_AudioManager_SetChannelVolume(audio, DUAL_CHANNEL_MASTER, 0.0f);
                DUAL_ResourceManager_PurgeAll(resources);

                if (handle) {
                    dlclose(handle);
                    DUAL_Log(DUAL_LOG_INFO, "Le fichier .so est unloaded");
                }

                DUAL_MemoryStats stats;
                DUAL_ResourceManager_GetStats(resources, &stats);
                DUAL_Log(DUAL_LOG_INFO, "Etat de la memoire: [%u/%u]", stats.vram_utilisee_octets, stats.vram_totale_octets);
            }
        }

        if (game.initialized)
            game.update(DUAL_GetDeltaTime(app),app, resources, renderer, renderer3D, audio, inputManager);

        if (game.initialized)
            game.draw(app, resources, renderer, renderer3D, audio, inputManager);

        DUAL_EndFrame(app);
    }

    if (game.initialized) {
        game.shutdown(app, resources, renderer, renderer3D, audio, inputManager);
        game.initialized = false;
    }

    DUAL_InputManager_Destroy(inputManager);
    DUAL_ResourceManager_Destroy(resources);
    DUAL_Renderer2D_Destroy(renderer);
    DUAL_Renderer3D_Destroy(renderer3D);
    DUAL_Shutdown(app);


    return 0;
}