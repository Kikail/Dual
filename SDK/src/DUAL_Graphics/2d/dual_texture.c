//
// Created by killian on 11/3/26.
//
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCAL
#include "stb_image.h"
#include "../../SDK/include/DUAL_Graphics/2d/dual_texture.h"
#include <stdlib.h>
#include "DUAL_Resources/dual_resources.h"
#include "../../SDK/include/DUAL_Core/dual_file.h"

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

    strcpy((*out_texture)->texture_path, chemin_fichier);
    DUAL_GetFilenameWithoutExtension((*out_texture)->texture_path, (*out_texture)->texture_name, 32);
    DUAL_ResourceManager_AddResource(resources, *out_texture, DUAL_RESOURCE_TEXTURE);

    return resultat;
}

DUAL_Result DUAL_Texture_LoadFromVFS(DUAL_ResourceManager* resources, DUAL_VFS_File* file, DUAL_TextureFilter filtre, DUAL_Texture** out_texture) {
    if (!resources || !file || !out_texture) return DUAL_ERROR_INVALID_ARG;

    int largeur, hauteur, canaux;

    stbi_set_flip_vertically_on_load(false);
    uint8_t* pixels = stbi_load_from_memory(file->data, file->size, &largeur, &hauteur, &canaux, STBI_rgb_alpha);

    if (!pixels) {
        printf("[ERREUR TEXTURE] Impossible de charger le fichier vfs: %s\n", file->name);
        return DUAL_ERROR_NOT_FOUND;
    }

    DUAL_Result resultat = DUAL_Texture_LoadFromMemory(resources, pixels, largeur, hauteur, filtre, out_texture);
    stbi_image_free(pixels);

    strcpy((*out_texture)->texture_path, file->name);
    DUAL_GetFilenameWithoutExtension((*out_texture)->texture_path, (*out_texture)->texture_name, 32);
    DUAL_ResourceManager_AddResource(resources, *out_texture, DUAL_RESOURCE_TEXTURE);

    return resultat;
}


DUAL_Result DUAL_Texture_LoadFromMemory(DUAL_ResourceManager* resources, const uint8_t* pixels, int32_t largeur, int32_t hauteur, DUAL_TextureFilter filtre, DUAL_Texture** out_texture) {
    if (!pixels || !out_texture) return DUAL_ERROR_INVALID_ARG;

    uint64_t taille_vram = (uint64_t)largeur * hauteur * 4;

    DUAL_Texture* tex = (DUAL_Texture*)malloc(sizeof(DUAL_Texture));

    tex->largeur = largeur;
    tex->hauteur = hauteur;

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

void DUAL_Texture_Destroy(void* texture) {
    DUAL_Texture* t = (DUAL_Texture*)texture;
    if (texture) {
        glDeleteTextures(1, &t->id_opengl);
        free(texture);
    }
}

void DUAL_Texture_GetSize(const DUAL_Texture* texture, int32_t* out_largeur, int32_t* out_hauteur) {
    if (texture) {
        if (out_largeur) *out_largeur = texture->largeur;
        if (out_hauteur) *out_hauteur = texture->hauteur;
    }
}

void DUAL_DrawSprite(DUAL_Renderer2D* renderer, const DUAL_SpriteParams* params) {
    if (!renderer || !params || !params->texture) return;

    if (renderer->texture_courante != params->texture) {
        DUAL_Renderer2D_FlushBatch(renderer);
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
        DUAL_Renderer2D_FlushBatch(renderer);
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

GLuint DUAL_Texture_GetTextureID(const DUAL_Texture* texture) {
    return texture ? texture->id_opengl : 0;
}