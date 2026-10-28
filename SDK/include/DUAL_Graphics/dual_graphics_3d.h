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
#include "../DUAL_Core/dual_core.h"
#include "../DUAL_Math/dual_math.h"
#include "../../include/DUAL_Graphics/camera3d.h"
#include "dual_graphics_2d.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BONES 100

/* ============================================================================
 *  Types opaques et Déclarations
 * ========================================================================== */



#define DUAL_MAX_LIGHTS_3D 4
#define MAX_BONE_INFLUENCE 4
#define MAX_BONES 100
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

/* Structures de gestion de l'animation */

typedef struct KeyPosition{
    DUAL_Vec3 position;
    float timeStamp;
}KeyPosition;

typedef struct KeyRotation{
    DUAL_Quat orientation;
    float timeStamp;
}KeyRotation;

typedef struct KeyScale{
    DUAL_Vec3 scale;
    float timeStamp;
}KeyScale;

typedef struct Bone{
    KeyPosition* m_Positions;
    KeyRotation* m_Rotations;
    KeyScale*    m_Scales;
    int m_NumPositions;
    int m_NumRotations;
    int m_NumScalings;

    DUAL_Mat4 m_LocalTransform;
    char* m_Name;
    int m_ID;
}Bone;

typedef struct AssimpNodeData {
    DUAL_Mat4 transformation;
    char* name;
    int childrenCount;
    struct AssimpNodeData* children;
}AssimpNodeData;


typedef enum DUAL_Renderer3d_Shaders {
    SHADER3D_LIT,
    SHADER3D_UNLIT,
    SHADER3D_SKELETAL_LIT,
    SHADER3D_SKELETAL_UNLIT,
    SHADER3D_SHADER_DEBUG
}DUAL_Renderer3d_Shaders;

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

typedef struct BoneInfo{
    char* name;
    int id;
    DUAL_Mat4 offset;
} BoneInfo;

typedef struct DUAL_Mesh{
    GLuint   vao, vbo, ebo;
    uint32_t index_count;
    uint32_t material_index;
}DUAL_Mesh;

typedef struct DUAL_Material{
    DUAL_Texture* texture_diffuse;
}DUAL_Material;

typedef struct DUAL_Model{
    DUAL_Mesh* meshes;
    uint32_t   mesh_count;
    DUAL_AABB  bounding_box_local;

    DUAL_Material** materials;
    unsigned int material_count;

    BoneInfo* m_BoneInfoMap;
    int m_BoneCounter;
    int m_BoneMapCapacity;

    char model_name[32];
    char model_path[128];
}DUAL_Model;


typedef struct Animation{
    float m_Duration;
    int m_TicksPerSecond;

    Bone* m_Bones;
    int m_NumBones;

    AssimpNodeData m_RootNode;

    DUAL_Model* m_LinkedModel;
}Animation;

typedef struct Animator{
    DUAL_Mat4 m_FinalBoneMatrices[MAX_BONES];
    Animation* m_CurrentAnimation;
    float m_CurrentTime;
    float m_DeltaTime;
    bool m_Loop;
    bool m_Finished;
}Animator;

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

typedef struct Vertex3D{
    DUAL_Vec3 position;
    DUAL_Vec3 normale;
    DUAL_Vec2 tex_coords;
    float   m_Weights[MAX_BONE_INFLUENCE];
    int32_t m_BoneIDs[MAX_BONE_INFLUENCE];
} Vertex3D;

/* ============================================================================
 *  Énumérations
 * ========================================================================== */



/* ============================================================================
 *  Structures
 * ========================================================================== */



typedef struct DUAL_Animated_Model{
    DUAL_Model* model;
    Animation** animations;
    unsigned int animation_count;
    Animator animator;

    char model_name[32];
    char model_path[128];
}DUAL_Animated_Model;


DUAL_Camera3D* DUAL_Renderer3D_GetCamera(DUAL_Renderer3D* renderer);

/* ============================================================================
 *  Chargement et gestion des modèles
 * ========================================================================== */

DUAL_Result DUAL_Model_Load(DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Model** out_model, unsigned int* out_material_count, DUAL_Material*** out_materials);
void DUAL_Model_Destroy(DUAL_Model* model);
DUAL_AABB DUAL_Model_GetBoundingBox(const DUAL_Model* model);
uint32_t DUAL_Model_GetMeshCount(const DUAL_Model* model);

/* ============================================================================
 *  Matériaux
 * ========================================================================== */

DUAL_Result DUAL_Material_Create(DUAL_ResourceManager* resources, DUAL_Texture* texture_diffuse, DUAL_Material** out_material);
void DUAL_Material_Destroy(DUAL_Material* material);

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
void DUAL_DrawBillboard(DUAL_Renderer3D* renderer, const DUAL_Texture* texture, DUAL_Transform3D transform, DUAL_Color color);

/* ============================================================================
 *  Shaders
 * ========================================================================== */
void DUAL_Renderer3D_UseCustomShader(DUAL_Renderer3D* renderer, DUAL_Shader* shader);
void DUAL_Renderer3D_UseShader(DUAL_Renderer3D* renderer, DUAL_Renderer3d_Shaders shader);
void DUAL_Renderer3d_SendLightInfosToShader(DUAL_Renderer3D* renderer, DUAL_Shader* shader);

/* ============================================================================
 *  Debug
 * ========================================================================== */

void DUAL_Debug_DrawLine(DUAL_Renderer3D* debug, DUAL_Vec3 start, DUAL_Vec3 end, DUAL_Vec3 color);
void DUAL_Debug_DrawCircle(DUAL_Renderer3D* debug, DUAL_Vec3 center, float radius, int segments, DUAL_Vec3 color);
void DUAL_Debug_DrawAABB(DUAL_Renderer3D* debug, DUAL_AABB box, DUAL_Vec3 color);
void DUAL_Debug_DrawSphere(DUAL_Renderer3D* debug, DUAL_Sphere sphere, DUAL_Vec3 color);
void DUAL_Debug_Render(DUAL_Renderer3D* debug, DUAL_Renderer3D* renderer);
void DUAL_Debug_Draw_Model_BoundingBox(DUAL_Renderer3D* debug, DUAL_AABB box, DUAL_Transform3D transform, DUAL_Vec3 color);



DUAL_Result DUAL_AnimatedModel_Create(DUAL_ResourceManager* resources, const char* glb_path, DUAL_Animated_Model** out_animated_model);
void DUAL_AnimatedModel_PlayAnimation(DUAL_Animated_Model* anim_model, unsigned int index);
void DUAL_AnimatedModel_Update(DUAL_Animated_Model* anim_model, float dt);
void DUAL_AnimatedModel_Destroy(DUAL_Animated_Model* anim_model);

void DUAL_Debug_Transform(DUAL_Transform3D transform);
void DUAL_Debug_Vec3(DUAL_Vec3 vector);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_GRAPHICS_3D_H */