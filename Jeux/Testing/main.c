#include "dual_core.h"
#include <stdio.h>

int main() {
    DUAL_AppConfig config = {
        .titre_fenetre = "Test de ma Console",
        .largeur_ecran = 800,
        .hauteur_ecran = 480,
        .plein_ecran = false,
        .vsync_actif = true
    };

    DUAL_App* app = NULL;
    if (DUAL_Init(&config, &app) == DUAL_OK) {
        while (DUAL_ShouldRun(app)) {
            DUAL_BeginFrame(app);
            
            // Le rendu ira ici !
            
            DUAL_EndFrame(app);
        }
    }
    DUAL_Shutdown(app);
    return 0;
}