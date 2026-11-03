#include <stddef.h>
#include <glad/glad.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Inclusion de nos modules */
#include "../../../include/DUAL_Graphics/2d/dual_graphics_2d.h"
#include "../../../include/DUAL_Math/dual_math.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "DUAL_Graphics/2d/dual_texture.h"
#include "DUAL_Resources/dual_resources.h"


DUAL_Result DUAL_Renderer2D_Create(DUAL_App* app, DUAL_Renderer2D** out_renderer) {
    if (!app || !out_renderer) return DUAL_ERROR_INVALID_ARG;

    DUAL_Renderer2D* renderer = (DUAL_Renderer2D*)malloc(sizeof(DUAL_Renderer2D));
    if (!renderer) return DUAL_ERROR_OUT_OF_MEMORY;

    renderer->app = app;

    DUAL_Result result = DUAL_OK;
    bool compilation_result = true;
    compilation_result = DUAL_Shader_load_VS_FS(&renderer->shaders.base_shader, INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_2d_base.vs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_2d_base.fs", DUAL_SHADER_LIT);


    // On assigne le shader par defaut dans le shader principal
    renderer->current_shader = &renderer->shaders.base_shader;

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

    renderer->camera.position = (DUAL_Vec2){0.0f, 0.0f};
    renderer->camera.zoom = 1.0f;
    renderer->camera.rotation = 0.0f;

    *out_renderer = renderer;
    return DUAL_OK;
}

void DUAL_Renderer2D_Destroy(DUAL_Renderer2D* renderer) {
    if (renderer) {
        glDeleteVertexArrays(1, &renderer->vao);
        glDeleteBuffers(1, &renderer->vbo);
        free(renderer->vertex_buffer);
        free(renderer);
    }
}

void DUAL_Renderer2D_FlushBatch(DUAL_Renderer2D* renderer) {
    if (renderer->vertex_count == 0) return;

    DUAL_Shader_use(renderer->current_shader);

    int32_t w, h;
    DUAL_GetScreenDimensions(renderer->app, &w, &h);
    DUAL_Mat4 projection = DUAL_Mat4_Ortho(0.0f, (float)w, (float)h, 0.0f, -1.0f, 1.0f);

    DUAL_Mat4 view = DUAL_Camera2D_GetViewMatrix(&renderer->camera);

    DUAL_Shader_setMat4(renderer->current_shader, "uProjection", projection);
    DUAL_Shader_setMat4(renderer->current_shader, "uView", view);

    // BIND DE LA TEXTURE ET UNIFORM
    if (renderer->texture_courante && renderer->texture_courante->id_opengl != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderer->texture_courante->id_opengl);
        DUAL_Shader_setInt(renderer->current_shader, "uTexture", 0);
        DUAL_Shader_setBool(renderer->current_shader, "uUseTexture", true);
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
        DUAL_Shader_setBool(renderer->current_shader, "uUseTexture", false);
    }

    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex2D) * renderer->vertex_count, renderer->vertex_buffer);

    glDrawArrays(GL_TRIANGLES, 0, renderer->vertex_count);

    glBindVertexArray(0);
    renderer->vertex_count = 0;
}

DUAL_Camera2D* DUAL_Renderer2D_GetCamera(DUAL_Renderer2D* renderer) {
    if (!renderer) return NULL;
    return &renderer->camera;
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
        DUAL_Renderer2D_FlushBatch(renderer);
    }
}

void DUAL_DrawRect(DUAL_Renderer2D* renderer, DUAL_Rect rect, DUAL_Color couleur) {
    if (!renderer) return;

    if (renderer->texture_courante != NULL) {
        DUAL_Renderer2D_FlushBatch(renderer);
        renderer->texture_courante = NULL;
    }

    if (renderer->vertex_count + 6 >= renderer->max_vertices) {
        DUAL_Renderer2D_FlushBatch(renderer);
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
        DUAL_Renderer2D_FlushBatch(renderer);
        renderer->texture_courante = NULL;
    }

    if (renderer->vertex_count + 6 >= renderer->max_vertices) {
        DUAL_Renderer2D_FlushBatch(renderer);
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

void DUAL_Renderer2D_UseShader(DUAL_Renderer2D* renderer, DUAL_Renderer2d_Shaders shader) {
    if (!renderer) return;
    switch (shader) {
        case DUAL_SHADER2D_DEFAULT:
            renderer->current_shader = &renderer->shaders.base_shader;
            break;
        default:
            break;
    }
}

void DUAL_Renderer2D_UseCustomShader(DUAL_Renderer2D* renderer, DUAL_Shader* shader) {
    if (!renderer || !shader) return;
    renderer->current_shader = shader;
}