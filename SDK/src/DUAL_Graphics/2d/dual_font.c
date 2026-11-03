//
// Created by killian on 11/3/26.
//
#include "../../SDK/include/DUAL_Graphics/2d/dual_font.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DUAL_Graphics/2d/dual_texture.h"
#include "DUAL_Resources/dual_resources.h"

DUAL_Result DUAL_Font_LoadFromFile(DUAL_ResourceManager* resources, const char* chemin_fichier, int32_t taille_pixels, DUAL_Font** out_font) {
    if (!chemin_fichier || !out_font) return DUAL_ERROR_INVALID_ARG;

    FILE* f = fopen(chemin_fichier, "rb");
    if (!f) return DUAL_ERROR_NOT_FOUND;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* ttf_buffer = (uint8_t*)malloc((size_t)size);
    if (!ttf_buffer) {
        fclose(f);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }
    size_t read_bytes = fread(ttf_buffer, 1, (size_t)size, f);
    fclose(f);
    if (read_bytes < (size_t)size) {
        free(ttf_buffer);
        return DUAL_ERROR_INIT_FAILED;
    }

    uint64_t taille_ram = sizeof(DUAL_Font);

    DUAL_Font* font = (DUAL_Font*)malloc(sizeof(DUAL_Font));
    font->taille_pixels = (float)taille_pixels;
    font->texture_atlas = NULL;


    int atlas_w = 1024, atlas_h = 1024;
    uint8_t* bitmap_mono = (uint8_t*)calloc((size_t)atlas_w * atlas_h, 1);
    if (!bitmap_mono) {
        free(ttf_buffer);
        free(font);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }
    stbtt_BakeFontBitmap(ttf_buffer, 0, font->taille_pixels, bitmap_mono, atlas_w, atlas_h, 32, 96, font->données_caractères);
    free(ttf_buffer);

    uint8_t* bitmap_rgba = (uint8_t*)malloc((size_t)atlas_w * atlas_h * 4);
    if (!bitmap_rgba) {
        free(bitmap_mono);
        free(font);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }
    for (int idx = 0; idx < atlas_w * atlas_h; idx++) {
        bitmap_rgba[idx*4 + 0] = 255;
        bitmap_rgba[idx*4 + 1] = 255;
        bitmap_rgba[idx*4 + 2] = 255;
        bitmap_rgba[idx*4 + 3] = bitmap_mono[idx];
    }

    DUAL_Result res_tex = DUAL_Texture_LoadFromMemory(resources, bitmap_rgba, atlas_w, atlas_h, DUAL_FILTER_LINEAR, &font->texture_atlas);

    free(bitmap_mono);
    free(bitmap_rgba);

    if (res_tex != DUAL_OK) {
        free(font);
        return res_tex;
    }

    strcpy(font->font_path, chemin_fichier);
    DUAL_GetFilenameWithoutExtension(chemin_fichier, font->font_name, 32);
    DUAL_ResourceManager_AddResource(resources, font, DUAL_RESOURCE_FONT);

    *out_font = font;

    return DUAL_OK;
}

void DUAL_Font_Destroy(void* f) {
    DUAL_Font* font = (DUAL_Font*)f;
    if (font) {
        free(font);
    }
}

void DUAL_DrawText(DUAL_Renderer2D* renderer, const DUAL_Font* font, const char* texte, DUAL_Vec2 position, DUAL_Color couleur) {
    if (!renderer || !font || !texte) return;

    float x = position.x;
    float y = position.y + font->taille_pixels;

    while (*texte) {
        char c = *texte;
        if (c >= 32 && c < 128) {
            stbtt_bakedchar* bc = &font->données_caractères[c - 32];

            if (bc->x1 > bc->x0 && bc->y1 > bc->y0) {
                DUAL_SpriteParams p = {
                    .texture = font->texture_atlas,
                    .position = { x + bc->xoff, y + bc->yoff },
                    .origine = {0,0},
                    .echelle = {1.0f, 1.0f},
                    .rect_source = { (float)bc->x0, (float)bc->y0, (float)(bc->x1 - bc->x0), (float)(bc->y1 - bc->y0) },
                    .rotation_radians = 0.0f,
                    .teinte = couleur,
                    .mode_melange = DUAL_BLEND_ALPHA,
                    .profondeur = 0
                };

                DUAL_DrawSprite(renderer, &p);
            }
            x += bc->xadvance;
        }
        texte++;
    }
}

void DUAL_MeasureText(const DUAL_Font* font, const char* texte, float* out_largeur, float* out_hauteur) {
    if (!font || !texte) return;
    float x = 0.0f;
    while (*texte) {
        char c = *texte;
        if (c >= 32 && c < 128) {
            x += font->données_caractères[c - 32].xadvance;
        }
        texte++;
    }
    if (out_largeur) *out_largeur = x;
    if (out_hauteur) *out_hauteur = font->taille_pixels;
}