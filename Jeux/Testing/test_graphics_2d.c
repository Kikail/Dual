//
// Created by killian on 7/9/26.
//
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../SDK/include/DUAL_Core/dual_core.h"
#include "../../SDK/include/DUAL_Graphics/dual_graphics_2d.h"
#include "../../SDK/include/DUAL_Resources/dual_resources.h"
#include "dual_utils.h"

// Macro pour simuler notre image de chat de 128x128 en RGBA8
#define IMAGE_WIDTH  128
#define IMAGE_HEIGHT 128
#define IMAGE_CHANNELS 4
#define IMAGE_SIZE (IMAGE_WIDTH * IMAGE_HEIGHT * IMAGE_CHANNELS)

const char* vertex_shader_src_main =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "layout (location = 2) in vec4 aColor;\n"
    "out vec2 TexCoord;\n"
    "out vec4 VertexColor;\n"
    "uniform mat4 uProjection;\n"
    "uniform mat4 uView;\n"
    "void main() {\n"
    "   gl_Position = uProjection * uView * vec4(aPos, 0.0, 1.0);\n"
    "   TexCoord = aTexCoord;\n"
    "   VertexColor = aColor;\n"
    "}\n";

const char* fragment_shader_src_main =
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "in vec4 VertexColor;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTexture;\n"
    "uniform bool uUseTexture;\n"
    "void main() {\n"
    "   if (uUseTexture) {\n"
    "       FragColor = texture(uTexture, TexCoord) * VertexColor * vec4(1.0, 0.0, 0.0, 1.0);\n"
    "   } else {\n"
    "       // Mode sans texture. Un UV avec x < -1.5 est le code pour DUAL_DrawRect\n"
    "       if (TexCoord.x < -1.5) {\n"
    "           FragColor = VertexColor;\n"
    "           return;\n"
    "       }\n"
    "       // Mode procédural : DUAL_DrawCircleOutline (TexCoord de -1.0 à 1.0)\n"
    "       float distance = length(TexCoord);\n"
    "       \n"
    "       float epaisseur = 0.05; // Ajuste ici l'épaisseur du trait (en % du rayon)\n"
    "       float lissage = 0.01;   // Force de l'anti-aliasing\n"
    "       \n"
    "       // Anti-aliasing du bord extérieur et intérieur\n"
    "       float alpha_ext = 1.0 - smoothstep(1.0 - lissage, 1.0, distance);\n"
    "       float alpha_int = smoothstep(1.0 - epaisseur - lissage, 1.0 - epaisseur, distance);\n"
    "       \n"
    "       float final_alpha = alpha_ext * alpha_int;\n"
    "       \n"
    "       if (final_alpha <= 0.0) discard;\n"
    "       FragColor = vec4(VertexColor.rgb, VertexColor.a * final_alpha);\n"
    "   }\n"
    "}\n";

uint8_t* get_pixel_image(DUAL_ResourceManager* resourceManager) {
    uint8_t* cat_pixels = (uint8_t*)malloc(IMAGE_SIZE);
    if (!cat_pixels) {
        DUAL_Log(DUAL_LOG_ERROR, "Échec de l'allocation mémoire pour les pixels");
        return NULL;
    }

    DUAL_ResourceHandle* handle = NULL;
    DUAL_Result result = DUAL_ResourceManager_Track(resourceManager, DUAL_MEMORY_RAM, DUAL_RESOURCE_AUTRE, IMAGE_SIZE, "cat_pixels", &handle);
    DEBUG_DUAL_RESULT(result);

    // 2. Remplissage du buffer avec un motif (ici, un dégradé orange/rose "couleur chat")
    // Dans un vrai projet, ces données proviendraient d'un fichier .h généré ou d'un décodeur
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
        for (int x = 0; x < IMAGE_WIDTH; x++) {
            int index = (y * IMAGE_WIDTH + x) * IMAGE_CHANNELS;

            // On génère un motif de chat conceptuel (par exemple, un dégradé orange)
            cat_pixels[index + 0] = (uint8_t)(230);             // R (Orange de base)
            cat_pixels[index + 1] = (uint8_t)(120 + (y / 2));   // G
            cat_pixels[index + 2] = (uint8_t)(30 + (x / 2));    // B
            cat_pixels[index + 3] = 255;                        // A (Opaque)
        }
    }

    return cat_pixels;
}

int main() {
    DUAL_Log(DUAL_LOG_INFO, "Test Graphics 2D started !");

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

    // On creer le resource manager
    DUAL_ResourceManager* resourceManager = NULL;
    DUAL_ResourceManager_Create(app, &resourceManager);

    // On charge des ressources
    DUAL_Texture* ambulance = NULL;
    result = DUAL_Texture_LoadFromFile(resourceManager, "/home/killian/CLionProjects/Dual/Jeux/resources/2D/Cars/ambulance.png", DUAL_FILTER_NEAREST, &ambulance);
    DEBUG_DUAL_RESULT(result);
    DUAL_Texture* degrade = NULL;
    result = DUAL_Texture_LoadFromMemory(resourceManager, get_pixel_image(resourceManager), IMAGE_WIDTH, IMAGE_HEIGHT, DUAL_FILTER_NEAREST, &degrade);
    DEBUG_DUAL_RESULT(result);
    DUAL_Font* font = NULL;
    result = DUAL_Font_LoadFromFile(resourceManager, "/home/killian/CLionProjects/Dual/Jeux/resources/2D/Minecraft.ttf", 50, &font);
    DEBUG_DUAL_RESULT(result);

    // On recupere la hauteur et largeur d'une texture
    int32_t w = 0; int32_t h = 0;
    DUAL_Texture_GetSize(ambulance, &w, &h);
    DUAL_Log(DUAL_LOG_INFO, "w:%d h:%d", w, h);

    // On creer les parametres d'affichage de nos textures
    DUAL_SpriteParams ambulance_params = {
        .texture = ambulance,
        .echelle = {3.0, 3.0},
        .mode_melange = DUAL_BLEND_NONE,
        .origine = {0.0, h},
        .position = {20.0, 300.0},
        .profondeur = 0.0,
        .rect_source = {0.0, 0.0, w, h},
        .rotation_radians = 0.0,
        .teinte = DUAL_COLOR_WHITE
    };
    DUAL_Texture_GetSize(degrade, &w, &h);
    DUAL_Log(DUAL_LOG_INFO, "w:%d h:%d", w, h);
    DUAL_SpriteParams degrade_params = {
        .texture = degrade,
        .echelle = {1.0, 1.0},
        .mode_melange = DUAL_BLEND_NONE,
        .origine = {0.0, 0.0},
        .position = {400.0 - (w/2), 350},
        .profondeur = 0.0,
        .rect_source = {0.0, 0.0, w, h},
        .rotation_radians = 0.0,
        .teinte = DUAL_COLOR_WHITE
    };

    // On creer des formes a afficher
    DUAL_Rect rectangle = {500.0, 300.0, 128, 128};
    DUAL_Circle circle = {(DUAL_Vec2){200.0, 30.0}, 100.0};

    // On affiche les stats de notre resource manager
    DUAL_ResourceManager_Log(resourceManager);

    // On creer notre 2d renderer
    DUAL_Renderer2D* renderer2D = NULL;
    result = DUAL_Renderer2D_Create(app, &renderer2D);
    DEBUG_DUAL_RESULT(result);

    // Utilisation de shader perso
    GLuint shader;
    result = DUAL_Renderer2D_LoadShader(renderer2D, vertex_shader_src_main, fragment_shader_src_main, &shader);
    DEBUG_DUAL_RESULT(result);
    DUAL_Renderer2D_UseShader(renderer2D, shader);
    DUAL_Renderer2D_ResetShader(renderer2D);

    // Boucle du jeu principal
    while (DUAL_ShouldRun(app)) {
        DUAL_BeginFrame(app);

        // On deplace la camera
        DUAL_Renderer2D_SetCameraPosition(renderer2D, (DUAL_Vec2){0.0, 200 * sin(DUAL_GetTime(app))});

        // On selectionne l'ecran du bas
        DUAL_SetActiveScreen(app, DUAL_SCREEN_BOTTOM);

        // On dessine nos images
        DUAL_Renderer2D_Begin(renderer2D);
        DUAL_DrawSprite(renderer2D, &ambulance_params);
        DUAL_DrawSprite(renderer2D, &degrade_params);
        DUAL_DrawRect(renderer2D, rectangle, DUAL_COLOR_RED);
        DUAL_DrawCircleOutline(renderer2D, circle, DUAL_COLOR_RED, 32);
        DUAL_Renderer2D_End(renderer2D);

        // On selectionne l'ecran du haut
        DUAL_SetActiveScreen(app, DUAL_SCREEN_TOP);

        // On dessine nos images
        DUAL_Renderer2D_Begin(renderer2D);
        float tw = 0; float th = 0;
        DUAL_MeasureText(font, "TEST TEXT", &tw, &th);
        DUAL_DrawText(renderer2D, font, "TEST TEXT", (DUAL_Vec2){400-(tw/2), 240-(th/2)}, DUAL_COLOR_WHITE);
        DUAL_Renderer2D_End(renderer2D);

        DUAL_EndFrame(app);
    }

    // On ferme proprement l'application
    DUAL_ResourceManager_Destroy(resourceManager);
    DUAL_Renderer2D_Destroy(renderer2D);
    DUAL_Shutdown(app);

    return EXIT_SUCCESS;
}
