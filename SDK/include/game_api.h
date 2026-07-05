#ifndef GAME_API_H
#define GAME_API_H

#define INCLUDES_FONCTIONS DUAL_App* app, DUAL_ResourceManager* resourceManager, DUAL_Renderer2D* renderer2D, DUAL_Renderer3D* renderer3D, DUAL_AudioManager* audioManager

// On définit les fonctions que ton moteur va appeler dans le jeu
typedef struct {
    bool initialized;
    void (*init)(INCLUDES_FONCTIONS);
    void (*update)(float dt, INCLUDES_FONCTIONS);
    void (*draw)(INCLUDES_FONCTIONS);
    void (*shutdown)(INCLUDES_FONCTIONS);
} GameAPI;

// La fonction que le moteur appellera pour récupérer les pointeurs
// C'est le "point d'entrée" de ta cartouche
void get_game_api(GameAPI* api);
GameAPI game_api_create() {
    GameAPI api;
    api.initialized = false;
    return api;
}

#endif