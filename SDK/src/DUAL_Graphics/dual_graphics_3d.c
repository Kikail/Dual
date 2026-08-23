#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <stdint.h>

#include <glad/glad.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

/* Inclusion de nos modules */
#include "../../include/DUAL_Graphics/dual_graphics_3d.h"
#include "../../include/DUAL_Graphics/dual_graphics_2d.h"
#include "../../include/DUAL_Graphics/shader.h"
#include "../../include/DUAL_Math/dual_math.h"
#include "../../include/DUAL_Core/dual_core.h"
#include "../../include/DUAL_Resources/dual_resources.h"
#include "../../include/dual_utils.h"
#include "../../include/DUAL_Graphics/camera3d.h"

extern void   DUAL_Internal_GetScreenDimensions(const DUAL_App* app, int32_t* out_w, int32_t* out_h);
extern GLuint DUAL_Internal_GetTextureID(const DUAL_Texture* texture);

#define DUAL_MAX_LIGHTS_3D 4
#define MAX_BONE_INFLUENCE 4
#define MAX_BONES 100

/* ============================================================================
 * Définition des structures opaques
 * ========================================================================== */

typedef struct {
    char* name;
    int id;
    DUAL_Mat4 offset;
} BoneInfo;

struct DUAL_Mesh {
    GLuint   vao, vbo, ebo;
    uint32_t index_count;
    uint32_t material_index;
};

struct DUAL_Model {
    DUAL_Mesh* meshes;
    uint32_t   mesh_count;
    DUAL_AABB  bounding_box_local;
    DUAL_ResourceHandle* handle;

    DUAL_Material** materials;
    unsigned int material_count;

    BoneInfo* m_BoneInfoMap;
    int m_BoneCounter;
    int m_BoneMapCapacity;
};

struct DUAL_Material {
    DUAL_Texture* texture_diffuse;
    DUAL_ResourceHandle* handle;
};

struct Internal_Shaders3d_s {
    DUAL_Shader shader_lit;
    DUAL_Shader shader_unlit;
    DUAL_Shader shader_skeleton;
    DUAL_Shader shader_skeleton_unlit;
    DUAL_Shader shader_debug;
    DUAL_Shader shader_billboard;
};

struct DUAL_Renderer3D {
    DUAL_App* app;

    // On creer une configuration de shaders de base dans le renderer
    DUAL_Shader* current_shader;
    struct Internal_Shaders3d_s shaders;

    GLuint ubo_bones;

    /* État de rendu */
    DUAL_RenderMode3D render_mode;

    /* Caméra */
    DUAL_Camera3D camera;

    /* Projection */
    DUAL_ProjectionMode3D projection_mode;
    DUAL_Mat4 perspective_projection;
    DUAL_Mat4 orthographic_projection;
    float fov_radians;
    float ortho_demi_hauteur;
    float plan_proche;
    float plan_lointain;

    /* Culling */
    DUAL_CullMode cull_mode;

    /* Éclairage */
    DUAL_Vec3  ambient_light;
    DUAL_Light lights[DUAL_MAX_LIGHTS_3D];

    /* Billboard */
    GLuint billboard_vbo, billboard_vao;

    /* Debug Gizmos */
    DUAL_DebugRenderer3D debug_renderer;
};

/* --- OPTIMISATION : Réduction de la taille des vertices --- */
typedef struct {
    DUAL_Vec3 position;
    DUAL_Vec3 normale;
    DUAL_Vec2 tex_coords;
    float   m_Weights[MAX_BONE_INFLUENCE];
    int32_t m_BoneIDs[MAX_BONE_INFLUENCE]; // Utiliser int32_t pour un ivec4 parfait
} Vertex3D;

/* ============================================================================
 * UTILITAIRES ASSIMP & BONES
 * ========================================================================== */

static void SetVertexBoneDataToDefault(Vertex3D* vertex) {
    for (unsigned int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        vertex->m_BoneIDs[i] = -1;
        vertex->m_Weights[i] = 0.0f;
    }
}

static void SetVertexBoneData(Vertex3D* vertex, int boneID, float weight) {
    for (unsigned int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        if (vertex->m_BoneIDs[i] < 0) {
            vertex->m_BoneIDs[i] = boneID;
            vertex->m_Weights[i] = weight;
            break;
        }
    }
}

static bool DoBoneInfoExist(DUAL_Model* model, const char* name, int* out_id) {
    for (int i = 0; i < model->m_BoneCounter; i++) {
        if (strcmp(model->m_BoneInfoMap[i].name, name) == 0) {
            if (out_id) *out_id = model->m_BoneInfoMap[i].id;
            return true;
        }
    }
    return false;
}

static DUAL_Mat4 ConvertAssimpMatrix(const struct aiMatrix4x4* from) {
    DUAL_Mat4 to;
    to.m[0]  = from->a1; to.m[1]  = from->b1; to.m[2]  = from->c1; to.m[3]  = from->d1;
    to.m[4]  = from->a2; to.m[5]  = from->b2; to.m[6]  = from->c2; to.m[7]  = from->d2;
    to.m[8]  = from->a3; to.m[9]  = from->b3; to.m[10] = from->c3; to.m[11] = from->d3;
    to.m[12] = from->a4; to.m[13] = from->b4; to.m[14] = from->c4; to.m[15] = from->d4;
    return to;
}

static void ExtractBoneWeightForVertices(DUAL_Model* model, Vertex3D* vertices, unsigned int nb_vertices, struct aiMesh* mesh, const struct aiScene* scene, uint32_t base_vertex) {
    (void)nb_vertices;
    (void)scene;
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
        int boneID = -1;
        char* boneName = mesh->mBones[boneIndex]->mName.data;

        if (!DoBoneInfoExist(model, boneName, &boneID)) {
            if (model->m_BoneCounter >= model->m_BoneMapCapacity) {
                model->m_BoneMapCapacity *= 2;
                model->m_BoneInfoMap = realloc(model->m_BoneInfoMap, sizeof(BoneInfo) * model->m_BoneMapCapacity);
            }

            boneID = model->m_BoneCounter;
            model->m_BoneInfoMap[boneID].id = boneID;
            model->m_BoneInfoMap[boneID].name = strdup(boneName);
            model->m_BoneInfoMap[boneID].offset = ConvertAssimpMatrix(&mesh->mBones[boneIndex]->mOffsetMatrix);
            model->m_BoneCounter++;
        }

        struct aiVertexWeight* weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;
        for (int weightIndex = 0; weightIndex < numWeights; weightIndex++) {
            int vertexId = weights[weightIndex].mVertexId + base_vertex;
            float weight = weights[weightIndex].mWeight;
            SetVertexBoneData(&vertices[vertexId], boneID, weight);
        }
    }
}

/* ============================================================================
 * LOGIQUE D'ANIMATION (Conservée intacte)
 * ========================================================================== */

static int Bone_GetPositionIndex(Bone* bone, float animationTime) {
    for (int index = 0; index < bone->m_NumPositions - 1; ++index) {
        if (animationTime < bone->m_Positions[index + 1].timeStamp) return index;
    }
    return 0;
}

static int Bone_GetRotationIndex(Bone* bone, float animationTime) {
    for (int index = 0; index < bone->m_NumRotations - 1; ++index) {
        if (animationTime < bone->m_Rotations[index + 1].timeStamp) return index;
    }
    return 0;
}

static int Bone_GetScaleIndex(Bone* bone, float animationTime) {
    for (int index = 0; index < bone->m_NumScalings - 1; ++index) {
        if (animationTime < bone->m_Scales[index + 1].timeStamp) return index;
    }
    return 0;
}

static float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) {
    float midWayLength = animationTime - lastTimeStamp;
    float framesDiff = nextTimeStamp - lastTimeStamp;
    if (framesDiff <= 0.0f) return 0.0f;
    return midWayLength / framesDiff;
}

static DUAL_Mat4 Bone_InterpolatePosition(Bone* bone, float animationTime) {
    if (bone->m_NumPositions == 1) return DUAL_Mat4_Translate(bone->m_Positions[0].position);
    int p0Index = Bone_GetPositionIndex(bone, animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(bone->m_Positions[p0Index].timeStamp, bone->m_Positions[p1Index].timeStamp, animationTime);
    DUAL_Vec3 finalPos = DUAL_Vec3_Mix(bone->m_Positions[p0Index].position, bone->m_Positions[p1Index].position, scaleFactor);
    return DUAL_Mat4_Translate(finalPos);
}

static DUAL_Mat4 Bone_InterpolateRotation(Bone* bone, float animationTime) {
    if (bone->m_NumRotations == 1) return DUAL_Quat_ToMat4(DUAL_Quat_Normalize(bone->m_Rotations[0].orientation));
    int p0Index = Bone_GetRotationIndex(bone, animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(bone->m_Rotations[p0Index].timeStamp, bone->m_Rotations[p1Index].timeStamp, animationTime);
    DUAL_Quat finalRot = DUAL_Quat_Slerp(bone->m_Rotations[p0Index].orientation, bone->m_Rotations[p1Index].orientation, scaleFactor);
    return DUAL_Quat_ToMat4(DUAL_Quat_Normalize(finalRot));
}

static DUAL_Mat4 Bone_InterpolateScaling(Bone* bone, float animationTime) {
    if (bone->m_NumScalings == 1) return DUAL_Mat4_Scale(bone->m_Scales[0].scale);
    int p0Index = Bone_GetScaleIndex(bone, animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(bone->m_Scales[p0Index].timeStamp, bone->m_Scales[p1Index].timeStamp, animationTime);
    DUAL_Vec3 finalScale = DUAL_Vec3_Mix(bone->m_Scales[p0Index].scale, bone->m_Scales[p1Index].scale, scaleFactor);
    return DUAL_Mat4_Scale(finalScale);
}

void Bone_Update(Bone* bone, float animationTime) {
    DUAL_Mat4 translation = Bone_InterpolatePosition(bone, animationTime);
    DUAL_Mat4 rotation    = Bone_InterpolateRotation(bone, animationTime);
    DUAL_Mat4 scale       = Bone_InterpolateScaling(bone, animationTime);
    bone->m_LocalTransform = DUAL_Mat4_Multiply(translation, DUAL_Mat4_Multiply(rotation, scale));
}

static Bone* Animation_FindBone(Animation* animation, const char* name) {
    for (int i = 0; i < animation->m_NumBones; i++) {
        if (strcmp(animation->m_Bones[i].m_Name, name) == 0) return &animation->m_Bones[i];
    }
    return NULL;
}

static void ReadHierarchyData(AssimpNodeData* dest, const struct aiNode* src) {
    dest->name = strdup(src->mName.data);
    dest->transformation = ConvertAssimpMatrix(&src->mTransformation);
    dest->childrenCount = src->mNumChildren;

    if (dest->childrenCount > 0) {
        dest->children = malloc(sizeof(AssimpNodeData) * dest->childrenCount);
        for (int i = 0; i < dest->childrenCount; i++) {
            ReadHierarchyData(&dest->children[i], src->mChildren[i]);
        }
    } else {
        dest->children = NULL;
    }
}

DUAL_Result DUAL_Animation_Load(const char* path, DUAL_Model* model, Animation** out_animation) {
    const struct aiScene* scene = aiImportFile(path, aiProcess_Triangulate);
    if (!scene || !scene->mRootNode || scene->mNumAnimations == 0) {
        if (scene) aiReleaseImport(scene);
        return DUAL_ERROR_NOT_FOUND;
    }

    struct aiAnimation* aiAnim = scene->mAnimations[0];
    Animation* anim = malloc(sizeof(Animation));
    anim->m_Duration = (float)aiAnim->mDuration;
    anim->m_TicksPerSecond = aiAnim->mTicksPerSecond != 0 ? (int)aiAnim->mTicksPerSecond : 25;
    anim->m_LinkedModel = model;

    ReadHierarchyData(&anim->m_RootNode, scene->mRootNode);

    anim->m_NumBones = aiAnim->mNumChannels;
    anim->m_Bones = malloc(sizeof(Bone) * anim->m_NumBones);

    for (int i = 0; i < anim->m_NumBones; i++) {
        struct aiNodeAnim* channel = aiAnim->mChannels[i];
        Bone* bone = &anim->m_Bones[i];

        bone->m_Name = strdup(channel->mNodeName.data);

        int boneID = -1;
        if (!DoBoneInfoExist(model, bone->m_Name, &boneID)) {
            if (model->m_BoneCounter >= model->m_BoneMapCapacity) {
                model->m_BoneMapCapacity *= 2;
                model->m_BoneInfoMap = realloc(model->m_BoneInfoMap, sizeof(BoneInfo) * model->m_BoneMapCapacity);
            }
            boneID = model->m_BoneCounter;
            model->m_BoneInfoMap[boneID].id = boneID;
            model->m_BoneInfoMap[boneID].name = strdup(bone->m_Name);
            model->m_BoneInfoMap[boneID].offset = DUAL_Mat4_Identity();
            model->m_BoneCounter++;
        }
        bone->m_ID = boneID;
        bone->m_LocalTransform = DUAL_Mat4_Identity();

        bone->m_NumPositions = channel->mNumPositionKeys;
        bone->m_Positions = malloc(sizeof(KeyPosition) * bone->m_NumPositions);
        for(int j=0; j < bone->m_NumPositions; j++) {
            bone->m_Positions[j].position = (DUAL_Vec3){channel->mPositionKeys[j].mValue.x, channel->mPositionKeys[j].mValue.y, channel->mPositionKeys[j].mValue.z};
            bone->m_Positions[j].timeStamp = (float)channel->mPositionKeys[j].mTime;
        }

        bone->m_NumRotations = channel->mNumRotationKeys;
        bone->m_Rotations = malloc(sizeof(KeyRotation) * bone->m_NumRotations);
        for(int j=0; j < bone->m_NumRotations; j++) {
            bone->m_Rotations[j].orientation = (DUAL_Quat){channel->mRotationKeys[j].mValue.x, channel->mRotationKeys[j].mValue.y, channel->mRotationKeys[j].mValue.z, channel->mRotationKeys[j].mValue.w};
            bone->m_Rotations[j].timeStamp = (float)channel->mRotationKeys[j].mTime;
        }

        bone->m_NumScalings = channel->mNumScalingKeys;
        bone->m_Scales = malloc(sizeof(KeyScale) * bone->m_NumScalings);
        for(int j=0; j < bone->m_NumScalings; j++) {
            bone->m_Scales[j].scale = (DUAL_Vec3){channel->mScalingKeys[j].mValue.x, channel->mScalingKeys[j].mValue.y, channel->mScalingKeys[j].mValue.z};
            bone->m_Scales[j].timeStamp = (float)channel->mScalingKeys[j].mTime;
        }
    }

    aiReleaseImport(scene);
    *out_animation = anim;
    return DUAL_OK;
}

void Animator_Init(Animator* animator, Animation* animation) {
    animator->m_CurrentAnimation = animation;
    animator->m_CurrentTime = 0.0f;
    animator->m_DeltaTime = 0.0f;
    for (int i = 0; i < MAX_BONES; i++) {
        animator->m_FinalBoneMatrices[i] = DUAL_Mat4_Identity();
    }
}

static void Animator_CalculateBoneTransform(Animator* animator, const AssimpNodeData* node, DUAL_Mat4 parentTransform) {
    const char* nodeName = node->name;
    DUAL_Mat4 nodeTransform = node->transformation;
    Bone* bone = Animation_FindBone(animator->m_CurrentAnimation, nodeName);

    if (bone) {
        Bone_Update(bone, animator->m_CurrentTime);
        nodeTransform = bone->m_LocalTransform;
    }

    DUAL_Mat4 globalTransformation = DUAL_Mat4_Multiply(parentTransform, nodeTransform);

    DUAL_Model* model = animator->m_CurrentAnimation->m_LinkedModel;
    for (int i = 0; i < model->m_BoneCounter; i++) {
        if (strcmp(model->m_BoneInfoMap[i].name, nodeName) == 0) {
            int boneID = model->m_BoneInfoMap[i].id;
            if (boneID >= 0 && boneID < MAX_BONES) {
                DUAL_Mat4 offset = model->m_BoneInfoMap[i].offset;
                animator->m_FinalBoneMatrices[boneID] = DUAL_Mat4_Multiply(globalTransformation, offset);
            }
            break;
        }
    }

    for (int i = 0; i < node->childrenCount; i++) {
        Animator_CalculateBoneTransform(animator, &node->children[i], globalTransformation);
    }
}

void Animator_UpdateAnimation(Animator* animator, float dt) {
    animator->m_DeltaTime = dt;
    if (animator->m_CurrentAnimation) {
        animator->m_CurrentTime += animator->m_CurrentAnimation->m_TicksPerSecond * dt;
        animator->m_CurrentTime = fmodf(animator->m_CurrentTime, animator->m_CurrentAnimation->m_Duration);
        Animator_CalculateBoneTransform(animator, &animator->m_CurrentAnimation->m_RootNode, DUAL_Mat4_Identity());
    }
}

static GLuint CompileShader3D(GLenum type, const char* source, DUAL_Result* out_result) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("[ERREUR SHADER 3D] %s\n", infoLog);
    }
    return shader;
}

static void ApplyCullMode3D(DUAL_CullMode mode) {
    switch (mode) {
        case DUAL_CULL_BACK:  glEnable(GL_CULL_FACE); glCullFace(GL_BACK); break;
        case DUAL_CULL_FRONT: glEnable(GL_CULL_FACE); glCullFace(GL_FRONT); break;
        case DUAL_CULL_NONE:
        default:              glDisable(GL_CULL_FACE); break;
    }
}

/* ============================================================================
 * GESTION ET CHARGEMENT DES MODÈLES
 * ========================================================================== */

DUAL_Result DUAL_Model_Load(DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Model** out_model, unsigned int* out_material_count, DUAL_Material*** out_materials) {
    if (!chemin_fichier || !out_model) return DUAL_ERROR_INVALID_ARG;

    const unsigned int flags_assimp = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_PreTransformVertices;
    const struct aiScene* scene = aiImportFile(chemin_fichier, flags_assimp);

    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        if (scene) aiReleaseImport(scene);
        return DUAL_ERROR_NOT_FOUND;
    }

    DUAL_Model* model = malloc(sizeof(struct DUAL_Model));
    model->handle = NULL;
    model->mesh_count = scene->mNumMeshes;
    model->meshes = malloc(sizeof(DUAL_Mesh) * model->mesh_count);

    model->m_BoneCounter = 0;
    model->m_BoneMapCapacity = MAX_BONES;
    model->m_BoneInfoMap = malloc(sizeof(BoneInfo) * model->m_BoneMapCapacity);

    DUAL_Vec3 bb_min = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
    DUAL_Vec3 bb_max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    model->materials = calloc(scene->mNumMaterials, sizeof(DUAL_Material*));
    model->material_count = scene->mNumMaterials;

    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        const struct aiMaterial* material = scene->mMaterials[i];
        struct aiString texPath;
        char texPathBuffer[512] = {0};

        if (aiGetMaterialTexture(material, aiTextureType_DIFFUSE, 0, &texPath, NULL, NULL, NULL, NULL, NULL, NULL) == aiReturn_SUCCESS) {
            const char* last_slash = strrchr(chemin_fichier, '/');
            const char* last_backslash = strrchr(chemin_fichier, '\\');
            const char* last_sep = (last_slash > last_backslash) ? last_slash : last_backslash;

            if (last_sep != NULL) {
                size_t dir_len = (size_t)(last_sep - chemin_fichier + 1);
                if (dir_len < sizeof(texPathBuffer)) {
                    strncpy(texPathBuffer, chemin_fichier, dir_len);
                    texPathBuffer[dir_len] = '\0';
                }
            }

            strncat(texPathBuffer, texPath.data, sizeof(texPathBuffer) - strlen(texPathBuffer) - 1);

            DUAL_Texture* texture = NULL;
            if (DUAL_Texture_LoadFromFile(resources, texPathBuffer, DUAL_FILTER_NEAREST, &texture) == DUAL_OK) {
                DUAL_Material_Create(resources, texture, &model->materials[i]);
            }
        }
    }

    if (out_material_count) *out_material_count = model->material_count;
    if (out_materials) *out_materials = model->materials;

    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        const struct aiMesh* mesh = scene->mMeshes[m];

        Vertex3D* vertices = malloc(sizeof(Vertex3D) * mesh->mNumVertices);
        uint32_t num_indices = mesh->mNumFaces * 3;
        uint32_t* indices = malloc(sizeof(uint32_t) * num_indices);

        for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
            SetVertexBoneDataToDefault(&vertices[v]);
            DUAL_Vec3 pos = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };

            DUAL_Vec3 normale = { 0.0f, 1.0f, 0.0f };
            if (mesh->mNormals) {
                normale = (DUAL_Vec3){ mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z };
            }

            DUAL_Vec2 uv = { 0.0f, 0.0f };
            if (mesh->mTextureCoords[0]) {
                uv = (DUAL_Vec2){ mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y };
            }

            vertices[v].position   = pos;
            vertices[v].normale    = normale;
            vertices[v].tex_coords = uv;

            if (pos.x < bb_min.x) bb_min.x = pos.x; if (pos.x > bb_max.x) bb_max.x = pos.x;
            if (pos.y < bb_min.y) bb_min.y = pos.y; if (pos.y > bb_max.y) bb_max.y = pos.y;
            if (pos.z < bb_min.z) bb_min.z = pos.z; if (pos.z > bb_max.z) bb_max.z = pos.z;

            float total_w = vertices[v].m_Weights[0] + vertices[v].m_Weights[1] + vertices[v].m_Weights[2] + vertices[v].m_Weights[3];
            if (total_w > 0.0f) {
                vertices[v].m_Weights[0] /= total_w;
                vertices[v].m_Weights[1] /= total_w;
                vertices[v].m_Weights[2] /= total_w;
                vertices[v].m_Weights[3] /= total_w;
            }
        }

        uint32_t index_cursor = 0;
        for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
            for (unsigned int k = 0; k < mesh->mFaces[f].mNumIndices; k++) {
                indices[index_cursor++] = mesh->mFaces[f].mIndices[k];
            }
        }

        ExtractBoneWeightForVertices(model, vertices, mesh->mNumVertices, (struct aiMesh*)mesh, scene, 0);

        model->meshes[m].index_count = index_cursor;
        model->meshes[m].material_index = mesh->mMaterialIndex;

        glGenVertexArrays(1, &model->meshes[m].vao);
        glGenBuffers(1, &model->meshes[m].vbo);
        glGenBuffers(1, &model->meshes[m].ebo);

        glBindVertexArray(model->meshes[m].vao);

        glBindBuffer(GL_ARRAY_BUFFER, model->meshes[m].vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex3D) * mesh->mNumVertices, vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->meshes[m].ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * index_cursor, indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, position)); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, normale)); glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, tex_coords)); glEnableVertexAttribArray(2);

        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, m_Weights)); glEnableVertexAttribArray(3);
        glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex3D), (void*)offsetof(Vertex3D, m_BoneIDs));glEnableVertexAttribArray(4);

        glBindVertexArray(0);

        free(vertices);
        free(indices);
    }

    aiReleaseImport(scene);
    model->bounding_box_local.min = bb_min;
    model->bounding_box_local.max = bb_max;

    *out_model = model;
    return DUAL_OK;
}

void DUAL_Model_Destroy(DUAL_ResourceManager* resources, DUAL_Model* model) {
    (void)resources;
    if (!model) return;
    if (model->meshes) {
        for (uint32_t i = 0; i < model->mesh_count; i++) {
            glDeleteVertexArrays(1, &model->meshes[i].vao);
            glDeleteBuffers(1, &model->meshes[i].vbo);
            glDeleteBuffers(1, &model->meshes[i].ebo);
        }
        free(model->meshes);
    }
    if (model->m_BoneInfoMap) {
        for (int i = 0; i < model->m_BoneCounter; i++) {
            free(model->m_BoneInfoMap[i].name);
        }
        free(model->m_BoneInfoMap);
    }
    free(model->materials);
    free(model);
}

DUAL_AABB DUAL_Model_GetBoundingBox(const DUAL_Model* model) {
    if (!model) return (DUAL_AABB){{0,0,0}, {0,0,0}};
    return model->bounding_box_local;
}

uint32_t DUAL_Model_GetMeshCount(const DUAL_Model* model) {
    if (!model) return 0;
    return model->mesh_count;
}

/* ============================================================================
 * GESTION DES MATÉRIAUX
 * ========================================================================== */

DUAL_Result DUAL_Material_Create(DUAL_ResourceManager* resources, DUAL_Texture* texture_diffuse, DUAL_Material** out_material) {
    (void)resources;
    if (!out_material) return DUAL_ERROR_INVALID_ARG;
    DUAL_Material* mat = malloc(sizeof(struct DUAL_Material));
    if (!mat) return DUAL_ERROR_OUT_OF_MEMORY;
    mat->texture_diffuse = texture_diffuse;
    mat->handle = NULL;
    *out_material = mat;
    return DUAL_OK;
}

void DUAL_Material_Destroy(DUAL_ResourceManager* resources, DUAL_Material* material) {
    (void)resources;
    if (material) free(material);
}

DUAL_Result DUAL_Renderer3D_Create(DUAL_App* app, DUAL_Renderer3D** out_renderer) {
    if (!app || !out_renderer) return DUAL_ERROR_INVALID_ARG;
    DUAL_Renderer3D* renderer = malloc(sizeof(struct DUAL_Renderer3D));
    memset(renderer, 0, sizeof(struct DUAL_Renderer3D));
    renderer->app = app;

    bool compilation_result = true;
    compilation_result = DUAL_Shader_load_VS_FS(&renderer->shaders.shader_lit, INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_lit_base.vs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_lit_base.fs", DUAL_SHADER_LIT);
    compilation_result = DUAL_Shader_load_VS_FS(&renderer->shaders.shader_unlit, INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_unlit_base.vs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_unlit_base.fs", DUAL_SHADER_UNLIT);
    compilation_result = DUAL_Shader_load_VS_FS(&renderer->shaders.shader_skeleton, INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_skeletal_base.vs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_lit_base.fs", DUAL_SHADER_LIT);
    compilation_result = DUAL_Shader_load_VS_FS(&renderer->shaders.shader_skeleton_unlit, INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_skeletal_base.vs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_unlit_base.fs", DUAL_SHADER_UNLIT);
    compilation_result = DUAL_Shader_load_VS_FS(&renderer->shaders.shader_debug, INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_debug_base.vs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_debug_base.fs", DUAL_SHADER_UNLIT);
    compilation_result = DUAL_Shader_load_VS_GEO_FS(&renderer->shaders.shader_billboard, INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_billboard.vs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_billboard.gs", INTERNAL_RESOURCES_SHADERS_PATH "/i_shader_3d_billboard.fs", DUAL_SHADER_UNLIT);

    renderer->render_mode = DUAL_RENDER_LIT;
    renderer->camera = DUAL_Camera3D_Create((DUAL_Vec3){0.0f, 0.0f, 0.0f}, DUAL_VECTOR_UP, DUAL_VECTOR_FRONT, DUAL_CAMERA_YAW, DUAL_CAMERA_PITCH, DUAL_CAMERA_SPEED, DUAL_CAMERA_SENSIVITY, DUAL_CAMERA_ZOOM);
    renderer->projection_mode = DUAL_PROJECTION_PERSPECTIVE;

    renderer->fov_radians = 1.0471975512f;
    renderer->ortho_demi_hauteur = 5.0f;
    renderer->plan_proche = 0.1f;
    renderer->plan_lointain = 100.0f;

    int32_t w = 0, h = 0;
    DUAL_Internal_GetScreenDimensions(renderer->app, &w, &h);
    float aspect = (h != 0) ? ((float)w / (float)h) : 1.0f;
    float demi_h = (renderer->ortho_demi_hauteur > 0.0f) ? renderer->ortho_demi_hauteur : 5.0f;
    float demi_w = demi_h * aspect;
    renderer->orthographic_projection = DUAL_Mat4_Ortho(-demi_w, demi_w, -demi_h, demi_h, renderer->plan_proche, renderer->plan_lointain);
    renderer->perspective_projection = DUAL_Mat4_Perspective(renderer->fov_radians, aspect, renderer->plan_proche, renderer->plan_lointain);

    renderer->cull_mode = DUAL_CULL_BACK;


    // On creer le VBO et VAO pour le billboard
    DUAL_Vec3 billboard_point = {0.0f, 0.0f, 0.0f};
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

    DUAL_Renderer3D_UseShader(renderer, SHADER3D_SHADER_DEBUG);
    DUAL_Shader_setVec3(renderer->current_shader, "uColor", (DUAL_Vec3){1.0f, 1.0f, 1.0f});
    DUAL_Renderer3D_UseShader(renderer, SHADER3D_LIT);

    *out_renderer = renderer;
    return DUAL_OK;
}

void DUAL_Renderer3D_Destroy(DUAL_Renderer3D* renderer) {
    if (!renderer) return;
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

/* ============================================================================
 * PHASES DE RENDU
 * ========================================================================== */

void DUAL_Renderer3D_Begin(DUAL_Renderer3D* renderer) {
    if (!renderer) return;

    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    ApplyCullMode3D(renderer->cull_mode);
    glPolygonMode(GL_FRONT_AND_BACK, (renderer->render_mode == DUAL_RENDER_WIREFRAME) ? GL_LINE : GL_FILL);

    int32_t w = 0, h = 0;
    DUAL_Internal_GetScreenDimensions(renderer->app, &w, &h);
    float aspect = (h != 0) ? ((float)w / (float)h) : 1.0f;

    DUAL_Mat4 proj;
    if (renderer->projection_mode == DUAL_PROJECTION_ORTHOGRAPHIC) {
        proj = renderer->orthographic_projection;
    } else {
        proj = renderer->perspective_projection;
    }
}

void DUAL_Renderer3D_End(DUAL_Renderer3D* renderer) {
    if (!renderer) return;

    if (renderer->debug_renderer.count > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, renderer->debug_renderer.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(LineVertex) * renderer->debug_renderer.count, renderer->debug_renderer.vertices);

        DUAL_Shader* previous_shader = renderer->current_shader;

        DUAL_Renderer3D_UseShader(renderer, SHADER3D_SHADER_DEBUG);
        DUAL_Shader_use(renderer->current_shader);

        DUAL_Shader_setMat4(renderer->current_shader, "uModel", DUAL_Mat4_Identity());
        DUAL_Shader_setMat4(renderer->current_shader, "uView", DUAL_Camera3D_GetViewMatrix(&renderer->camera));
        DUAL_Shader_setMat4(renderer->current_shader, "uProjection", renderer->perspective_projection);

        glBindVertexArray(renderer->debug_renderer.vao);
        glDrawArrays(GL_LINES, 0, (GLsizei)renderer->debug_renderer.count);
        glBindVertexArray(0);

        renderer->current_shader = previous_shader;
        DUAL_Shader_use(renderer->current_shader);

        renderer->debug_renderer.count = 0;
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

/* ============================================================================
 * FONCTIONS DE DESSIN
 * ========================================================================== */

void DUAL_DrawAnimatedModel(DUAL_Renderer3D* renderer, const DUAL_Model* model, const DUAL_Material** materials, DUAL_Transform3D transform, Animator* animator) {
    if (!renderer || !model) return;

    if (!materials) materials = (const DUAL_Material**)model->materials;

    DUAL_Shader_use(renderer->current_shader);

    DUAL_Mat4 scale   = DUAL_Mat4_Scale(transform.echelle);
    DUAL_Mat4 rot_x   = DUAL_Mat4_Rotate((DUAL_Vec3){ 1.0f, 0.0f, 0.0f }, transform.rotation_euler_radians.x);
    DUAL_Mat4 rot_y   = DUAL_Mat4_Rotate((DUAL_Vec3){ 0.0f, 1.0f, 0.0f }, transform.rotation_euler_radians.y);
    DUAL_Mat4 rot_z   = DUAL_Mat4_Rotate((DUAL_Vec3){ 0.0f, 0.0f, 1.0f }, transform.rotation_euler_radians.z);
    DUAL_Mat4 trans   = DUAL_Mat4_Translate(transform.position);
    DUAL_Mat4 modele  = DUAL_Mat4_Multiply(trans, DUAL_Mat4_Multiply(DUAL_Mat4_Multiply(rot_z, DUAL_Mat4_Multiply(rot_y, rot_x)), scale));

    // On envoit les matrices au shader
    DUAL_Shader_setMat4(renderer->current_shader, "uModel", modele);
    DUAL_Shader_setMat4(renderer->current_shader, "uView", DUAL_Camera3D_GetViewMatrix(&renderer->camera));
    if (renderer->projection_mode == DUAL_PROJECTION_PERSPECTIVE) {
        DUAL_Shader_setMat4(renderer->current_shader, "uProjection", renderer->perspective_projection);
    }
    else {
        DUAL_Shader_setMat4(renderer->current_shader, "uProjection", renderer->orthographic_projection);
    }

    if (animator) {
        DUAL_Shader_setMat4Array(renderer->current_shader, "uFinalBonesMatrices", MAX_BONES, animator->m_FinalBoneMatrices);
    }

    if (renderer->current_shader->renderMode == DUAL_SHADER_LIT) {
        DUAL_Renderer3d_SendLightInfosToShader(renderer, renderer->current_shader);
    }

    for (uint32_t i = 0; i < model->mesh_count; i++) {
        const DUAL_Material* material = NULL;
        uint32_t mat_idx = model->meshes[i].material_index;

        if (materials && mat_idx < model->material_count) {
            material = materials[mat_idx];
        }

        if (material && material->texture_diffuse) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, DUAL_Internal_GetTextureID(material->texture_diffuse));
            glUniform1i(glGetUniformLocation(renderer->current_shader->shaderID, "texture_diffuse"), 0);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glBindVertexArray(model->meshes[i].vao);
        glDrawElements(GL_TRIANGLES, (GLsizei)model->meshes[i].index_count, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

void DUAL_DrawBillboard(DUAL_Renderer3D* renderer, const DUAL_Texture* texture, DUAL_Transform3D transform) {
    if (!renderer || !texture) return;

    DUAL_Shader* previous_shader = renderer->current_shader;

    renderer->current_shader = &renderer->shaders.shader_billboard;
    DUAL_Shader_use(renderer->current_shader);

    // Désactivation du Cull Face pour éviter que le Quad ne soit ignoré
    glDisable(GL_CULL_FACE);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, DUAL_Internal_GetTextureID(texture));
    glUniform1i(glGetUniformLocation(renderer->current_shader->shaderID, "texture_diffuse"), 0);

    DUAL_Mat4 vp = DUAL_Mat4_Multiply(renderer->perspective_projection, DUAL_Camera3D_GetViewMatrix(&renderer->camera));
    DUAL_Shader_setMat4(renderer->current_shader, "uVP", vp);

    DUAL_Shader_setVec3(renderer->current_shader, "uCameraPos", renderer->camera.position);

    DUAL_Vec2 size = { transform.echelle.x, transform.echelle.y };
    DUAL_Shader_setVec2(renderer->current_shader, "uSize", size);

    DUAL_Shader_setVec3(renderer->current_shader, "uPosition", transform.position);

    glBindVertexArray(renderer->billboard_vao);
    glDrawArrays(GL_POINTS, 0, 1);
    glBindVertexArray(0);

    // Réactivation du Culling selon le mode global
    ApplyCullMode3D(renderer->cull_mode);

    renderer->current_shader = previous_shader;
    if (previous_shader) {
        DUAL_Shader_use(previous_shader);
    }
}

void DUAL_Renderer3d_SendLightInfosToShader(DUAL_Renderer3D* renderer, DUAL_Shader* shader) {
    DUAL_Shader_setVec3(shader, "uAmbientLight", renderer->ambient_light);
    char uname[64];
    for (int i = 0; i < DUAL_MAX_LIGHTS_3D; i++) {
        sprintf(uname, "uLights[%d].type", i);      DUAL_Shader_setInt(shader, uname, renderer->lights[i].type);
        sprintf(uname, "uLights[%d].position", i);  DUAL_Shader_setVec3(shader, uname, renderer->lights[i].position);
        sprintf(uname, "uLights[%d].direction", i); DUAL_Shader_setVec3(shader, uname, renderer->lights[i].direction);
        sprintf(uname, "uLights[%d].color", i);     DUAL_Shader_setVec3(shader, uname, renderer->lights[i].couleur);
        sprintf(uname, "uLights[%d].intensity", i); DUAL_Shader_setFloat(shader, uname, renderer->lights[i].intensite);
        sprintf(uname, "uLights[%d].range", i);     DUAL_Shader_setFloat(shader, uname, renderer->lights[i].portee);
    }
}

void DUAL_DrawModel(DUAL_Renderer3D* renderer, const DUAL_Model* model, const DUAL_Material** materials, DUAL_Transform3D transform) {
    DUAL_DrawAnimatedModel(renderer, model, materials, transform, NULL);
}

void DUAL_Renderer3D_UseShader(DUAL_Renderer3D* renderer, DUAL_Renderer3d_Shaders shader) {
    if (!renderer) return;
    switch (shader) {
        case SHADER3D_LIT:
            renderer->current_shader = &renderer->shaders.shader_lit;
            break;
        case SHADER3D_UNLIT:
            renderer->current_shader = &renderer->shaders.shader_unlit;
            break;
        case SHADER3D_SKELETAL_LIT:
            renderer->current_shader = &renderer->shaders.shader_skeleton;
            break;
        case SHADER3D_SKELETAL_UNLIT:
            renderer->current_shader = &renderer->shaders.shader_skeleton_unlit;
            break;
        case SHADER3D_SHADER_DEBUG:
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
    if (debug->debug_renderer.count + 2 > debug->debug_renderer.capacity) return; // Sécurité realloc

    debug->debug_renderer.vertices[debug->debug_renderer.count++] = (LineVertex){ start, color };
    debug->debug_renderer.vertices[debug->debug_renderer.count++] = (LineVertex){ end, color };
}

void DUAL_Debug_DrawCircle(DUAL_Renderer3D* debug, DUAL_Vec3 center, float radius, int segments, DUAL_Vec3 color) {
    float step = (2.0f * 3.14159265f) / (float)segments;

    for (int i = 0; i < segments; i++) {
        float angle1 = i * step;
        float angle2 = (i + 1) * step;

        // Cercle dans le plan XZ (au sol)
        DUAL_Vec3 p1 = { center.x + cosf(angle1) * radius, center.y, center.z + sinf(angle1) * radius };
        DUAL_Vec3 p2 = { center.x + cosf(angle2) * radius, center.y, center.z + sinf(angle2) * radius };

        DUAL_Debug_DrawLine(debug, p1, p2, color);
    }
}

void DUAL_Debug_DrawAABB(DUAL_Renderer3D* debug, DUAL_AABB box, DUAL_Vec3 color) {
    DUAL_Vec3 min = box.min;
    DUAL_Vec3 max = box.max;

    // Les 8 coins du cube
    DUAL_Vec3 c1 = { min.x, min.y, min.z }, c2 = { max.x, min.y, min.z };
    DUAL_Vec3 c3 = { max.x, min.y, max.z }, c4 = { min.x, min.y, max.z };
    DUAL_Vec3 c5 = { min.x, max.y, min.z }, c6 = { max.x, max.y, min.z };
    DUAL_Vec3 c7 = { max.x, max.y, max.z }, c8 = { min.x, max.y, max.z };

    // Bas
    DUAL_Debug_DrawLine(debug, c1, c2, color); DUAL_Debug_DrawLine(debug, c2, c3, color);
    DUAL_Debug_DrawLine(debug, c3, c4, color); DUAL_Debug_DrawLine(debug, c4, c1, color);
    // Haut
    DUAL_Debug_DrawLine(debug, c5, c6, color); DUAL_Debug_DrawLine(debug, c6, c7, color);
    DUAL_Debug_DrawLine(debug, c7, c8, color); DUAL_Debug_DrawLine(debug, c8, c5, color);
    // Piliers
    DUAL_Debug_DrawLine(debug, c1, c5, color); DUAL_Debug_DrawLine(debug, c2, c6, color);
    DUAL_Debug_DrawLine(debug, c3, c7, color); DUAL_Debug_DrawLine(debug, c4, c8, color);
}

void DUAL_Debug_Render(DUAL_Renderer3D* debug, DUAL_Renderer3D* renderer) {
    if (debug->debug_renderer.count == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, debug->debug_renderer.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(LineVertex) * debug->debug_renderer.count, debug->debug_renderer.vertices);

    DUAL_Shader* current_shader = renderer->current_shader;

    DUAL_Renderer3D_UseShader(renderer, SHADER3D_SHADER_DEBUG);
    DUAL_Shader_use(renderer->current_shader);
    DUAL_Shader_setMat4(renderer->current_shader, "uModel", DUAL_Mat4_Identity());
    DUAL_Shader_setMat4(renderer->current_shader, "uView", DUAL_Camera3D_GetViewMatrix(&renderer->camera));
    DUAL_Shader_setMat4(renderer->current_shader, "uProjection", renderer->perspective_projection);

    glBindVertexArray(debug->debug_renderer.vao);
    glDrawArrays(GL_LINES, 0, debug->debug_renderer.count);
    glBindVertexArray(0);

    debug->debug_renderer.count = 0;

    DUAL_Renderer3D_UseCustomShader(renderer, current_shader);
}

DUAL_Vec3 DUAL_Mat4_TransformPoint(DUAL_Mat4 m, DUAL_Vec3 v) {
    DUAL_Vec3 res;
    res.x = m.m[0]*v.x + m.m[4]*v.y + m.m[8]*v.z  + m.m[12];
    res.y = m.m[1]*v.x + m.m[5]*v.y + m.m[9]*v.z  + m.m[13];
    res.z = m.m[2]*v.x + m.m[6]*v.y + m.m[10]*v.z + m.m[14];
    return res;
}
void DUAL_Debug_Draw_Model_BoundingBox(DUAL_Renderer3D* debug, DUAL_AABB box, DUAL_Transform3D transform, DUAL_Vec3 color) {
    DUAL_Mat4 scale   = DUAL_Mat4_Scale(transform.echelle);
    DUAL_Mat4 rot_x   = DUAL_Mat4_Rotate((DUAL_Vec3){ 1.0f, 0.0f, 0.0f }, transform.rotation_euler_radians.x);
    DUAL_Mat4 rot_y   = DUAL_Mat4_Rotate((DUAL_Vec3){ 0.0f, 1.0f, 0.0f }, transform.rotation_euler_radians.y);
    DUAL_Mat4 rot_z   = DUAL_Mat4_Rotate((DUAL_Vec3){ 0.0f, 0.0f, 1.0f }, transform.rotation_euler_radians.z);
    DUAL_Mat4 trans   = DUAL_Mat4_Translate(transform.position);
    DUAL_Mat4 modelMat = DUAL_Mat4_Multiply(trans, DUAL_Mat4_Multiply(DUAL_Mat4_Multiply(rot_z, DUAL_Mat4_Multiply(rot_y, rot_x)), scale));

    DUAL_Vec3 min = box.min;
    DUAL_Vec3 max = box.max;
    DUAL_Vec3 corners[8] = {
        { min.x, min.y, min.z }, { max.x, min.y, min.z },
        { max.x, min.y, max.z }, { min.x, min.y, max.z },
        { min.x, max.y, min.z }, { max.x, max.y, min.z },
        { max.x, max.y, max.z }, { min.x, max.y, max.z }
    };

    for (int i = 0; i < 8; i++) {
        corners[i] = DUAL_Mat4_TransformPoint(modelMat, corners[i]);
    }

    // (Bas)
    DUAL_Debug_DrawLine(debug, corners[0], corners[1], color);
    DUAL_Debug_DrawLine(debug, corners[1], corners[2], color);
    DUAL_Debug_DrawLine(debug, corners[2], corners[3], color);
    DUAL_Debug_DrawLine(debug, corners[3], corners[0], color);
    // (Haut)
    DUAL_Debug_DrawLine(debug, corners[4], corners[5], color);
    DUAL_Debug_DrawLine(debug, corners[5], corners[6], color);
    DUAL_Debug_DrawLine(debug, corners[6], corners[7], color);
    DUAL_Debug_DrawLine(debug, corners[7], corners[4], color);
    // (Piliers)
    DUAL_Debug_DrawLine(debug, corners[0], corners[4], color);
    DUAL_Debug_DrawLine(debug, corners[1], corners[5], color);
    DUAL_Debug_DrawLine(debug, corners[2], corners[6], color);
    DUAL_Debug_DrawLine(debug, corners[3], corners[7], color);
}