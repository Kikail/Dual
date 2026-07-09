//
// Created by killian on 7/9/26.
//
#include <stdio.h>
#include <stdlib.h>

#include "dual_core.h"
#include "dual_utils.h"

int main() {
    DUAL_Log(DUAL_LOG_INFO, "Dual App started !");

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

    // On peut changer la couleur de fond
    DUAL_SetScreenClearColor(app, DUAL_SCREEN_BOTTOM, DUAL_COLOR_WHITE);
    DUAL_SetScreenClearColor(app, DUAL_SCREEN_TOP, DUAL_COLOR_BLACK);

    // On creer un compteur pour afficher les stats toutes les 10 secondes
    float compteur = 0.0;
    float duree = 10.0;

    // Boucle du jeu principal
    while (DUAL_ShouldRun(app)) {
        DUAL_BeginFrame(app);

        // Compteur pour afficher les stats
        compteur += DUAL_GetDeltaTime(app);
        if (compteur >= duree) {
            compteur = 0.0;
            DUAL_Log(DUAL_LOG_INFO, "Stats: [FPS: %d][DeltaTime: %f][Time: %f]", DUAL_GetFPS(app), DUAL_GetDeltaTime(app), DUAL_GetTime(app));
        }

        DUAL_EndFrame(app);
    }

    // On ferme proprement l'application
    DUAL_Shutdown(app);

    return EXIT_SUCCESS;
}
