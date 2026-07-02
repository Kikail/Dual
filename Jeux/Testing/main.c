#include <stdio.h>
#include <stdlib.h>

/* Inclusion des modules de libdual */
#include <math.h>

#include "dual_core.h"
#include "dual_math.h"
#include "dual_graphics_2d.h"
#include "dual_graphics_3d.h"

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    /* ========================================================================
     * 1. Initialisation du moteur (Core)
     * ====================================================================== */
    DUAL_App* app = NULL;

    // Configuration de l'application selon ton dual_core.h
    DUAL_AppConfig config = {
        .titre_fenetre = "Test libdual - Double Écran",
        .largeur_ecran = 400 * 2, // Résolution standard type console portable
        .hauteur_ecran = 240 * 2,
        .plein_ecran = false,
        .vsync_actif = true,
        .fps_cible = 60
    };

    if (DUAL_Init(&config, &app) != DUAL_OK) {
        DUAL_Log(DUAL_LOG_ERROR, "Impossible d'initialiser libdual.");
        return -1;
    }

    /* ========================================================================
     * 2.1 Initialisation du rendu 2D
     * ====================================================================== */
    DUAL_Renderer2D* renderer = NULL;
    if (DUAL_Renderer2D_Create(app, &renderer) != DUAL_OK) {
        DUAL_Log(DUAL_LOG_ERROR, "Impossible d'initialiser le Renderer 2D.");
        DUAL_Shutdown(app);
        return -1;
    }

    /* ========================================================================
     * 2.2 Initialisation du rendu 3D
     * ====================================================================== */
    DUAL_Renderer3D* renderer3D = NULL;
    if (DUAL_Renderer3D_Create(app, &renderer3D) != DUAL_OK) {
        DUAL_Log(DUAL_LOG_ERROR, "Impossible d'initialiser le Renderer 3D.");
        DUAL_Shutdown(app);
        return -1;
    }

    /* ========================================================================
     * 3. Chargement des ressources (Assets)
     * ====================================================================== */
    DUAL_Texture* texture_logo = NULL;
    DUAL_Texture* image = NULL;
    DUAL_Font* police = NULL;
    DUAL_Material* material = NULL;
    DUAL_Model* model = NULL;
    DUAL_Transform3D transform = {
        {0.0,0.0,-5.0},
        {0.0,0.0,0.0},
        {1.0,1.0,1.0}
    };

    // Chargment des ressources
    DUAL_Texture_LoadFromFile(NULL, "/home/killian/CLionProjects/Dual/assets/logo.png", DUAL_FILTER_LINEAR, &texture_logo);
    DUAL_Texture_LoadFromFile(NULL, "/home/killian/CLionProjects/Dual/assets/image.png", DUAL_FILTER_LINEAR, &image);
    DUAL_Font_LoadFromFile(NULL, "/home/killian/CLionProjects/Dual/assets/arial.ttf", 20, &police);
    DUAL_Result r = DUAL_Material_Create(NULL, image, &material);
    if (r != DUAL_OK) {
        DUAL_Log(DUAL_LOG_ERROR, "Impossible de charger le material.");
        DUAL_Shutdown(app);
        return -1;
    }
    r = DUAL_Model_LoadFromOBJ(NULL, "/home/killian/CLionProjects/Dual/assets/model.obj", &model);
    if (r != DUAL_OK) {
        DUAL_Log(DUAL_LOG_ERROR, "Impossible de charger le model.");
        DUAL_Shutdown(app);
        return -1;
    }

    int tex_w = 0, tex_h = 0;
    if (texture_logo) {
        DUAL_Texture_GetSize(texture_logo, &tex_w, &tex_h);
    }

    /* ========================================================================
     * 4. Variables d'état du jeu
     * ====================================================================== */
    DUAL_Vec2 position = { 200.0f, 120.0f };
    DUAL_Vec2 velocite = { 150.0f, 100.0f }; // Pixels par seconde
    float rotation = 0.0f;

    /* ========================================================================
     * 5. Boucle principale
     * ====================================================================== */
    while (DUAL_ShouldRun(app)) {

        // --- LOGIQUE (Mise à jour) ---
        float dt = (float)DUAL_GetDeltaTime(app);

        // Mise à jour de la position : pos = pos + (vel * dt)
        DUAL_Vec2 deplacement = DUAL_Vec2_Scale(velocite, dt);
        position = DUAL_Vec2_Add(position, deplacement);

        // Rotation (2 radians par seconde)
        rotation += 2.0f * dt;

        // Rebonds sur les bords de l'écran du haut (400x240)
        if (position.x < 0.0f || position.x > (float)config.largeur_ecran) velocite.x *= -1.0f;
        if (position.y < 0.0f || position.y > (float)config.hauteur_ecran) velocite.y *= -1.0f;

        // --- RENDU ---
        DUAL_BeginFrame(app);


        // --- RENDU ÉCRAN SUPÉRIEUR ---
        DUAL_SetActiveScreen(app, DUAL_SCREEN_TOP);

        // --- RENDU 2D ---
        DUAL_Renderer2D_Begin(renderer);
        if (texture_logo) {
            DUAL_SpriteParams params = {0};
            params.texture = texture_logo;
            params.position = position;
            params.origine = (DUAL_Vec2){ tex_w / 2.0f, tex_h / 2.0f };
            params.echelle = (DUAL_Vec2){ 1.0f, 1.0f };
            params.rect_source = (DUAL_Rect){ 0.0f, 0.0f, (float)tex_w, (float)tex_h };
            params.rotation_radians = rotation;
            params.teinte = (DUAL_Color){ 1.0f, 1.0f, 1.0f, 1.0f };
            params.mode_melange = DUAL_BLEND_ALPHA;
            params.profondeur = 0;

            DUAL_DrawSprite(renderer, &params);
        }
        DUAL_Renderer2D_End(renderer);

        // --- RENDU ÉCRAN INFÉRIEUR ---
        DUAL_SetActiveScreen(app, DUAL_SCREEN_BOTTOM);
        DUAL_Renderer2D_Begin(renderer);

        if (police) {
            DUAL_Color couleur_texte = { 1.0f, 0.8f, 0.0f, 1.0f };

            char fps_texte[64];
            snprintf(fps_texte, sizeof(fps_texte), "FPS: %d", DUAL_GetFPS(app));

            DUAL_DrawText(renderer, police, "libdual 2D fonctionne !", (DUAL_Vec2){ 10.0f, 20.0f }, couleur_texte);
            DUAL_DrawText(renderer, police, fps_texte, (DUAL_Vec2){ 10.0f, 50.0f }, (DUAL_Color){1.0f, 1.0f, 1.0f, 1.0f});
        }
        DUAL_Renderer2D_End(renderer);

        // --- RENDU 3D ---
        DUAL_Renderer3D_Begin(renderer3D);

        transform.rotation_euler_radians.x += DUAL_GetDeltaTime(app);
        transform.rotation_euler_radians.y += DUAL_GetDeltaTime(app) * 2;
        transform.position.z = -sin(DUAL_GetTime(app)) * 2.5 - 7.5;
        DUAL_DrawModel(renderer3D, model, material, transform);

        DUAL_Renderer3D_End(renderer3D);

        DUAL_EndFrame(app);
    }

    /* ========================================================================
     * 6. Nettoyage
     * ====================================================================== */
    DUAL_Log(DUAL_LOG_INFO, "Fermeture de l'application...");

    if (police) DUAL_Font_Destroy(NULL, police);
    if (texture_logo) DUAL_Texture_Destroy(NULL, texture_logo);

    DUAL_Renderer2D_Destroy(renderer);
    DUAL_Renderer3D_Destroy(renderer3D);
    DUAL_Shutdown(app);

    return 0;
}
