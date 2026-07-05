#ifndef GAME_API_H
#define GAME_API_H

#include <stdbool.h>

// On pré-déclare les structures sans charger tout le SDK
// Cela évite les erreurs de type inconnu (unknown type name)
typedef struct DUAL_App DUAL_App;
typedef struct DUAL_ResourceManager DUAL_ResourceManager;
typedef struct DUAL_Renderer2D DUAL_Renderer2D;
typedef struct DUAL_Renderer3D DUAL_Renderer3D;
typedef struct DUAL_AudioManager DUAL_AudioManager;
typedef struct DUAL_InputManager DUAL_InputManager;

#define INCLUDES_FONCTIONS DUAL_App* app, DUAL_ResourceManager* resourceManager, DUAL_Renderer2D* renderer2D, DUAL_Renderer3D* renderer3D, DUAL_AudioManager* audioManager, DUAL_InputManager* inputManager

// Définition de l'API
typedef struct {
    bool initialized;
    void (*init)(INCLUDES_FONCTIONS);
    void (*update)(float dt, INCLUDES_FONCTIONS);
    void (*draw)(INCLUDES_FONCTIONS);
    void (*shutdown)(INCLUDES_FONCTIONS);
} GameAPI;

// Déclarations des fonctions globales du module
void get_game_api(GameAPI* api);

static inline GameAPI game_api_create(void) {
    GameAPI api;
    api.initialized = false;
    api.init = NULL;
    api.update = NULL;
    api.draw = NULL;
    api.shutdown = NULL;
    return api;
}

#endif // GAME_API_H