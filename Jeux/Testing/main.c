#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "dual_audio.h"
#include "dual_core.h"
#include "dual_math.h"
#include "dual_graphics_2d.h"
#include "dual_graphics_3d.h"
#include "dual_resources.h" // <-- AJOUT

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    DUAL_App* app = NULL;
    DUAL_AppConfig config = {
        .titre_fenetre = "Test libdual - Double Ecran",
        .largeur_ecran = 400 * 2,
        .hauteur_ecran = 240 * 2,
        .plein_ecran = false,
        .vsync_actif = true,
        .fps_cible = 60
    };

    if (DUAL_Init(&config, &app) != DUAL_OK) {
        return -1;
    }

    /* ========================================================================
     * Initialisation du Gestionnaire de Ressources (Resource Manager)
     * ====================================================================== */
    DUAL_ResourceManager* resources = NULL;
    if (DUAL_ResourceManager_Create(app, &resources) != DUAL_OK) {
        DUAL_Log(DUAL_LOG_ERROR, "Impossible d'initialiser le Resource Manager.");
        DUAL_Shutdown(app);
        return -1;
    }

    // Ajustement optionnel des budgets mémoire (ex: Limiter à 32Mo VRAM pour tester le débordement)
    DUAL_ResourceManager_SetVRAMBudget(resources, 32 * 1024 * 1024);

    DUAL_Renderer2D* renderer = NULL;
    DUAL_Renderer2D_Create(app, &renderer);

    DUAL_Renderer3D* renderer3D = NULL;
    DUAL_Renderer3D_Create(app, &renderer3D);

    /* ========================================================================
     * Chargement des ressources en passant l'instance "resources"
     * ====================================================================== */
    DUAL_Texture* texture_logo = NULL;
    DUAL_Texture* image = NULL;
    DUAL_Font* police = NULL;
    DUAL_Material* material = NULL;
    DUAL_Model* model = NULL;
    DUAL_Transform3D transform = {{0.0,0.0,-5.0}, {0.0,0.0,0.0}, {1.0,1.0,1.0}};

    // Remplacement des anciens paramètres NULL par notre gestionnaire "resources"
    DUAL_Texture_LoadFromFile(resources, "/home/killian/CLionProjects/Dual/assets/logo.png", DUAL_FILTER_LINEAR, &texture_logo);
    DUAL_Texture_LoadFromFile(resources, "/home/killian/CLionProjects/Dual/assets/image.png", DUAL_FILTER_LINEAR, &image);
    DUAL_Font_LoadFromFile(resources, "/home/killian/CLionProjects/Dual/assets/arial.ttf", 20, &police);
    DUAL_Material_Create(resources, image, &material);
    DUAL_Model_LoadFromOBJ(resources, "/home/killian/CLionProjects/Dual/assets/model.obj", &model);

    // 1. Initialisation du moteur audio
    DUAL_AudioManager* audio = NULL;
    if (DUAL_AudioManager_Create(app, &audio) != 0) {
        printf("Erreur: Impossible d'initialiser l'audio.\n");
        return -1;
    }

    // 2. Chargement des ressources
    DUAL_Music* music_level1 = NULL;
    DUAL_Sound* sfx_jump = NULL;

    // Musique (streaming MP3/OGG)
    DUAL_Music_OpenFromFile(audio, resources, "/home/killian/CLionProjects/Dual/assets/music.mp3", &music_level1);

    // 3. Réglage des volumes globaux
    DUAL_AudioManager_SetChannelVolume(audio, DUAL_CHANNEL_MASTER, 1.0f); // 100%
    DUAL_AudioManager_SetChannelVolume(audio, DUAL_CHANNEL_MUSIC, 0.5f);  // Musique à 50%
    DUAL_AudioManager_SetChannelVolume(audio, DUAL_CHANNEL_SFX, 1.0f);    // Bruitages à 100%

    // 4. Lancement de la musique en boucle
    if (music_level1) {
        DUAL_Music_Play(audio, music_level1, true); // true = boucle
    }

    int tex_w = 0, tex_h = 0;
    if (texture_logo) DUAL_Texture_GetSize(texture_logo, &tex_w, &tex_h);

    DUAL_Vec2 position = { 200.0f, 120.0f };
    DUAL_Vec2 velocite = { 150.0f, 100.0f };
    float rotation = 0.0f;

    /* ========================================================================
     * Boucle principale
     * ====================================================================== */
    while (DUAL_ShouldRun(app)) {
        float dt = (float)DUAL_GetDeltaTime(app);
        position = DUAL_Vec2_Add(position, DUAL_Vec2_Scale(velocite, dt));
        rotation += 2.0f * dt;

        if (position.x < 0.0f || position.x > (float)config.largeur_ecran) velocite.x *= -1.0f;
        if (position.y < 0.0f || position.y > (float)config.hauteur_ecran) velocite.y *= -1.0f;

        DUAL_BeginFrame(app);

        // Ecran du haut
        DUAL_SetActiveScreen(app, DUAL_SCREEN_TOP);
        DUAL_Renderer2D_Begin(renderer);
        if (texture_logo) {
            DUAL_SpriteParams params = {
                .texture = texture_logo, .position = position,
                .origine = { tex_w / 2.0f, tex_h / 2.0f }, .echelle = { 1.0f, 1.0f },
                .rect_source = { 0.0f, 0.0f, (float)tex_w, (float)tex_h },
                .rotation_radians = rotation, .teinte = { 1.0f, 1.0f, 1.0f, 1.0f },
                .mode_melange = DUAL_BLEND_ALPHA
            };
            DUAL_DrawSprite(renderer, &params);
        }
        DUAL_Renderer2D_End(renderer);

        // Ecran du bas
        DUAL_SetActiveScreen(app, DUAL_SCREEN_BOTTOM);
        DUAL_Renderer2D_Begin(renderer);
        if (police) {
            char fps_texte[64];
            snprintf(fps_texte, sizeof(fps_texte), "FPS: %d", DUAL_GetFPS(app));
            DUAL_DrawText(renderer, police, "libdual 2D avec Resource Manager !", (DUAL_Vec2){ 10.0f, 20.0f }, (DUAL_Color){ 1.0f, 0.8f, 0.0f, 1.0f });
            DUAL_DrawText(renderer, police, fps_texte, (DUAL_Vec2){ 10.0f, 50.0f }, (DUAL_Color){1.0f, 1.0f, 1.0f, 1.0f});

            // Affichage des stats mémoire à l'écran pour vérification
            DUAL_MemoryStats stats;
            DUAL_ResourceManager_GetStats(resources, &stats);
            char stats_texte[128];
            snprintf(stats_texte, sizeof(stats_texte), "RAM: %ld/%ld Octets (Assets: %d)", stats.ram_utilisee_octets, stats.ram_totale_octets, stats.nombre_ressources_actives);
            DUAL_DrawText(renderer, police, stats_texte, (DUAL_Vec2){ 10.0f, 90.0f }, (DUAL_Color){0.4f, 1.0f, 0.4f, 1.0f});
        }
        DUAL_Renderer2D_End(renderer);

        // Rendu 3D
        DUAL_Renderer3D_Begin(renderer3D);
        transform.rotation_euler_radians.x += dt;
        transform.rotation_euler_radians.y += dt * 2;
        transform.position.z = -sin(DUAL_GetTime(app)) * 2.5 - 7.5;
        if (model && material) {
            DUAL_DrawModel(renderer3D, model, material, transform);
        }
        DUAL_Renderer3D_End(renderer3D);

        DUAL_EndFrame(app);
    }

    /* ========================================================================
     * Nettoyage propre
     * ====================================================================== */
    DUAL_Log(DUAL_LOG_INFO, "Fermeture de l'application...");

    // Destruction explicite en transmettant le manager
    if (police) DUAL_Font_Destroy(resources, police);
    if (texture_logo) DUAL_Texture_Destroy(resources, texture_logo);
    if (material) DUAL_Material_Destroy(resources, material);
    if (model) DUAL_Model_Destroy(resources, model);

    // Note : Si vous oubliez des ressources, DUAL_ResourceManager_Destroy se chargera
    // de purger tout ce qui traîne automatiquement pour éviter les fuites de mémoire.
    DUAL_ResourceManager_Destroy(resources);

    DUAL_Renderer2D_Destroy(renderer);
    DUAL_Renderer3D_Destroy(renderer3D);
    DUAL_Shutdown(app);

    return 0;
}