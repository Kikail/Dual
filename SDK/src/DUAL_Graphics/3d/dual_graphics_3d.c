#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <stdint.h>

#include <glad/glad.h>

#include "../../../include/DUAL_Graphics/3d/dual_graphics_3d.h"
#include "../../../include/DUAL_Graphics/2d/dual_graphics_2d.h"
#include "../../../include/DUAL_Graphics/shader.h"
#include "../../../include/DUAL_Math/dual_math.h"
#include "../../../include/DUAL_Core/dual_core.h"
#include "../../../include/DUAL_Resources/dual_resources.h"
#include "../../../include/dual_utils.h"
#include "DUAL_Graphics/2d/dual_texture.h"

#define LIGHT_UNIFORMS(i) { \
    "uLights[" #i "].type", "uLights[" #i "].position", "uLights[" #i "].direction", \
    "uLights[" #i "].color", "uLights[" #i "].intensity", "uLights[" #i "].range" }

static const uint8_t kBoxEdges[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0},
    {4,5}, {5,6}, {6,7}, {7,4},
    {0,4}, {1,5}, {2,6}, {3,7}
};

static const char* const kLightUniforms[DUAL_MAX_LIGHTS_3D][6] = {
    LIGHT_UNIFORMS(0), LIGHT_UNIFORMS(1), LIGHT_UNIFORMS(2), LIGHT_UNIFORMS(3)
};

DUAL_Result DUAL_Renderer3D_Create(DUAL_App* app, DUAL_Renderer3D** out_renderer) {
    if (!app || !out_renderer) return DUAL_ERROR_INVALID_ARG;
    DUAL_Renderer3D* renderer = calloc(1, sizeof(DUAL_Renderer3D));
    if (!renderer) return DUAL_ERROR_OUT_OF_MEMORY;
    renderer->app = app;

    DUAL_Shader_load_VS_FS(&renderer->shaders.shader_lit, INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_lit_base.vs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_lit_base.fs", DUAL_SHADER_LIT);
    DUAL_Shader_load_VS_FS(&renderer->shaders.shader_unlit, INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_unlit_base.vs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_unlit_base.fs", DUAL_SHADER_UNLIT);
    DUAL_Shader_load_VS_FS(&renderer->shaders.shader_skeleton, INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_skeletal_base.vs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_lit_base.fs", DUAL_SHADER_LIT);
    DUAL_Shader_load_VS_FS(&renderer->shaders.shader_skeleton_unlit, INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_skeletal_base.vs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_unlit_base.fs", DUAL_SHADER_UNLIT);
    DUAL_Shader_load_VS_FS(&renderer->shaders.shader_debug, INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_debug_base.vs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_debug_base.fs", DUAL_SHADER_UNLIT);
    DUAL_Shader_load_VS_GEO_FS(&renderer->shaders.shader_billboard, INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_billboard.vs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_billboard.gs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_billboard.fs", DUAL_SHADER_UNLIT);

    renderer->render_mode = DUAL_RENDER_LIT;
    renderer->camera = DUAL_Camera3D_Create((DUAL_Vec3){0.0f, 0.0f, 0.0f}, DUAL_VECTOR_UP, DUAL_VECTOR_FRONT, DUAL_CAMERA_YAW, DUAL_CAMERA_PITCH, DUAL_CAMERA_SPEED, DUAL_CAMERA_SENSIVITY, DUAL_CAMERA_ZOOM);
    renderer->projection_mode = DUAL_PROJECTION_PERSPECTIVE;

    renderer->fov_radians = 1.0471975512f;
    renderer->ortho_demi_hauteur = 5.0f;
    renderer->plan_proche = 0.1f;
    renderer->plan_lointain = 500.0f;

    int32_t w = 0, h = 0;
    DUAL_GetScreenDimensions(renderer->app, &w, &h);
    float aspect = (h != 0) ? ((float)w / (float)h) : 1.0f;
    float demi_h = renderer->ortho_demi_hauteur;
    float demi_w = demi_h * aspect;
    renderer->orthographic_projection = DUAL_Mat4_Ortho(-demi_w, demi_w, -demi_h, demi_h, renderer->plan_proche, renderer->plan_lointain);
    renderer->perspective_projection = DUAL_Mat4_Perspective(renderer->fov_radians, aspect, renderer->plan_proche, renderer->plan_lointain);

    renderer->cull_mode = DUAL_CULL_BACK;

    const DUAL_Vec3 billboard_point = {0.0f, 0.0f, 0.0f};
    glGenVertexArrays(1, &renderer->billboard_vao);
    glGenBuffers(1, &renderer->billboard_vbo);
    glBindVertexArray(renderer->billboard_vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->billboard_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(DUAL_Vec3), &billboard_point, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DUAL_Vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    renderer->debug_renderer.count = 0;
    renderer->debug_renderer.capacity = DUAL_DEBUG_MAX_VERTICES;

    glGenVertexArrays(1, &renderer->debug_renderer.vao);
    glGenBuffers(1, &renderer->debug_renderer.vbo);
    glBindVertexArray(renderer->debug_renderer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->debug_renderer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(LineVertex) * DUAL_DEBUG_MAX_VERTICES, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, color));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    DUAL_Renderer3D_UseShader(renderer, DUAL_SHADER3D_SHADER_DEBUG);
    DUAL_Shader_setVec3(renderer->current_shader, "uColor", (DUAL_Vec3){1.0f, 1.0f, 1.0f});
    DUAL_Renderer3D_UseShader(renderer, DUAL_SHADER3D_LIT);

    *out_renderer = renderer;
    return DUAL_OK;
}

void DUAL_Renderer3D_Destroy(DUAL_Renderer3D* renderer) {
    if (!renderer) return;
    glDeleteVertexArrays(1, &renderer->billboard_vao);
    glDeleteBuffers(1, &renderer->billboard_vbo);
    glDeleteVertexArrays(1, &renderer->debug_renderer.vao);
    glDeleteBuffers(1, &renderer->debug_renderer.vbo);
    free(renderer);
}

void DUAL_Renderer3D_SetProjection(DUAL_Renderer3D* renderer, float fov_radians, float plan_proche, float plan_lointain) {
    if (!renderer) return;
    renderer->fov_radians = fov_radians;
    renderer->plan_proche = plan_proche;
    renderer->plan_lointain = plan_lointain;
}

void DUAL_Renderer3D_SetProjectionMode(DUAL_Renderer3D* renderer, DUAL_ProjectionMode3D mode) {
    if (!renderer) return;
    renderer->projection_mode = mode;
}

void DUAL_Renderer3D_SetOrthographic(DUAL_Renderer3D* renderer, float demi_hauteur, float plan_proche, float plan_lointain) {
    if (!renderer) return;
    renderer->ortho_demi_hauteur = demi_hauteur;
    renderer->plan_proche = plan_proche;
    renderer->plan_lointain = plan_lointain;
}

void DUAL_Renderer3D_SetCullMode(DUAL_Renderer3D* renderer, DUAL_CullMode mode) {
    if (!renderer) return;
    renderer->cull_mode = mode;
}

void DUAL_Renderer3D_SetRenderMode(DUAL_Renderer3D* renderer, DUAL_RenderMode3D mode) {
    if (!renderer) return;
    renderer->render_mode = mode;
}

void DUAL_Renderer3D_SetLight(DUAL_Renderer3D* renderer, int32_t index, DUAL_Light light) {
    if (!renderer || index < 0 || index >= DUAL_MAX_LIGHTS_3D) return;
    renderer->lights[index] = light;
}

void DUAL_Renderer3D_SetAmbientLight(DUAL_Renderer3D* renderer, DUAL_Vec3 couleur_ambiante) {
    if (!renderer) return;
    renderer->ambient_light = couleur_ambiante;
}

static void FlushDebugLines(DUAL_Renderer3D* debug, DUAL_Renderer3D* renderer) {
    if (debug->debug_renderer.count == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, debug->debug_renderer.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(LineVertex) * debug->debug_renderer.count, debug->debug_renderer.vertices);

    DUAL_Shader* previous_shader = renderer->current_shader;
    DUAL_Shader* shader = &renderer->shaders.shader_debug;
    renderer->current_shader = shader;

    DUAL_Shader_use(shader);
    DUAL_Shader_setMat4(shader, "uModel", DUAL_Mat4_Identity());
    DUAL_Shader_setMat4(shader, "uView", DUAL_Camera3D_GetViewMatrix(&renderer->camera));
    DUAL_Shader_setMat4(shader, "uProjection", renderer->perspective_projection);

    glBindVertexArray(debug->debug_renderer.vao);
    glDrawArrays(GL_LINES, 0, (GLsizei)debug->debug_renderer.count);
    glBindVertexArray(0);

    debug->debug_renderer.count = 0;

    renderer->current_shader = previous_shader;
    if (previous_shader) DUAL_Shader_use(previous_shader);
}

static void ApplyCullMode3D(DUAL_CullMode mode) {
    switch (mode) {
        case DUAL_CULL_BACK:  glEnable(GL_CULL_FACE); glCullFace(GL_BACK); break;
        case DUAL_CULL_FRONT: glEnable(GL_CULL_FACE); glCullFace(GL_FRONT); break;
        case DUAL_CULL_NONE:
        default:              glDisable(GL_CULL_FACE); break;
    }
}

void DUAL_Renderer3D_Begin(DUAL_Renderer3D* renderer) {
    if (!renderer) return;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClear(GL_DEPTH_BUFFER_BIT);
    ApplyCullMode3D(renderer->cull_mode);
    glPolygonMode(GL_FRONT_AND_BACK, (renderer->render_mode == DUAL_RENDER_WIREFRAME) ? GL_LINE : GL_FILL);
}

void DUAL_Renderer3D_End(DUAL_Renderer3D* renderer) {
    if (!renderer) return;
    FlushDebugLines(renderer, renderer);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}



void DUAL_DrawBillboard(DUAL_Renderer3D* renderer, const DUAL_Texture* texture, DUAL_Transform3D transform, DUAL_Color color) {
    if (!renderer || !texture) return;

    DUAL_Shader* previous_shader = renderer->current_shader;
    DUAL_Shader* shader = &renderer->shaders.shader_billboard;
    renderer->current_shader = shader;
    DUAL_Shader_use(shader);

    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, DUAL_Texture_GetTextureID(texture));
    glUniform1i(glGetUniformLocation(shader->shaderID, "texture_diffuse"), 0);

    DUAL_Shader_setMat4(shader, "uVP", DUAL_Mat4_Multiply(renderer->perspective_projection, DUAL_Camera3D_GetViewMatrix(&renderer->camera)));
    DUAL_Shader_setVec3(shader, "uCameraPos", renderer->camera.position);
    DUAL_Shader_setVec2(shader, "uSize", (DUAL_Vec2){ transform.echelle.x, transform.echelle.y });
    DUAL_Shader_setVec3(shader, "uPosition", transform.position);
    DUAL_Shader_setColor(shader, "uColor", color);

    glBindVertexArray(renderer->billboard_vao);
    glDrawArrays(GL_POINTS, 0, 1);
    glBindVertexArray(0);

    ApplyCullMode3D(renderer->cull_mode);
    glDepthMask(GL_TRUE);

    renderer->current_shader = previous_shader;
    if (previous_shader) {
        DUAL_Shader_use(previous_shader);
    }
}

void DUAL_Renderer3d_SendLightInfosToShader(DUAL_Renderer3D* renderer, DUAL_Shader* shader) {
    DUAL_Shader_setVec3(shader, "uAmbientLight", renderer->ambient_light);
    for (int i = 0; i < DUAL_MAX_LIGHTS_3D; i++) {
        const DUAL_Light* light = &renderer->lights[i];
        const char* const* names = kLightUniforms[i];
        DUAL_Shader_setInt(shader, names[0], light->type);
        DUAL_Shader_setVec3(shader, names[1], light->position);
        DUAL_Shader_setVec3(shader, names[2], light->direction);
        DUAL_Shader_setVec3(shader, names[3], light->couleur);
        DUAL_Shader_setFloat(shader, names[4], light->intensite);
        DUAL_Shader_setFloat(shader, names[5], light->portee);
    }
}

void DUAL_Renderer3D_UseShader(DUAL_Renderer3D* renderer, DUAL_Renderer3d_Shaders shader) {
    if (!renderer) return;
    switch (shader) {
        case DUAL_SHADER3D_LIT:
            renderer->current_shader = &renderer->shaders.shader_lit;
            break;
        case DUAL_SHADER3D_UNLIT:
            renderer->current_shader = &renderer->shaders.shader_unlit;
            break;
        case DUAL_SHADER3D_SKELETAL_LIT:
            renderer->current_shader = &renderer->shaders.shader_skeleton;
            break;
        case DUAL_SHADER3D_SKELETAL_UNLIT:
            renderer->current_shader = &renderer->shaders.shader_skeleton_unlit;
            break;
        case DUAL_SHADER3D_SHADER_DEBUG:
            renderer->current_shader = &renderer->shaders.shader_debug;
            break;
        default:
            break;
    }
}

void DUAL_Renderer3D_UseCustomShader(DUAL_Renderer3D* renderer, DUAL_Shader* shader) {
    if (!renderer || !shader) return;
    renderer->current_shader = shader;
}

DUAL_Camera3D* DUAL_Renderer3D_GetCamera(DUAL_Renderer3D* renderer) {
    return &renderer->camera;
}

void DUAL_Debug_DrawLine(DUAL_Renderer3D* debug, DUAL_Vec3 start, DUAL_Vec3 end, DUAL_Vec3 color) {
    if (debug->debug_renderer.count + 2 > debug->debug_renderer.capacity) return;

    debug->debug_renderer.vertices[debug->debug_renderer.count++] = (LineVertex){ start, color };
    debug->debug_renderer.vertices[debug->debug_renderer.count++] = (LineVertex){ end, color };
}

void DUAL_Debug_DrawCircle(DUAL_Renderer3D* debug, DUAL_Vec3 center, float radius, int segments, DUAL_Vec3 color) {
    if (segments <= 0) return;
    const float step = TWO_PI_F / (float)segments;

    DUAL_Vec3 p1 = { center.x + radius, center.y, center.z };

    for (int i = 0; i < segments; i++) {
        float angle = (float)(i + 1) * step;
        DUAL_Vec3 p2 = { center.x + cosf(angle) * radius, center.y, center.z + sinf(angle) * radius };
        DUAL_Debug_DrawLine(debug, p1, p2, color);
        p1 = p2;
    }
}

static void EmitBoxEdges(DUAL_Renderer3D* debug, const DUAL_Vec3 corners[8], DUAL_Vec3 color) {
    for (int i = 0; i < 12; i++) {
        DUAL_Debug_DrawLine(debug, corners[kBoxEdges[i][0]], corners[kBoxEdges[i][1]], color);
    }
}

void DUAL_Debug_DrawAABB(DUAL_Renderer3D* debug, DUAL_AABB box, DUAL_Vec3 color) {
    const DUAL_Vec3 min = box.min;
    const DUAL_Vec3 max = box.max;
    const DUAL_Vec3 corners[8] = {
        { min.x, min.y, min.z }, { max.x, min.y, min.z },
        { max.x, min.y, max.z }, { min.x, min.y, max.z },
        { min.x, max.y, min.z }, { max.x, max.y, min.z },
        { max.x, max.y, max.z }, { min.x, max.y, max.z }
    };
    EmitBoxEdges(debug, corners, color);
}

void DUAL_Debug_DrawSphere(DUAL_Renderer3D* debug, DUAL_Sphere sphere, DUAL_Vec3 color) {
    const int segments = 16;
    const float step = TWO_PI_F / (float)segments;
    const DUAL_Vec3 c = sphere.centre;
    const float r = sphere.rayon;

    float cos1 = 1.0f, sin1 = 0.0f;

    for (int i = 0; i < segments; i++) {
        float theta2 = (float)(i + 1) * step;
        float cos2 = cosf(theta2), sin2 = sinf(theta2);

        DUAL_Debug_DrawLine(debug,
            (DUAL_Vec3){ c.x + r * cos1, c.y, c.z + r * sin1 },
            (DUAL_Vec3){ c.x + r * cos2, c.y, c.z + r * sin2 }, color);

        DUAL_Debug_DrawLine(debug,
            (DUAL_Vec3){ c.x + r * cos1, c.y + r * sin1, c.z },
            (DUAL_Vec3){ c.x + r * cos2, c.y + r * sin2, c.z }, color);

        DUAL_Debug_DrawLine(debug,
            (DUAL_Vec3){ c.x, c.y + r * cos1, c.z + r * sin1 },
            (DUAL_Vec3){ c.x, c.y + r * cos2, c.z + r * sin2 }, color);

        cos1 = cos2;
        sin1 = sin2;
    }
}

void DUAL_Debug_Render(DUAL_Renderer3D* debug, DUAL_Renderer3D* renderer) {
    FlushDebugLines(debug, renderer);
}

DUAL_Vec3 DUAL_Mat4_TransformPoint(DUAL_Mat4 m, DUAL_Vec3 v) {
    DUAL_Vec3 res;
    res.x = m.m[0]*v.x + m.m[4]*v.y + m.m[8]*v.z  + m.m[12];
    res.y = m.m[1]*v.x + m.m[5]*v.y + m.m[9]*v.z  + m.m[13];
    res.z = m.m[2]*v.x + m.m[6]*v.y + m.m[10]*v.z + m.m[14];
    return res;
}


void DUAL_Debug_Transform(DUAL_Transform3D transform) {
    DUAL_Debug_Vec3(transform.position);
    DUAL_Debug_Vec3(transform.rotation_euler_radians);
    DUAL_Debug_Vec3(transform.echelle);
    printf("\n");
}
void DUAL_Debug_Vec3(DUAL_Vec3 vector) {
    printf("(%f, %f, %f)", vector.x, vector.y, vector.z);
}

DUAL_Mat4 DUAL_BuildModelMatrix(const DUAL_Transform3D* transform) {
    DUAL_Mat4 rotation = DUAL_Mat4_Identity();
    if (transform->rotation_euler_radians.x != 0.0f) {
        rotation = DUAL_Mat4_Rotate((DUAL_Vec3){ 1.0f, 0.0f, 0.0f }, transform->rotation_euler_radians.x);
    }
    if (transform->rotation_euler_radians.y != 0.0f) {
        rotation = DUAL_Mat4_Multiply(DUAL_Mat4_Rotate((DUAL_Vec3){ 0.0f, 1.0f, 0.0f }, transform->rotation_euler_radians.y), rotation);
    }
    if (transform->rotation_euler_radians.z != 0.0f) {
        rotation = DUAL_Mat4_Multiply(DUAL_Mat4_Rotate((DUAL_Vec3){ 0.0f, 0.0f, 1.0f }, transform->rotation_euler_radians.z), rotation);
    }
    return DUAL_Mat4_Multiply(DUAL_Mat4_Translate(transform->position),
           DUAL_Mat4_Multiply(rotation, DUAL_Mat4_Scale(transform->echelle)));
}

void DUAL_Debug_Draw_Model_BoundingBox(DUAL_Renderer3D* debug, DUAL_AABB box, DUAL_Transform3D transform, DUAL_Vec3 color) {
    const DUAL_Mat4 modelMat = DUAL_BuildModelMatrix(&transform);

    const DUAL_Vec3 min = box.min;
    const DUAL_Vec3 max = box.max;
    DUAL_Vec3 corners[8] = {
        { min.x, min.y, min.z }, { max.x, min.y, min.z },
        { max.x, min.y, max.z }, { min.x, min.y, max.z },
        { min.x, max.y, min.z }, { max.x, max.y, min.z },
        { max.x, max.y, max.z }, { min.x, max.y, max.z }
    };

    for (int i = 0; i < 8; i++) {
        corners[i] = DUAL_Mat4_TransformPoint(modelMat, corners[i]);
    }

    EmitBoxEdges(debug, corners, color);
}