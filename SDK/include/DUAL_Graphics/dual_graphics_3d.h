/**
 * @file dual_graphics_3d.h
 * @brief Module de rendu 3D de libdual : chargement de modèles,
 *        gestion des matériaux, éclairage, caméra et système d'animation squelettique.
 */

#ifndef DUAL_GRAPHICS_3D_H
#define DUAL_GRAPHICS_3D_H

#include <stdint.h>
#include <stdbool.h>
#include "../DUAL_Core/dual_core.h"
#include "../DUAL_Math/dual_math.h"
#include "../DUAL_Resources/dual_resources.h"
#include "dual_graphics_2d.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BONES 100

/* ============================================================================
 *  Types opaques et Déclarations
 * ========================================================================== */

typedef struct DUAL_Mesh DUAL_Mesh;
typedef struct DUAL_Model DUAL_Model;
typedef struct DUAL_Material DUAL_Material;
typedef struct DUAL_Renderer3D DUAL_Renderer3D;
typedef struct KeyPosition KeyPosition;
typedef struct KeyRotation KeyRotation;
typedef struct KeyScale KeyScale;
typedef struct Bone Bone;
typedef struct AssimpNodeData AssimpNodeData;
typedef struct Animation Animation;
typedef struct Animator Animator;
typedef struct DUAL_Shader DUAL_Shader;
typedef struct DUAL_Camera3D DUAL_Camera3D;

/* ============================================================================
 *  Énumérations
 * ========================================================================== */

typedef enum DUAL_LightType {
    DUAL_LIGHT_DIRECTIONAL = 0,
    DUAL_LIGHT_POINT       = 1
} DUAL_LightType;

typedef enum DUAL_ProjectionMode3D {
    DUAL_PROJECTION_PERSPECTIVE  = 0,
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

/* Structures de gestion de l'animation */

struct KeyPosition {
    DUAL_Vec3 position;
    float timeStamp;
};

struct KeyRotation {
    DUAL_Quat orientation;
    float timeStamp;
};

struct KeyScale {
    DUAL_Vec3 scale;
    float timeStamp;
};

struct Bone {
    KeyPosition* m_Positions;
    KeyRotation* m_Rotations;
    KeyScale*    m_Scales;
    int m_NumPositions;
    int m_NumRotations;
    int m_NumScalings;

    DUAL_Mat4 m_LocalTransform;
    char* m_Name;
    int m_ID;
};

struct AssimpNodeData {
    DUAL_Mat4 transformation;
    char* name;
    int childrenCount;
    struct AssimpNodeData* children;
};

struct Animation {
    float m_Duration;
    int m_TicksPerSecond;

    Bone* m_Bones;
    int m_NumBones;

    AssimpNodeData m_RootNode;

    DUAL_Model* m_LinkedModel;
};

struct Animator {
    DUAL_Mat4 m_FinalBoneMatrices[MAX_BONES];
    Animation* m_CurrentAnimation;
    float m_CurrentTime;
    float m_DeltaTime;
};

typedef enum DUAL_Renderer3d_Shaders {
    SHADER3D_LIT,
    SHADER3D_UNLIT,
    SHADER3D_SKELETAL_LIT,
    SHADER3D_SKELETAL_UNLIT,
    SHADER3D_SHADER_DEBUG
}DUAL_Renderer3d_Shaders;

DUAL_Camera3D* DUAL_Renderer3D_GetCamera(DUAL_Renderer3D* renderer);

/* ============================================================================
 *  Chargement et gestion des modèles
 * ========================================================================== */

DUAL_Result DUAL_Model_Load(DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Model** out_model, unsigned int* out_material_count, DUAL_Material*** out_materials);
void DUAL_Model_Destroy(DUAL_ResourceManager* resources, DUAL_Model* model);
DUAL_AABB DUAL_Model_GetBoundingBox(const DUAL_Model* model);
uint32_t DUAL_Model_GetMeshCount(const DUAL_Model* model);

/* ============================================================================
 *  Matériaux
 * ========================================================================== */

DUAL_Result DUAL_Material_Create(DUAL_ResourceManager* resources, DUAL_Texture* texture_diffuse, DUAL_Material** out_material);
void DUAL_Material_Destroy(DUAL_ResourceManager* resources, DUAL_Material* material);

/* ============================================================================
 *  Animations & Bones
 * ========================================================================== */

DUAL_Result DUAL_Animation_Load(const char* path, DUAL_Model* model, Animation** out_animation);
void Animator_Init(Animator* animator, Animation* animation);
void Animator_UpdateAnimation(Animator* animator, float dt);
void Bone_Update(Bone* bone, float animationTime);

/* ============================================================================
 *  Renderer 3D et caméra
 * ========================================================================== */

DUAL_Result DUAL_Renderer3D_Create(DUAL_App* app, DUAL_Renderer3D** out_renderer);
void DUAL_Renderer3D_Destroy(DUAL_Renderer3D* renderer);
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
 *  Dessin (Mise à jour : Tableau de pointeurs de matériaux par mesh)
 * ========================================================================== */

void DUAL_DrawModel(DUAL_Renderer3D* renderer, const DUAL_Model* model, const DUAL_Material** materials, DUAL_Transform3D transform);
void DUAL_DrawAnimatedModel(DUAL_Renderer3D* renderer, const DUAL_Model* model, const DUAL_Material** materials, DUAL_Transform3D transform, Animator* animator);

/* ============================================================================
 *  Shaders
 * ========================================================================== */
void DUAL_Renderer3D_UseCustomShader(DUAL_Renderer3D* renderer, DUAL_Shader* shader);
void DUAL_Renderer3D_UseShader(DUAL_Renderer3D* renderer, DUAL_Renderer3d_Shaders shader);
void DUAL_Renderer3d_SendLightInfosToShader(DUAL_Renderer3D* renderer, DUAL_Shader* shader);

/* ============================================================================
 *  Debug
 * ========================================================================== */

#define DUAL_DEBUG_MAX_VERTICES 20000

typedef struct {
    DUAL_Vec3 position;
    DUAL_Vec3 color;
} LineVertex;

typedef struct {
    GLuint vao, vbo;
    LineVertex vertices[DUAL_DEBUG_MAX_VERTICES];
    uint32_t capacity;
    uint32_t count;
} DUAL_DebugRenderer3D;

void DUAL_Debug_DrawLine(DUAL_Renderer3D* debug, DUAL_Vec3 start, DUAL_Vec3 end, DUAL_Vec3 color);
void DUAL_Debug_DrawCircle(DUAL_Renderer3D* debug, DUAL_Vec3 center, float radius, int segments, DUAL_Vec3 color);
void DUAL_Debug_DrawAABB(DUAL_Renderer3D* debug, DUAL_AABB box, DUAL_Vec3 color);
void DUAL_Debug_Render(DUAL_Renderer3D* debug, DUAL_Renderer3D* renderer);
void DUAL_Debug_Draw_Model_BoundingBox(DUAL_Renderer3D* debug, DUAL_AABB box, DUAL_Transform3D transform, DUAL_Vec3 color);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_GRAPHICS_3D_H */