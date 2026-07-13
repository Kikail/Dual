#include <stddef.h>
#include <glad/glad.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Inclusion de nos modules */
#include "dual_graphics_2d.h"
#include "dual_math.h"

/* Configuration et inclusion de STB Image */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCAL
#include "stb_image.h"

/* Configuration et inclusion de STB Truetype */
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

/* Déclarations externes pour interroger la configuration de l'écran */
extern void DUAL_Internal_GetScreenDimensions(const DUAL_App* app, int32_t* out_w, int32_t* out_h);

/* ============================================================================
 * Définition des structures opaques
 * ========================================================================== */

struct DUAL_Texture {
    GLuint id_opengl;
    int32_t largeur;
    int32_t hauteur;
    DUAL_ResourceHandle* handle;
};

struct DUAL_Font {
    DUAL_Texture* texture_atlas;
    float taille_pixels;
    stbtt_bakedchar données_caractères[96]; // Stocke les données ASCII 32 à 126
    DUAL_ResourceHandle* handle;
};

typedef struct {
    DUAL_Vec2 position;
    DUAL_Vec2 tex_coords;
    DUAL_Color couleur;
} Vertex2D;

struct DUAL_Renderer2D {
    DUAL_App* app;
    GLuint vao, vbo;
    GLuint shader_program;

    // Caméra
    DUAL_Vec2 camera_pos;
    float camera_zoom;
    float camera_rotation;

    // Système de Batching
    Vertex2D* vertex_buffer;
    uint32_t vertex_count;
    uint32_t max_vertices;
    DUAL_Texture* texture_courante; // Texture active pour le batch actuel
};

/* ============================================================================
 * Shaders 2D intégrés
 * ========================================================================== */

const char* vertex_shader_src =
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

const char* fragment_shader_src =
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "in vec4 VertexColor;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTexture;\n"
    "uniform bool uUseTexture;\n"
    "void main() {\n"
    "   if (uUseTexture) {\n"
    "       FragColor = texture(uTexture, TexCoord) * VertexColor;\n"
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

static GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("[ERREUR SHADER] %s\n", infoLog);
    }
    return shader;
}

/* ============================================================================
 * GESTION DES TEXTURES (stb_image)
 * ========================================================================== */

DUAL_Result DUAL_Texture_LoadFromFile(DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_TextureFilter filtre, DUAL_Texture** out_texture) {
    int largeur, hauteur, canaux;

    stbi_set_flip_vertically_on_load(false);
    uint8_t* pixels = stbi_load(chemin_fichier, &largeur, &hauteur, &canaux, STBI_rgb_alpha);

    if (!pixels) {
        printf("[ERREUR TEXTURE] Impossible de charger : %s\n", chemin_fichier);
        return DUAL_ERROR_NOT_FOUND;
    }

    DUAL_Result resultat = DUAL_Texture_LoadFromMemory(resources, pixels, largeur, hauteur, filtre, out_texture);
    stbi_image_free(pixels);
    return resultat;
}

DUAL_Result DUAL_Texture_LoadFromMemory(DUAL_ResourceManager* resources, const uint8_t* pixels, int32_t largeur, int32_t hauteur, DUAL_TextureFilter filtre, DUAL_Texture** out_texture) {
    if (!pixels || !out_texture) return DUAL_ERROR_INVALID_ARG;

    uint64_t taille_vram = (uint64_t)largeur * hauteur * 4;
    DUAL_ResourceHandle* res_handle = NULL;

    if (resources) {
        DUAL_Result res = DUAL_ResourceManager_Track(resources, DUAL_MEMORY_VRAM, DUAL_RESOURCE_TEXTURE, taille_vram, "Texture_2D", &res_handle);
        if (res != DUAL_OK) return res;
    }

    DUAL_Texture* tex = (DUAL_Texture*)malloc(sizeof(struct DUAL_Texture));
    if (!tex) {
        if (resources && res_handle) DUAL_ResourceManager_Untrack(resources, res_handle);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }

    tex->largeur = largeur;
    tex->hauteur = hauteur;
    tex->handle = res_handle;

    if (resources && res_handle) {
        extern void DUAL_Internal_ResourceHandle_SetCallback(DUAL_ResourceHandle* handle, void* ptr, void (*cb)(DUAL_ResourceManager*, void*));
        DUAL_Internal_ResourceHandle_SetCallback(res_handle, tex, (void(*)(DUAL_ResourceManager*, void*))DUAL_Texture_Destroy);
    }

    glGenTextures(1, &tex->id_opengl);
    glBindTexture(GL_TEXTURE_2D, tex->id_opengl);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, largeur, hauteur, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    GLint gl_filtre = (filtre == DUAL_FILTER_NEAREST) ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filtre);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filtre);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    *out_texture = tex;
    return DUAL_OK;
}

void DUAL_Texture_Destroy(DUAL_ResourceManager* resources, DUAL_Texture* texture) {
    if (texture) {
        glDeleteTextures(1, &texture->id_opengl);
        if (resources && texture->handle) {
            DUAL_ResourceManager_Untrack(resources, texture->handle);
        }
        free(texture);
    }
}

void DUAL_Texture_GetSize(const DUAL_Texture* texture, int32_t* out_largeur, int32_t* out_hauteur) {
    if (texture) {
        if (out_largeur) *out_largeur = texture->largeur;
        if (out_hauteur) *out_hauteur = texture->hauteur;
    }
}

GLuint DUAL_Internal_GetTextureID(const DUAL_Texture* texture) {
    return texture ? texture->id_opengl : 0;
}

/* ============================================================================
 * RENDU GENERAL ET BATCHING
 * ========================================================================== */

DUAL_Result DUAL_Renderer2D_Create(DUAL_App* app, DUAL_Renderer2D** out_renderer) {
    if (!app || !out_renderer) return DUAL_ERROR_INVALID_ARG;

    DUAL_Renderer2D* renderer = (DUAL_Renderer2D*)malloc(sizeof(struct DUAL_Renderer2D));
    if (!renderer) return DUAL_ERROR_OUT_OF_MEMORY;

    renderer->app = app;
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragment_shader_src);
    renderer->shader_program = glCreateProgram();
    glAttachShader(renderer->shader_program, vs);
    glAttachShader(renderer->shader_program, fs);
    glLinkProgram(renderer->shader_program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    renderer->max_vertices = 2000 * 4;
    renderer->vertex_count = 0;
    renderer->vertex_buffer = (Vertex2D*)malloc(sizeof(Vertex2D) * renderer->max_vertices);
    renderer->texture_courante = NULL;

    glGenVertexArrays(1, &renderer->vao);
    glGenBuffers(1, &renderer->vbo);
    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex2D) * renderer->max_vertices, NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, tex_coords));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, couleur));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    renderer->camera_pos = (DUAL_Vec2){0.0f, 0.0f};
    renderer->camera_zoom = 1.0f;
    renderer->camera_rotation = 0.0f;

    *out_renderer = renderer;
    return DUAL_OK;
}

void DUAL_Renderer2D_Destroy(DUAL_Renderer2D* renderer) {
    if (renderer) {
        glDeleteVertexArrays(1, &renderer->vao);
        glDeleteBuffers(1, &renderer->vbo);
        glDeleteProgram(renderer->shader_program);
        free(renderer->vertex_buffer);
        free(renderer);
    }
}

static void FlushBatch(DUAL_Renderer2D* renderer) {
    if (renderer->vertex_count == 0) return;

    glUseProgram(renderer->shader_program);

    int32_t w, h;
    DUAL_Internal_GetScreenDimensions(renderer->app, &w, &h);
    DUAL_Mat4 projection = DUAL_Mat4_Ortho(0.0f, (float)w, (float)h, 0.0f, -1.0f, 1.0f);

    DUAL_Mat4 view = DUAL_Mat4_Identity();
    view = DUAL_Mat4_Multiply(view, DUAL_Mat4_Translate((DUAL_Vec3){-renderer->camera_pos.x, -renderer->camera_pos.y, 0.0f}));

    glUniformMatrix4fv(glGetUniformLocation(renderer->shader_program, "uProjection"), 1, GL_FALSE, projection.m);
    glUniformMatrix4fv(glGetUniformLocation(renderer->shader_program, "uView"), 1, GL_FALSE, view.m);

    // Configuration de l'uniform booléen uUseTexture
    GLint use_tex_loc = glGetUniformLocation(renderer->shader_program, "uUseTexture");
    if (renderer->texture_courante) {
        glUniform1i(use_tex_loc, 1);
        glBindTexture(GL_TEXTURE_2D, renderer->texture_courante->id_opengl);
    } else {
        glUniform1i(use_tex_loc, 0);
    }

    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex2D) * renderer->vertex_count, renderer->vertex_buffer);

    glDrawArrays(GL_TRIANGLES, 0, renderer->vertex_count);

    glBindVertexArray(0);
    renderer->vertex_count = 0;
}

void DUAL_Renderer2D_Begin(DUAL_Renderer2D* renderer) {
    if (renderer) {
        renderer->vertex_count = 0;
        renderer->texture_courante = NULL;
        glDisable(GL_CULL_FACE);
    }
}

void DUAL_Renderer2D_End(DUAL_Renderer2D* renderer) {
    if (renderer) {
        FlushBatch(renderer);
    }
}

void DUAL_DrawSprite(DUAL_Renderer2D* renderer, const DUAL_SpriteParams* params) {
    if (!renderer || !params || !params->texture) return;

    if (renderer->texture_courante != params->texture) {
        FlushBatch(renderer);
        renderer->texture_courante = params->texture;
    }

    if (params->mode_melange == DUAL_BLEND_ALPHA) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else if (params->mode_melange == DUAL_BLEND_ADDITIVE) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    } else {
        glDisable(GL_BLEND);
    }

    if (renderer->vertex_count + 6 >= renderer->max_vertices) {
        FlushBatch(renderer);
    }

    float w = (params->rect_source.largeur > 0) ? params->rect_source.largeur : (float)params->texture->largeur;
    float h = (params->rect_source.hauteur > 0) ? params->rect_source.hauteur : (float)params->texture->hauteur;

    w *= params->echelle.x;
    h *= params->echelle.y;

    DUAL_Vec2 p0 = { -params->origine.x, -params->origine.y };
    DUAL_Vec2 p1 = { w - params->origine.x, -params->origine.y };
    DUAL_Vec2 p2 = { w - params->origine.x, h - params->origine.y };
    DUAL_Vec2 p3 = { -params->origine.x, h - params->origine.y };

    if (params->rotation_radians != 0.0f) {
        float cos_r = cosf(params->rotation_radians);
        float sin_r = sinf(params->rotation_radians);
        #define ROTATE(p) (DUAL_Vec2){ p.x * cos_r - p.y * sin_r, p.x * sin_r + p.y * cos_r }
        p0 = ROTATE(p0); p1 = ROTATE(p1); p2 = ROTATE(p2); p3 = ROTATE(p3);
    }

    p0.x += params->position.x; p0.y += params->position.y;
    p1.x += params->position.x; p1.y += params->position.y;
    p2.x += params->position.x; p2.y += params->position.y;
    p3.x += params->position.x; p3.y += params->position.y;

    float u0 = params->rect_source.x / params->texture->largeur;
    float v0 = params->rect_source.y / params->texture->hauteur;
    float u1 = (params->rect_source.x + ((params->rect_source.largeur > 0) ? params->rect_source.largeur : params->texture->largeur)) / params->texture->largeur;
    float v1 = (params->rect_source.y + ((params->rect_source.hauteur > 0) ? params->rect_source.hauteur : params->texture->hauteur)) / params->texture->hauteur;

    uint32_t i = renderer->vertex_count;
    renderer->vertex_buffer[i++] = (Vertex2D){ p0, {u0, v0}, params->teinte };
    renderer->vertex_buffer[i++] = (Vertex2D){ p1, {u1, v0}, params->teinte };
    renderer->vertex_buffer[i++] = (Vertex2D){ p2, {u1, v1}, params->teinte };

    renderer->vertex_buffer[i++] = (Vertex2D){ p0, {u0, v0}, params->teinte };
    renderer->vertex_buffer[i++] = (Vertex2D){ p2, {u1, v1}, params->teinte };
    renderer->vertex_buffer[i++] = (Vertex2D){ p3, {u0, v1}, params->teinte };

    renderer->vertex_count = i;
}

/* ============================================================================
 * GESTION DES POLICES ET DU TEXTE (stb_truetype)
 * ========================================================================== */

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

    uint64_t taille_ram = sizeof(struct DUAL_Font);
    DUAL_ResourceHandle* font_handle = NULL;
    if (resources) {
        DUAL_Result res = DUAL_ResourceManager_Track(resources, DUAL_MEMORY_RAM, DUAL_RESOURCE_AUTRE, taille_ram, chemin_fichier, &font_handle);
        if (res != DUAL_OK) {
            free(ttf_buffer);
            return res;
        }
    }

    DUAL_Font* font = (DUAL_Font*)malloc(sizeof(struct DUAL_Font));
    if (!font) {
        free(ttf_buffer);
        if (resources && font_handle) DUAL_ResourceManager_Untrack(resources, font_handle);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }
    font->taille_pixels = (float)taille_pixels;
    font->handle = font_handle;
    font->texture_atlas = NULL;

    if (resources && font_handle) {
        extern void DUAL_Internal_ResourceHandle_SetCallback(DUAL_ResourceHandle* handle, void* ptr, void (*cb)(DUAL_ResourceManager*, void*));
        DUAL_Internal_ResourceHandle_SetCallback(font_handle, font, (void(*)(DUAL_ResourceManager*, void*))DUAL_Font_Destroy);
    }

    int atlas_w = 1024, atlas_h = 1024;
    uint8_t* bitmap_mono = (uint8_t*)calloc((size_t)atlas_w * atlas_h, 1);
    if (!bitmap_mono) {
        free(ttf_buffer);
        if (resources && font_handle) DUAL_ResourceManager_Untrack(resources, font_handle);
        free(font);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }
    stbtt_BakeFontBitmap(ttf_buffer, 0, font->taille_pixels, bitmap_mono, atlas_w, atlas_h, 32, 96, font->données_caractères);
    free(ttf_buffer);

    uint8_t* bitmap_rgba = (uint8_t*)malloc((size_t)atlas_w * atlas_h * 4);
    if (!bitmap_rgba) {
        free(bitmap_mono);
        if (resources && font_handle) DUAL_ResourceManager_Untrack(resources, font_handle);
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
        if (resources && font_handle) DUAL_ResourceManager_Untrack(resources, font_handle);
        free(font);
        return res_tex;
    }

    *out_font = font;
    return DUAL_OK;
}

void DUAL_Font_Destroy(DUAL_ResourceManager* resources, DUAL_Font* font) {
    if (font) {
        if (resources && font->handle) {
            DUAL_ResourceManager_Untrack(resources, font->handle);
        }
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

/* ============================================================================
 * PRÉMICES DE DÉBOGAGE (Rectangles & cercles vides)
 * ========================================================================== */

void DUAL_DrawRect(DUAL_Renderer2D* renderer, DUAL_Rect rect, DUAL_Color couleur) {
    if (!renderer) return;

    if (renderer->texture_courante != NULL) {
        FlushBatch(renderer);
        renderer->texture_courante = NULL;
    }

    if (renderer->vertex_count + 6 >= renderer->max_vertices) {
        FlushBatch(renderer);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float x = rect.x;
    float y = rect.y;
    float w = rect.largeur;
    float h = rect.hauteur;

    DUAL_Vec2 p0 = { x,     y };
    DUAL_Vec2 p1 = { x + w, y };
    DUAL_Vec2 p2 = { x + w, y + h };
    DUAL_Vec2 p3 = { x,     y + h };

    // Coordonnées secrètes pour forcer le shader en mode "Rectangle plein"
    DUAL_Vec2 uv_neutre = { -2.0f, -2.0f };
    uint32_t idx = renderer->vertex_count;

    renderer->vertex_buffer[idx++] = (Vertex2D){ p0, uv_neutre, couleur };
    renderer->vertex_buffer[idx++] = (Vertex2D){ p1, uv_neutre, couleur };
    renderer->vertex_buffer[idx++] = (Vertex2D){ p2, uv_neutre, couleur };

    renderer->vertex_buffer[idx++] = (Vertex2D){ p0, uv_neutre, couleur };
    renderer->vertex_buffer[idx++] = (Vertex2D){ p2, uv_neutre, couleur };
    renderer->vertex_buffer[idx++] = (Vertex2D){ p3, uv_neutre, couleur };

    renderer->vertex_count = idx;
}

void DUAL_DrawCircleOutline(DUAL_Renderer2D* renderer, DUAL_Circle cercle, DUAL_Color couleur, int32_t segments) {
    if (!renderer) return;
    (void)segments; // Inutile maintenant, le GPU gère la perfection mathématique

    if (renderer->texture_courante != NULL) {
        FlushBatch(renderer);
        renderer->texture_courante = NULL;
    }

    if (renderer->vertex_count + 6 >= renderer->max_vertices) {
        FlushBatch(renderer);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Calcul de la boîte englobante (carré)
    float x0 = cercle.centre.x - cercle.rayon;
    float y0 = cercle.centre.y - cercle.rayon;
    float x1 = cercle.centre.x + cercle.rayon;
    float y1 = cercle.centre.y + cercle.rayon;

    DUAL_Vec2 p0 = { x0, y0 };
    DUAL_Vec2 p1 = { x1, y0 };
    DUAL_Vec2 p2 = { x1, y1 };
    DUAL_Vec2 p3 = { x0, y1 };

    // Coordonnées UV mappées de -1.0 à 1.0 (le fragment shader s'occupe de l'arrondi)
    DUAL_Vec2 uv0 = { -1.0f, -1.0f };
    DUAL_Vec2 uv1 = {  1.0f, -1.0f };
    DUAL_Vec2 uv2 = {  1.0f,  1.0f };
    DUAL_Vec2 uv3 = { -1.0f,  1.0f };

    uint32_t idx = renderer->vertex_count;

    renderer->vertex_buffer[idx++] = (Vertex2D){ p0, uv0, couleur };
    renderer->vertex_buffer[idx++] = (Vertex2D){ p1, uv1, couleur };
    renderer->vertex_buffer[idx++] = (Vertex2D){ p2, uv2, couleur };

    renderer->vertex_buffer[idx++] = (Vertex2D){ p0, uv0, couleur };
    renderer->vertex_buffer[idx++] = (Vertex2D){ p2, uv2, couleur };
    renderer->vertex_buffer[idx++] = (Vertex2D){ p3, uv3, couleur };

    renderer->vertex_count = idx;
}