#include <stdio.h>
#include "game_api.h" // Obligatoire pour la structure GameAPI

// On inclut les headers du SDK pour avoir accès aux fonctions DUAL_...
#include <string.h>
#include <math.h> // Nécessaire pour les conversions de degrés en radians
#include <stdlib.h>
#include <time.h>

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
static DUAL_Font* font = NULL;
static DUAL_Texture* background_day = NULL;
static DUAL_Texture* ground = NULL;
static DUAL_Texture* pipe = NULL;
static DUAL_Texture* menu = NULL;
static DUAL_Texture* bottom = NULL;
static DUAL_Texture* bottom_buttom = NULL;
static DUAL_Texture* gameoverTexture = NULL;

static DUAL_SoundInstance* wingInstance = NULL;
static DUAL_SoundInstance* hitInstance = NULL;
static DUAL_Sound* soundHit = NULL;
static DUAL_Sound* soundWing = NULL;

float rotation = 0.0f;
float bird_y = 200.0f;
float bird_velocity = 0.0f;
const float gravity = 800.0f;
const float jump_strength = -300.0f;
int tex_w = 0, tex_h = 0;
int groundH = 0;
bool pressed = false;

bool gameStarted = false;
bool gameOver = false;

DUAL_Vec2 backgrounds[4];
DUAL_Vec2 grounds[4];

typedef struct Pipe {
    DUAL_Vec2 pos;
    int espacement;
} Pipe;
Pipe pipes[3];

#define VITESSE_BACKGROUND 30.0
#define VITESSE_GROUD 90.0
#define ESPACEMENT_TUYAU 55.0

DUAL_Rect tuyau_bas;
DUAL_Rect tuyau_haut;

int RandomPipeHeight() {
    return (rand() % 140) + 160;
}

void init_world(INCLUDES_FONCTIONS) {
    for (int i = 0; i < 3; i++) {
        pipes[i].pos.x = i * 300 + 900;
        pipes[i].pos.y = RandomPipeHeight();
        pipes[i].espacement = ESPACEMENT_TUYAU;
    }
}

void my_init(INCLUDES_FONCTIONS) {
    // 1. VRAIE SÉCURITÉ : On s'assure que tout commence à NULL
    bird_down = NULL;
    bird_up = NULL;
    font = NULL;
    background_day = NULL;
    ground = NULL;
    pipe = NULL;
    menu = NULL;
    bottom = NULL;
    bottom_buttom = NULL;
    gameoverTexture = NULL;
    soundHit = NULL;
    soundWing = NULL;
    wingInstance = NULL;
    hitInstance = NULL;

    printf("Le jeu démarre !\n");
    const char* racine = DUAL_FS_GetCartridgeRoot();
    char chemin_absolu[512];

    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/sprites/yellowbird-downflap.png", racine);
    DUAL_Texture_LoadFromFile(resourceManager, chemin_absolu, DUAL_FILTER_LINEAR, &bird_down);

    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/sprites/yellowbird-upflap.png", racine);
    DUAL_Texture_LoadFromFile(resourceManager, chemin_absolu, DUAL_FILTER_LINEAR, &bird_up);

    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/sprites/background-day.png", racine);
    DUAL_Texture_LoadFromFile(resourceManager, chemin_absolu, DUAL_FILTER_LINEAR, &background_day);

    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/sprites/base.png", racine);
    DUAL_Texture_LoadFromFile(resourceManager, chemin_absolu, DUAL_FILTER_LINEAR, &ground);

    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/sprites/pipe-green.png", racine);
    DUAL_Texture_LoadFromFile(resourceManager, chemin_absolu, DUAL_FILTER_LINEAR, &pipe);

    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/sprites/message.png", racine);
    DUAL_Texture_LoadFromFile(resourceManager, chemin_absolu, DUAL_FILTER_LINEAR, &menu);

    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/sprites/bottom.png", racine);
    DUAL_Texture_LoadFromFile(resourceManager, chemin_absolu, DUAL_FILTER_LINEAR, &bottom);

    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/sprites/bottom_button.png", racine);
    DUAL_Texture_LoadFromFile(resourceManager, chemin_absolu, DUAL_FILTER_LINEAR, &bottom_buttom);

    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/sprites/gameover.png", racine);
    DUAL_Texture_LoadFromFile(resourceManager, chemin_absolu, DUAL_FILTER_LINEAR, &gameoverTexture);

    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/audio/hit.wav", racine);
    DUAL_Sound_LoadFromFile(audioManager, resourceManager, chemin_absolu, &soundHit);

    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/audio/wing.wav", racine);
    DUAL_Sound_LoadFromFile(audioManager, resourceManager, chemin_absolu, &soundWing);

    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/arial.ttf", racine);
    if (DUAL_Font_LoadFromFile(resourceManager, chemin_absolu, 80, &font) != DUAL_OK){
        DUAL_Log(DUAL_LOG_ERROR, "Erreur de chargement du font");
        font = NULL;
    }

    int w = 0, h = 0;

    // On initialise les backgrounds avec des variables locales pour éviter d'écraser tex_w/tex_h globaux
    if (background_day) {
        DUAL_Texture_GetSize(background_day, &w, &h);
        for (int i = 0; i < 4; i++) {
            backgrounds[i].x = i * w;
            backgrounds[i].y = 0;
        }
    }

    // On initialise le sol
    if (ground) {
        DUAL_Texture_GetSize(ground, &w, &h);
        for (int i = 0; i < 4; i++) {
            grounds[i].x = i * w;
            grounds[i].y = 480 - h;
        }
        groundH = h - 10.0;
    }

    init_world(PARAMS_FONCTIONS);
    srand(time(NULL));

    DUAL_SetScreenClearColor(app, DUAL_SCREEN_BOTTOM,(DUAL_Color){0.874, 0.85, 0.588, 1.0});

    gameStarted = false;
    gameOver = false;
    rotation = 0.0;
    bird_y = 200.0;
    bird_velocity = 0.0f;
    pressed = false;
}

void deplacements_oiseau(float dt, INCLUDES_FONCTIONS) {
    bird_velocity += gravity * dt;
    DUAL_TouchState touch = DUAL_GetTouchState(inputManager);

    if (touch.phase == DUAL_TOUCH_STARTED) {
        bird_velocity = jump_strength;
        if (wingInstance) {
            DUAL_SoundInstance_Play(audioManager, wingInstance);
        } else if (soundWing) {
            wingInstance = DUAL_Sound_Play(audioManager, soundWing, 1.0, 1.0);
        }
    }

    bird_y += bird_velocity * dt;

    if (bird_velocity < 0) {
        rotation = -30.0f * (M_PI / 180.0f);
    } else {
        float facteur_chute = bird_velocity / 400.0f;
        if (facteur_chute > 1.0f) facteur_chute = 1.0f;
        rotation = facteur_chute * (90.0f * (M_PI / 180.0f));
    }

    // Collision avec le sol
    if (bird_y > 480.0f - groundH - 10.0) {
        bird_y = 480.0f - groundH - 10.0;
        bird_velocity = 0;

        // CORRECTION : On ne joue le son QUE au moment du passage à gameOver
        if (!gameOver) {
            gameOver = true;
            if (hitInstance) {
                DUAL_SoundInstance_Play(audioManager, hitInstance);
            } else if (soundHit) {
                hitInstance = DUAL_Sound_Play(audioManager, soundHit, 1.0, 1.0);
            }
        }
    }

    if (bird_y < 10.0) {
        bird_y = 10.0;
        bird_velocity = 100;
    }
}

void deplacement_backgrounds(float dt, INCLUDES_FONCTIONS) {
    if (!background_day) return;
    DUAL_Texture_GetSize(background_day, &tex_w, &tex_h);
    for (int i = 0; i < 4; i++) {
        backgrounds[i].x -= VITESSE_BACKGROUND * dt;
        if (backgrounds[i].x < -tex_w) {
            backgrounds[i].x += 4 * tex_w;
        }
    }
}

void deplacement_grounds(float dt, INCLUDES_FONCTIONS) {
    if (!ground) return;
    DUAL_Texture_GetSize(ground, &tex_w, &tex_h);
    for (int i = 0; i < 4; i++) {
        grounds[i].x -= VITESSE_GROUD * dt;
        if (grounds[i].x < -tex_w) {
            grounds[i].x += 4 * tex_w;
        }
    }
}

void deplacement_pipes(float dt, INCLUDES_FONCTIONS) {
    if (!pipe) return;
    DUAL_Texture_GetSize(pipe, &tex_w, &tex_h);
    for (int i = 0; i < 3; i++) {
        pipes[i].pos.x -= VITESSE_GROUD * dt;
        if (pipes[i].pos.x < -tex_w) {
            pipes[i].pos.x += 3 * 300;
            pipes[i].pos.y = RandomPipeHeight();
        }
    }
}

void handleCollisions(float dt, INCLUDES_FONCTIONS) {
    if (!bird_down || !pipe) return;

    int w_ois, h_ois, w_pipe, h_pipe;
    DUAL_Texture_GetSize(bird_down, &w_ois, &h_ois);
    DUAL_Texture_GetSize(pipe, &w_pipe, &h_pipe);

    DUAL_Rect collider_oiseau = {200.0f - (w_ois / 2.0f), bird_y - (h_ois / 2.0f), w_ois, h_ois};

    for (int i = 0; i < 3; i++) {
        tuyau_bas = (DUAL_Rect){pipes[i].pos.x - (w_pipe / 2.0f), pipes[i].pos.y + ESPACEMENT_TUYAU, w_pipe, h_pipe};
        tuyau_haut = tuyau_bas;
        tuyau_haut.y = pipes[i].pos.y - ESPACEMENT_TUYAU - h_pipe;

        if (DUAL_CollideRectRect(collider_oiseau, tuyau_haut) || DUAL_CollideRectRect(collider_oiseau, tuyau_bas)) {
            gameOver = true;
            if (hitInstance) {
                DUAL_SoundInstance_Play(audioManager, hitInstance);
            } else if (soundHit) {
                hitInstance = DUAL_Sound_Play(audioManager, soundHit, 1.0, 1.0);
            }
            break;
        }
    }
}

void my_update(float dt, INCLUDES_FONCTIONS) {
    pressed = DUAL_IsTouching(inputManager);
    DUAL_TouchState touch = DUAL_GetTouchState(inputManager);

    // ÉTAT 1 : Le jeu est en cours (Pas de Game Over et Jeu Démarré)
    if (gameStarted && !gameOver) {
        deplacements_oiseau(dt, PARAMS_FONCTIONS);
        deplacement_backgrounds(dt, PARAMS_FONCTIONS);
        deplacement_grounds(dt, PARAMS_FONCTIONS);
        deplacement_pipes(dt, PARAMS_FONCTIONS);
        handleCollisions(dt, PARAMS_FONCTIONS);
    }
    // ÉTAT 2 : Écran de Game Over (Le joueur a perdu)
    else if (gameOver) {
        // Si on clique sur l'écran de game over, on RESET TOUT pour revenir au Menu
        if (touch.phase == DUAL_TOUCH_STARTED) {
            gameOver = false;
            gameStarted = false; // On retourne à l'état Menu d'abord
            rotation = 0.0f;
            bird_y = 200.0f;
            bird_velocity = 0.0f;
            pressed = false;
            init_world(PARAMS_FONCTIONS);
        }
    }
    // ÉTAT 3 : Écran de Menu de départ (!gameStarted && !gameOver)
    else {
        if (touch.phase == DUAL_TOUCH_STARTED) {
            gameStarted = true;
            // On applique le premier saut directement pour lancer l'oiseau
            bird_velocity = jump_strength;
            if (wingInstance) {
                DUAL_SoundInstance_Play(audioManager, wingInstance);
            } else if (soundWing) {
                wingInstance = DUAL_Sound_Play(audioManager, soundWing, 1.0, 1.0);
            }
        }
    }
}

void affichage_oiseau(INCLUDES_FONCTIONS) {
    DUAL_Texture* bird = (bird_velocity < 150) ? bird_down : bird_up;
    if (!bird) return;

    DUAL_Texture_GetSize(bird, &tex_w, &tex_h);
    DUAL_SpriteParams params = {
        .texture = bird,
        .position = {200.0f, bird_y},
        .origine = { tex_w / 2.0f, tex_h / 2.0f },
        .echelle = { 1.0f, 1.0f },
        .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
        .rotation_radians = rotation,
        .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
        .mode_melange = DUAL_BLEND_ALPHA
    };
    DUAL_DrawSprite(renderer2D, &params);
}

void affichage_background(INCLUDES_FONCTIONS) {
    if (!background_day) return;
    DUAL_Texture_GetSize(background_day, &tex_w, &tex_h);
    for (int i = 0; i < 4; i++) {
        DUAL_SpriteParams params = {
            .texture = background_day,
            .position = backgrounds[i],
            .origine = { 0.0f, 0.0f },
            .echelle = { 1.0f, 1.0f },
            .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
            .rotation_radians = 0.0f,
            .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
            .mode_melange = DUAL_BLEND_ALPHA
        };
        DUAL_DrawSprite(renderer2D, &params);
    }
}

void affichage_ground(INCLUDES_FONCTIONS) {
    if (!ground) return;
    DUAL_Texture_GetSize(ground, &tex_w, &tex_h);
    for (int i = 0; i < 4; i++) {
        DUAL_SpriteParams params = {
            .texture = ground,
            .position = grounds[i],
            .origine = { 0.0f, 0.0f },
            .echelle = { 1.0f, 1.0f },
            .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
            .rotation_radians = 0.0f,
            .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
            .mode_melange = DUAL_BLEND_ALPHA
        };
        DUAL_DrawSprite(renderer2D, &params);
    }
}

void affichage_menu(INCLUDES_FONCTIONS) {
    if (!menu) return;
    DUAL_Texture_GetSize(menu, &tex_w, &tex_h);
    DUAL_SpriteParams paramsHaut = {
        .texture = menu,
        .position = {400.0f, 240.0f},
        .origine = { tex_w / 2.0f, tex_h / 2.0f },
        .echelle = { 1.0f, 1.0f },
        .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
        .rotation_radians = 0.0f,
        .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
        .mode_melange = DUAL_BLEND_ALPHA
    };
    DUAL_DrawSprite(renderer2D, &paramsHaut);
}

void affichage_tuyaux(INCLUDES_FONCTIONS) {
    if (!pipe) return;
    DUAL_Texture_GetSize(pipe, &tex_w, &tex_h);

    for (int i = 0; i < 3; i++) {
        // tuyau bas
        DUAL_SpriteParams params = {
            .texture = pipe,
            .position = {pipes[i].pos.x, pipes[i].pos.y + pipes[i].espacement},
            .origine = { tex_w / 2.0f, 0.0f },
            .echelle = { 1.0f, 1.0f },
            .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
            .rotation_radians = 0.0f,
            .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
            .mode_melange = DUAL_BLEND_ALPHA
        };
        DUAL_DrawSprite(renderer2D, &params);

        // tuyau haut
        DUAL_SpriteParams paramsHaut = {
            .texture = pipe,
            .position = {pipes[i].pos.x, pipes[i].pos.y - pipes[i].espacement},
            .origine = { tex_w / 2.0f, 0.0f },
            .echelle = { 1.0f, 1.0f },
            .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
            .rotation_radians = M_PI,
            .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
            .mode_melange = DUAL_BLEND_ALPHA
        };
        DUAL_DrawSprite(renderer2D, &paramsHaut);
    }
}

void affichage_bas(INCLUDES_FONCTIONS) {
    DUAL_Texture* tex_render = pressed ? bottom : bottom_buttom;
    if (!tex_render) return;

    DUAL_Texture_GetSize(tex_render, &tex_w, &tex_h);
    DUAL_SpriteParams params = {
        .texture = tex_render,
        .position = {400.0f, 240.0f},
        .origine = { tex_w / 2.0f, tex_h / 2.0f },
        .echelle = { 1.0f, 1.0f },
        .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
        .rotation_radians = 0.0f,
        .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
        .mode_melange = DUAL_BLEND_ALPHA
    };
    DUAL_DrawSprite(renderer2D, &params);
}

void affichage_gameover(INCLUDES_FONCTIONS) {
    if (!gameoverTexture) return;
    DUAL_Texture_GetSize(gameoverTexture, &tex_w, &tex_h);
    DUAL_SpriteParams params = {
        .texture = gameoverTexture,
        .position = {400.0f, 240.0f},
        .origine = { tex_w / 2.0f, tex_h / 2.0f },
        .echelle = { 1.0f, 1.0f },
        .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
        .rotation_radians = 0.0f,
        .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
        .mode_melange = DUAL_BLEND_ALPHA
    };
    DUAL_DrawSprite(renderer2D, &params);
}

void my_draw(INCLUDES_FONCTIONS) {
    DUAL_SetActiveScreen(app, DUAL_SCREEN_TOP);
    DUAL_Renderer2D_Begin(renderer2D);

    affichage_background(PARAMS_FONCTIONS);
    affichage_tuyaux(PARAMS_FONCTIONS);
    affichage_ground(PARAMS_FONCTIONS);
    affichage_oiseau(PARAMS_FONCTIONS);

    DUAL_Renderer2D_End(renderer2D);

    DUAL_SetActiveScreen(app, DUAL_SCREEN_BOTTOM);
    DUAL_Renderer2D_Begin(renderer2D);

    affichage_bas(PARAMS_FONCTIONS);

    if (!gameStarted && !gameOver) {
        affichage_menu(PARAMS_FONCTIONS);
    }
    else if (gameOver) {
        affichage_gameover(PARAMS_FONCTIONS);
    }

    DUAL_Renderer2D_End(renderer2D);
}

void my_shutdown(INCLUDES_FONCTIONS) {
    printf("Fermeture du jeu, nettoyage des ressources...\n");

    // Libération de TOUTES les textures
    if (bird_down)       DUAL_Texture_Destroy(resourceManager, bird_down);
    if (bird_up)         DUAL_Texture_Destroy(resourceManager, bird_up);
    if (background_day)  DUAL_Texture_Destroy(resourceManager, background_day);
    if (ground)          DUAL_Texture_Destroy(resourceManager, ground);
    if (pipe)            DUAL_Texture_Destroy(resourceManager, pipe);
    if (menu)            DUAL_Texture_Destroy(resourceManager, menu);
    if (bottom)          DUAL_Texture_Destroy(resourceManager, bottom);
    if (bottom_buttom)   DUAL_Texture_Destroy(resourceManager, bottom_buttom);
    if (gameoverTexture) DUAL_Texture_Destroy(resourceManager, gameoverTexture);

    // Libération de la police
    if (font)            DUAL_Font_Destroy(resourceManager, font);

    // Libération des fichiers sons statiques
    if (soundHit)        DUAL_Sound_Destroy(resourceManager, soundHit);
    if (soundWing)       DUAL_Sound_Destroy(resourceManager, soundWing);

    // Remise à zéro des pointeurs globaux pour des raisons de sécurité
    bird_down = NULL;
    bird_up = NULL;
    background_day = NULL;
    ground = NULL;
    pipe = NULL;
    menu = NULL;
    bottom = NULL;
    bottom_buttom = NULL;
    gameoverTexture = NULL;
    font = NULL;
    soundHit = NULL;
    soundWing = NULL;
    wingInstance = NULL;
    hitInstance = NULL;
}

void get_game_api(GameAPI* api) {
    api->initialized = true;
    api->init = my_init;
    api->update = my_update;
    api->draw = my_draw;
    api->shutdown = my_shutdown;
}