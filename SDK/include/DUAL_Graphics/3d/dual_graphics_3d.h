/**
 * @file dual_graphics_3d.h
 * @brief Module de rendu 3D de libdual : chargement de modèles,
 *        gestion des matériaux, éclairage, caméra et système d'animation squelettique.
 */

#ifndef DUAL_GRAPHICS_3D_H
#define DUAL_GRAPHICS_3D_H

#include <stdint.h>
#include <stdbool.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "../../DUAL_Core/dual_core.h"
#include "../../DUAL_Core/dual_enums.h"
#include "../../DUAL_Math/dual_math.h"
#include "camera3d.h"
#include "../2d/dual_graphics_2d.h"
#include "hashtable/hashtable.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  Types opaques et Déclarations
 * ========================================================================== */



#define DUAL_MAX_LIGHTS_3D 4
#define TWO_PI_F 6.28318530718f
#define DUAL_DEBUG_MAX_VERTICES 20000

typedef enum DUAL_LightType {
    DUAL_LIGHT_DIRECTIONAL = 0,
    DUAL_LIGHT_POINT       = 1
} DUAL_LightType;

typedef struct DUAL_Light {
    DUAL_LightType type;
    DUAL_Vec3       position;
    DUAL_Vec3       direction;
    DUAL_Vec3       couleur;
    float           intensite;
    float           portee;
} DUAL_Light;

typedef struct DUAL_Transform3D {
    DUAL_Vec3 position;
    DUAL_Vec3 rotation_euler_radians;
    DUAL_Vec3 echelle;
} DUAL_Transform3D;


typedef struct LineVertex{
    DUAL_Vec3 position;
    DUAL_Vec3 color;
} LineVertex;

typedef struct DUAL_DebugRenderer3D{
    GLuint vao, vbo;
    LineVertex vertices[DUAL_DEBUG_MAX_VERTICES];
    uint32_t capacity;
    uint32_t count;
} DUAL_DebugRenderer3D;

typedef struct Internal_Shaders3d_s{
    DUAL_Shader shader_lit;
    DUAL_Shader shader_unlit;
    DUAL_Shader shader_skeleton;
    DUAL_Shader shader_skeleton_unlit;
    DUAL_Shader shader_debug;
    DUAL_Shader shader_billboard;
}Internal_Shaders3d_s;

typedef struct DUAL_Renderer3D{
    DUAL_App* app;

    DUAL_Shader* current_shader;
    Internal_Shaders3d_s shaders;

    GLuint ubo_bones;

    DUAL_RenderMode3D render_mode;

    DUAL_Camera3D camera;

    DUAL_ProjectionMode3D projection_mode;
    DUAL_Mat4 perspective_projection;
    DUAL_Mat4 orthographic_projection;
    float fov_radians;
    float ortho_demi_hauteur;
    float plan_proche;
    float plan_lointain;

    DUAL_CullMode cull_mode;

    DUAL_Vec3  ambient_light;
    DUAL_Light lights[DUAL_MAX_LIGHTS_3D];

    GLuint billboard_vbo, billboard_vao;

    DUAL_DebugRenderer3D debug_renderer;
}DUAL_Renderer3D;


DUAL_Camera3D* DUAL_Renderer3D_GetCamera(DUAL_Renderer3D* renderer);
DUAL_Result DUAL_Renderer3D_Create(DUAL_App* app, DUAL_Renderer3D** out_renderer);
void DUAL_Renderer3D_Destroy(DUAL_Renderer3D* renderer);
void DUAL_Renderer3D_SetProjection(DUAL_Renderer3D* renderer, float fov_radians, float plan_proche, float plan_lointain);
void DUAL_Renderer3D_SetProjectionMode(DUAL_Renderer3D* renderer, DUAL_ProjectionMode3D mode);
void DUAL_Renderer3D_SetOrthographic(DUAL_Renderer3D* renderer, float demi_hauteur, float plan_proche, float plan_lointain);
void DUAL_Renderer3D_SetCullMode(DUAL_Renderer3D* renderer, DUAL_CullMode mode);
void DUAL_Renderer3D_SetRenderMode(DUAL_Renderer3D* renderer, DUAL_RenderMode3D mode);
void DUAL_Renderer3D_Begin(DUAL_Renderer3D* renderer);
void DUAL_Renderer3D_End(DUAL_Renderer3D* renderer);
void DUAL_Renderer3D_SetLight(DUAL_Renderer3D* renderer, int32_t index, DUAL_Light light);
void DUAL_Renderer3D_SetAmbientLight(DUAL_Renderer3D* renderer, DUAL_Vec3 couleur_ambiante);
void DUAL_Renderer3D_UseCustomShader(DUAL_Renderer3D* renderer, DUAL_Shader* shader);
void DUAL_Renderer3D_UseShader(DUAL_Renderer3D* renderer, DUAL_Renderer3d_Shaders shader);
void DUAL_Renderer3d_SendLightInfosToShader(DUAL_Renderer3D* renderer, DUAL_Shader* shader);

void DUAL_Debug_DrawLine(DUAL_Renderer3D* debug, DUAL_Vec3 start, DUAL_Vec3 end, DUAL_Vec3 color);
void DUAL_Debug_DrawCircle(DUAL_Renderer3D* debug, DUAL_Vec3 center, float radius, int segments, DUAL_Vec3 color);
void DUAL_Debug_DrawAABB(DUAL_Renderer3D* debug, DUAL_AABB box, DUAL_Vec3 color);
void DUAL_Debug_DrawSphere(DUAL_Renderer3D* debug, DUAL_Sphere sphere, DUAL_Vec3 color);
void DUAL_Debug_Render(DUAL_Renderer3D* debug, DUAL_Renderer3D* renderer);
void DUAL_Debug_Draw_Model_BoundingBox(DUAL_Renderer3D* debug, DUAL_AABB box, DUAL_Transform3D transform, DUAL_Vec3 color);
void DUAL_Debug_Transform(DUAL_Transform3D transform);
void DUAL_Debug_Vec3(DUAL_Vec3 vector);

DUAL_Mat4 DUAL_BuildModelMatrix(const DUAL_Transform3D* transform);

void DUAL_DrawBillboard(DUAL_Renderer3D* renderer, const DUAL_Texture* texture, DUAL_Transform3D transform, DUAL_Color color);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_GRAPHICS_3D_H */