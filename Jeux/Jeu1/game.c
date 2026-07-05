#include <stdio.h>
#include "game_api.h" // Obligatoire pour la structure GameAPI

// On inclut les headers du SDK pour avoir accès aux fonctions DUAL_...
#include <string.h>
#include <math.h> // Nécessaire pour les conversions de degrés en radians

#include "dual_audio.h"
#include "dual_core.h"
#include "dual_fs.h"
#include "dual_math.h"
#include "dual_graphics_2d.h"
#include "dual_graphics_3d.h"
#include "dual_resources.h"
#include "dual_input.h"

static DUAL_Texture* bird_down = NULL;
static DUAL_Texture* bird_up = NULL;

static float rotation = 0.0f;

static float bird_y = 200.0f;      // Position verticale (y)
static float bird_velocity = 0.0f; // Vitesse actuelle
static const float gravity = 800.0f;     // Force qui tire l'oiseau vers le bas
static const float jump_strength = -300.0f; // Force du saut (négatif car y monte vers le haut)
int tex_w = 0, tex_h = 0;

// 1. Initialisation du jeu
void my_init(INCLUDES_FONCTIONS) {
    printf("Le jeu démarre !\n");

    // On récupère la racine de la clé configurée par le main
    const char* racine = DUAL_FS_GetCartridgeRoot();

    char chemin_absolu[512];

    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/sprites/yellowbird-downflap.png", racine);
    DUAL_Texture_LoadFromFile(resourceManager, chemin_absolu, DUAL_FILTER_LINEAR, &bird_down);
    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/sprites/yellowbird-upflap.png", racine);
    DUAL_Texture_LoadFromFile(resourceManager, chemin_absolu, DUAL_FILTER_LINEAR, &bird_up);

    if (bird_down) DUAL_Texture_GetSize(bird_down, &tex_w, &tex_h);
}

// 2. Logique du jeu (Calculs)
void my_update(float dt, INCLUDES_FONCTIONS) {
    // 1. Appliquer la gravité à la vitesse
    bird_velocity += gravity * dt;

    // 2. Vérifier l'input pour le saut
    if (DUAL_IsButtonPressed(inputManager, DUAL_BUTTON_UP)) {
        bird_velocity = jump_strength;
    }

    // 3. Mettre à jour la position en fonction de la vitesse
    bird_y += bird_velocity * dt;

    // 4. Calcul de l'effet de rotation de l'oiseau
    if (bird_velocity < 0) {
        // En phase de montée : on force l'angle vers le haut (-30 degrés environ)
        rotation = -30.0f * (M_PI / 180.0f);
    }
    else {
        // En phase de chute : on lisse l'angle de 0° à 90° max (1.57 radians) selon sa vitesse
        // Si la vitesse atteint ou dépasse 400, l'oiseau regarde droit vers le bas.
        float facteur_chute = bird_velocity / 400.0f;
        if (facteur_chute > 1.0f) facteur_chute = 1.0f;

        rotation = facteur_chute * (90.0f * (M_PI / 180.0f));
    }

    // 5. Sécurité : empêcher l'oiseau de sortir de l'écran
    if (bird_y > 480.0f) { bird_y = 480.0f; bird_velocity = 0; }
}

// 3. Affichage du jeu (Dessin)
void my_draw(INCLUDES_FONCTIONS) {
    // Sélection de l'écran virtuel ciblé (Top Screen)
    DUAL_SetActiveScreen(app, DUAL_SCREEN_TOP);

    // Un seul Begin suffit pour tout le rendu de cet écran
    DUAL_Renderer2D_Begin(renderer2D);

    DUAL_Texture* bird = NULL;
    // L'oiseau bat des ailes principalement lorsqu'il monte ou commence à planer
    if (bird_velocity < 150) {
        bird = bird_down;
    }
    else {
        bird = bird_up;
    }

    // Sécurité au cas où une texture a raté son chargement
    if (bird) {
        DUAL_SpriteParams params = {
            .texture = bird,
            .position = {200.0f, bird_y}, // 200 sur l'axe X pour le décentrer un peu vers la gauche
            .origine = { tex_w / 2.0f, tex_h / 2.0f },
            .echelle = { 1.0f, 1.0f },
            .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
            .rotation_radians = rotation,
            .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
            .mode_melange = DUAL_BLEND_ALPHA
        };
        DUAL_DrawSprite(renderer2D, &params);
    }

    DUAL_Renderer2D_End(renderer2D);
}

// 4. Nettoyage
void my_shutdown(INCLUDES_FONCTIONS) {
    printf("Fermeture du jeu, nettoyage des textures...\n");
    DUAL_Texture_Destroy(resourceManager, bird_down);
    DUAL_Texture_Destroy(resourceManager, bird_up);
}

// ========================================================================
// LE POINT D'ENTRÉE REQUIS PAR LE MOTEUR
// ========================================================================
void get_game_api(GameAPI* api) {
    api->initialized = true;
    api->init = my_init;
    api->update = my_update;
    api->draw = my_draw;
    api->shutdown = my_shutdown;
}