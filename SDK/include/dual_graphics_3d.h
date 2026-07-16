/**
 * @file dual_graphics_3d.h
 * @brief Module de rendu 3D de libdual : chargement de modèles (format OBJ),
 *        gestion des matériaux, éclairage de base et caméra 3D.
 */

#ifndef DUAL_GRAPHICS_3D_H
#define DUAL_GRAPHICS_3D_H

#include <stdint.h>
#include <stdbool.h>
#include "dual_core.h"
#include "dual_math.h"
#include "dual_resources.h"
#include "dual_graphics_2d.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  Types opaques
 * ========================================================================== */

typedef struct DUAL_Model DUAL_Model;
typedef struct DUAL_Material DUAL_Material;
typedef struct DUAL_Renderer3D DUAL_Renderer3D;

/* ============================================================================
 *  Enumérations
 * ========================================================================== */

typedef enum DUAL_LightType {
    DUAL_LIGHT_DIRECTIONAL = 0,
    DUAL_LIGHT_POINT       = 1
} DUAL_LightType;

typedef enum DUAL_ProjectionMode3D {
    DUAL_PROJECTION_PERSPECTIVE = 0,
    DUAL_PROJECTION_ORTHOGRAPHIC = 1
} DUAL_ProjectionMode3D;

typedef enum DUAL_CullMode {
    DUAL_CULL_BACK  = 0,
    DUAL_CULL_FRONT = 1,
    DUAL_CULL_NONE  = 2
} DUAL_CullMode;

typedef enum DUAL_RenderMode3D {
    DUAL_RENDER_LIT       = 0,
    DUAL_RENDER_UNLIT     = 1,
    DUAL_RENDER_WIREFRAME = 2
} DUAL_RenderMode3D;

/* ============================================================================
 *  Structures
 * ========================================================================== */

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

/* ============================================================================
 *  Chargement et gestion des modèles
 * ========================================================================== */

DUAL_Result DUAL_Model_LoadFromOBJ(DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Model** out_model);
void DUAL_Model_Destroy(DUAL_ResourceManager* resources, DUAL_Model* model);
DUAL_AABB DUAL_Model_GetBoundingBox(const DUAL_Model* model);

/* ============================================================================
 *  Matériaux
 * ========================================================================== */

DUAL_Result DUAL_Material_Create(DUAL_ResourceManager* resources, DUAL_Texture* texture_diffuse, DUAL_Material** out_material);
void DUAL_Material_Destroy(DUAL_ResourceManager* resources, DUAL_Material* material);
void DUAL_Material_SetShininess(DUAL_Material* material, float brillance);

/* ============================================================================
 *  Renderer 3D et caméra
 * ========================================================================== */

DUAL_Result DUAL_Renderer3D_Create(DUAL_App* app, DUAL_Renderer3D** out_renderer);
void DUAL_Renderer3D_Destroy(DUAL_Renderer3D* renderer);

void DUAL_Renderer3D_SetCameraLookAt(DUAL_Renderer3D* renderer, DUAL_Vec3 position, DUAL_Vec3 cible, DUAL_Vec3 haut);
void DUAL_Renderer3D_SetProjection(DUAL_Renderer3D* renderer, float fov_radians, float plan_proche, float plan_lointain);
void DUAL_Renderer3D_SetProjectionMode(DUAL_Renderer3D* renderer, DUAL_ProjectionMode3D mode);
void DUAL_Renderer3D_SetOrthographic(DUAL_Renderer3D* renderer, float demi_hauteur, float plan_proche, float plan_lointain);
void DUAL_Renderer3D_SetCullMode(DUAL_Renderer3D* renderer, DUAL_CullMode mode);

void DUAL_Renderer3D_SetRenderMode(DUAL_Renderer3D* renderer, DUAL_RenderMode3D mode);

void DUAL_Renderer3D_Begin(DUAL_Renderer3D* renderer);
void DUAL_Renderer3D_End(DUAL_Renderer3D* renderer);

/* ============================================================================
 *  Éclairage
 * ========================================================================== */

void DUAL_Renderer3D_SetLight(DUAL_Renderer3D* renderer, int32_t index, DUAL_Light light);
void DUAL_Renderer3D_SetAmbientLight(DUAL_Renderer3D* renderer, DUAL_Vec3 couleur_ambiante);

/* ============================================================================
 *  Dessin
 * ========================================================================== */

void DUAL_DrawModel(DUAL_Renderer3D* renderer, const DUAL_Model* model, const DUAL_Material* material, DUAL_Transform3D transform);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_GRAPHICS_3D_H */