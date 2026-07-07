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
}Pipe;
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

    // Vérifie bien que ce fichier existe sur ta carte SD à cet emplacement exact !
    snprintf(chemin_absolu, sizeof(chemin_absolu), "%s/assets/arial.ttf", racine);

    if (DUAL_Font_LoadFromFile(resourceManager, chemin_absolu, 80, &font) != DUAL_OK){
        DUAL_Log(DUAL_LOG_ERROR, "Erreur de chargement du font");
        font = NULL; // On force à NULL en cas d'échec
    }

    // On initialise les backgrounds
    DUAL_Texture_GetSize(background_day, &tex_w, &tex_h);
    for (int i = 0; i < 4; i++) {
        backgrounds[i].x = i * tex_w;
    }

    // On initialise les backgrounds
    DUAL_Texture_GetSize(ground, &tex_w, &tex_h);
    for (int i = 0; i < 4; i++) {
        grounds[i].x = i * tex_w;
        grounds[i].y = 480 - tex_h;
    }
    groundH = tex_h - 10.0;

    init_world(PARAMS_FONCTIONS);

    srand(time(NULL));

    DUAL_SetScreenBottomClearColor(app, (DUAL_Vec4){0.874, 0.85, 0.588, 1.0});

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
    }
    bird_y += bird_velocity * dt;
    if (bird_velocity < 0) {
        // En phase de montée : on force l'angle vers le haut (-30 degrés environ)
        rotation = -30.0f * (M_PI / 180.0f);
    }
    else {
        float facteur_chute = bird_velocity / 400.0f;
        if (facteur_chute > 1.0f) facteur_chute = 1.0f;
        rotation = facteur_chute * (90.0f * (M_PI / 180.0f));
    }
    if (bird_y > 480.0f - groundH - 10.0) {
        bird_y = 480.0f - groundH - 10.0;
        bird_velocity = 0;
        gameOver = true;
    }
    if (bird_y < 10.0) {
        bird_y = 10.0;
        bird_velocity = 100;
    }
}

void deplacement_backgrounds(float dt, INCLUDES_FONCTIONS) {
    DUAL_Texture_GetSize(background_day, &tex_w, &tex_h);
    for (int i = 0; i < 4; i++) {
        backgrounds[i].x -= VITESSE_BACKGROUND * dt;
        // Si la texture n est plus visible on la remet tout a droite
        if (backgrounds[i].x < -tex_w) {
            backgrounds[i].x += 4 * tex_w;
        }
    }
}

void deplacement_grounds(float dt, INCLUDES_FONCTIONS) {
    DUAL_Texture_GetSize(ground, &tex_w, &tex_h);
    for (int i = 0; i < 4; i++) {
        grounds[i].x -= VITESSE_GROUD * dt;
        // Si la texture n est plus visible on la remet tout a droite
        if (grounds[i].x < -tex_w) {
            grounds[i].x += 4 * tex_w;
        }
    }
}

void deplacement_pipes(float dt, INCLUDES_FONCTIONS) {
    DUAL_Texture_GetSize(pipe, &tex_w, &tex_h);
    for (int i = 0; i < 3; i++) {
        pipes[i].pos.x -= VITESSE_GROUD * dt;
        // Si la texture n est plus visible on la remet tout a droite
        if (pipes[i].pos.x < -tex_w) {
            pipes[i].pos.x += 3 * 300;
            pipes[i].pos.y = RandomPipeHeight();
        }
    }
}

void handleCollisions(float dt, INCLUDES_FONCTIONS) {
    DUAL_Texture_GetSize(bird_down, &tex_w, &tex_h);
    int w_ois = tex_w; int h_ois = tex_h;
    DUAL_Texture_GetSize(pipe, &tex_w, &tex_h);
    int w_pipe = tex_w; int h_pipe = tex_h;

    DUAL_Rect collider_oiseau = {200.0-(w_ois/2),bird_y-(h_ois/2), w_ois, h_ois};
    for (int i = 0; i < 3; i++) {
        tuyau_bas = (DUAL_Rect){pipes[i].pos.x - (w_pipe / 2), pipes[i].pos.y + ESPACEMENT_TUYAU, w_pipe, h_pipe};
        tuyau_haut = tuyau_bas;
        tuyau_haut.y = pipes[i].pos.y - ESPACEMENT_TUYAU - h_pipe;

        if (DUAL_CollideRectRect(collider_oiseau, tuyau_haut) || DUAL_CollideRectRect(collider_oiseau, tuyau_bas)) {
            gameOver = true;
            break;
        }
    }
}

void my_update(float dt, INCLUDES_FONCTIONS) {
    pressed = DUAL_IsTouching(inputManager);

    if (gameStarted && !gameOver) {
        deplacements_oiseau(dt, PARAMS_FONCTIONS);
        deplacement_backgrounds(dt, PARAMS_FONCTIONS);
        deplacement_grounds(dt, PARAMS_FONCTIONS);
        deplacement_pipes(dt, PARAMS_FONCTIONS);
        handleCollisions(dt, PARAMS_FONCTIONS);
    }
    else {
        DUAL_TouchState touch = DUAL_GetTouchState(inputManager);
        if (touch.phase == DUAL_TOUCH_STARTED) {
            gameStarted = true;
            deplacements_oiseau(dt, PARAMS_FONCTIONS);
        }
    }

    if (gameOver) {
        DUAL_TouchState touch = DUAL_GetTouchState(inputManager);
        if (touch.phase == DUAL_TOUCH_STARTED) {
            gameStarted = false;
            gameOver = false;
            rotation = 0.0;
            bird_y = 200.0;
            bird_velocity = 0.0f;
            pressed = false;
            init_world(PARAMS_FONCTIONS);
        }
    }
}

void affichage_oiseau(INCLUDES_FONCTIONS) {
    DUAL_Texture* bird = NULL;
    if (bird_velocity < 150) {
        bird = bird_down;
    }
    else {
        bird = bird_up;
    }
    DUAL_Texture_GetSize(bird_down, &tex_w, &tex_h);

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
}

void affichage_background(INCLUDES_FONCTIONS) {
    DUAL_Texture_GetSize(background_day, &tex_w, &tex_h);
    for (int i = 0; i < 4; i++) {
        DUAL_Vec2 pos = backgrounds[i];
        DUAL_SpriteParams params = {
            .texture = background_day,
            .position = pos,
            .origine = { 0.0, 0.0 },
            .echelle = { 1.0f, 1.0f },
            .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
            .rotation_radians = 0.0,
            .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
            .mode_melange = DUAL_BLEND_ALPHA
        };
        DUAL_DrawSprite(renderer2D, &params);
    }
}

void affichage_ground(INCLUDES_FONCTIONS) {
    DUAL_Texture_GetSize(ground, &tex_w, &tex_h);
    for (int i = 0; i < 4; i++) {
        DUAL_Vec2 pos = grounds[i];
        DUAL_SpriteParams params = {
            .texture = ground,
            .position = pos,
            .origine = { 0.0, 0.0 },
            .echelle = { 1.0f, 1.0f },
            .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
            .rotation_radians = 0.0,
            .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
            .mode_melange = DUAL_BLEND_ALPHA
        };
        DUAL_DrawSprite(renderer2D, &params);
    }
}

void affichage_menu(INCLUDES_FONCTIONS) {
    DUAL_Texture_GetSize(menu, &tex_w, &tex_h);
    DUAL_SpriteParams paramsHaut = {
        .texture = menu,
        .position = {400,240.0},
        .origine = { tex_w/2, tex_h/2.0f },
        .echelle = { 1.0f, 1.0f },
        .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
        .rotation_radians = 0.0,
        .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
        .mode_melange = DUAL_BLEND_ALPHA
    };
    DUAL_DrawSprite(renderer2D, &paramsHaut);
}

void affichage_tuyaux(INCLUDES_FONCTIONS) {
    DUAL_Texture_GetSize(pipe, &tex_w, &tex_h);

    for (int i = 0; i < 3; i++) {
        // tuyau bas
        DUAL_SpriteParams params = {
            .texture = pipe,
            .position = {pipes[i].pos.x, pipes[i].pos.y + pipes[i].espacement},
            .origine = { tex_w/2, 0.0 },
            .echelle = { 1.0f, 1.0f },
            .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
            .rotation_radians = 0.0,
            .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
            .mode_melange = DUAL_BLEND_ALPHA
        };
        DUAL_DrawSprite(renderer2D, &params);

        // tuyau haut
        DUAL_SpriteParams paramsHaut = {
            .texture = pipe,
            .position = {pipes[i].pos.x, pipes[i].pos.y - pipes[i].espacement},
            .origine = { tex_w/2, 0.0 },
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
    if (pressed) {
        DUAL_Texture_GetSize(bottom, &tex_w, &tex_h);
        DUAL_SpriteParams params = {
            .texture = bottom,
            .position = {400,240.0},
            .origine = { tex_w/2, tex_h/2.0f },
            .echelle = { 1.0f, 1.0f },
            .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
            .rotation_radians = 0.0,
            .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
            .mode_melange = DUAL_BLEND_ALPHA
        };
        DUAL_DrawSprite(renderer2D, &params);
    }
    else {
        DUAL_Texture_GetSize(bottom, &tex_w, &tex_h);
        DUAL_SpriteParams params = {
            .texture = bottom_buttom,
            .position = {400,240.0},
            .origine = { tex_w/2, tex_h/2.0f },
            .echelle = { 1.0f, 1.0f },
            .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
            .rotation_radians = 0.0,
            .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
            .mode_melange = DUAL_BLEND_ALPHA
        };
        DUAL_DrawSprite(renderer2D, &params);
    }
}

void affichage_gameover(INCLUDES_FONCTIONS) {
    DUAL_Texture_GetSize(gameoverTexture, &tex_w, &tex_h);
    DUAL_SpriteParams params = {
        .texture = gameoverTexture,
        .position = {400, 240},
        .origine = { tex_w/2.0, tex_h/2.0 },
        .echelle = { 1.0f, 1.0f },
        .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
        .rotation_radians = 0.0,
        .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
        .mode_melange = DUAL_BLEND_ALPHA
    };
    DUAL_DrawSprite(renderer2D, &params);
}

// 3. Affichage du jeu (Dessin)
void my_draw(INCLUDES_FONCTIONS) {
    // Sélection de l'écran virtuel ciblé (Top Screen)
    DUAL_SetActiveScreen(app, DUAL_SCREEN_TOP);

    // Un seul Begin suffit pour tout le rendu de cet écran
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
    printf("Fermeture du jeu, nettoyage des textures...\n");

    if (bird_down) DUAL_Texture_Destroy(resourceManager, bird_down);
    if (bird_up)   DUAL_Texture_Destroy(resourceManager, bird_up);
    if (font)      DUAL_Font_Destroy(resourceManager, font);

    bird_down = NULL;
    bird_up = NULL;
    font = NULL;
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