//
// Created by killian on 11/3/26.
//

#ifndef DUAL_DUAL_MODEL_H
#define DUAL_DUAL_MODEL_H
#include <stdint.h>

#include "dual_graphics_3d.h"
#include "DUAL_Graphics/2d/dual_texture.h"
#include "DUAL_Math/dual_math.h"
#include "hashtable/hashtable.h"

#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

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

typedef struct Vertex3D{
    DUAL_Vec3 position;
    DUAL_Vec3 normale;
    DUAL_Vec2 tex_coords;
    float   m_Weights[MAX_BONE_INFLUENCE];
    int32_t m_BoneIDs[MAX_BONE_INFLUENCE];
} Vertex3D;

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

typedef struct DUAL_Animated_Model{
    DUAL_Model* model;
    Hashtable* animations;
    unsigned int animation_count;
    Animator animator;

    char model_name[32];
    char model_path[128];
}DUAL_Animated_Model;

DUAL_Result DUAL_Model_Load(DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Model** out_model, unsigned int* out_material_count, DUAL_Material*** out_materials);
void DUAL_Model_Destroy(void* model);
DUAL_AABB DUAL_Model_GetBoundingBox(const DUAL_Model* model);
uint32_t DUAL_Model_GetMeshCount(const DUAL_Model* model);
DUAL_Result DUAL_Material_Create(DUAL_ResourceManager* resources, DUAL_Texture* texture_diffuse, DUAL_Material** out_material);
void DUAL_Material_Destroy(DUAL_Material* material);
DUAL_Result DUAL_Animation_Load(const char* path, DUAL_Model* model, Animation** out_animation);
void Animator_Init(Animator* animator, Animation* animation);
void Animator_UpdateAnimation(Animator* animator, float dt);
void Bone_Update(Bone* bone, float animationTime);
void DUAL_DrawModel(DUAL_Renderer3D* renderer, const DUAL_Model* model, const DUAL_Material** materials, DUAL_Transform3D transform);
void DUAL_DrawAnimatedModel(DUAL_Renderer3D* renderer, const DUAL_Model* model, const DUAL_Material** materials, DUAL_Transform3D transform, Animator* animator);
DUAL_Result DUAL_AnimatedModel_Create(DUAL_ResourceManager* resources, const char* glb_path, DUAL_Animated_Model** out_animated_model);
void DUAL_AnimatedModel_PlayAnimation(DUAL_Animated_Model* anim_model, const char* anim_name);
void DUAL_AnimatedModel_Update(DUAL_Animated_Model* anim_model, float dt);
void DUAL_AnimatedModel_Destroy(void* anim_model);

#endif //DUAL_DUAL_MODEL_H
