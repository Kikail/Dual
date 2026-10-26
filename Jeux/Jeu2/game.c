#include <stdio.h>
#include "game_api.h" // Obligatoire pour la structure GameAPI
#include "DUAL_Core/dual_core.h"
#include "DUAL_Graphics/dual_graphics_3d.h"


void my_init(INCLUDES_FONCTIONS) {

}

void my_update(float dt, INCLUDES_FONCTIONS) {

}


void my_draw(INCLUDES_FONCTIONS) {
    DUAL_SetActiveScreen(app, DUAL_SCREEN_MAIN);
    DUAL_Renderer3D_Begin(renderer3D);

    DUAL_SetScreenClearColor(app, DUAL_SCREEN_MAIN, (DUAL_Color){0.3, 0.3, 0.3});

    DUAL_Renderer3D_End(renderer3D);
}

void my_shutdown(INCLUDES_FONCTIONS) {
    printf("Fermeture du jeu, nettoyage des ressources...\n");


}

void get_game_api(GameAPI* api) {
    api->initialized = true;
    api->init = my_init;
    api->update = my_update;
    api->draw = my_draw;
    api->shutdown = my_shutdown;
}