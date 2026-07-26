#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include <glad/glad.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

/* Inclusion de nos modules */
#include "dual_graphics_3d.h"
#include "dual_graphics_2d.h"
#include "dual_math.h"
#include "dual_core.h"
#include "dual_resources.h"

extern void   DUAL_Internal_GetScreenDimensions(const DUAL_App* app, int32_t* out_w, int32_t* out_h);
extern GLuint DUAL_Internal_GetTextureID(const DUAL_Texture* texture);

#define DUAL_MAX_LIGHTS_3D 4
#define MAX_BONE_INFLUENCE 4

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
    float         brillance;
    DUAL_ResourceHandle* handle;
};

struct DUAL_Renderer3D {
    DUAL_App* app;

    /* Shaders */
    GLuint shader_lit;
    GLuint shader_unlit;
    GLuint shader_skeleton;
    GLuint shader_skeleton_unlit;

    /* État de rendu */
    DUAL_RenderMode3D render_mode;

    /* Caméra */
    DUAL_Mat4 view;
    DUAL_Vec3 camera_position;

    /* Projection */
    DUAL_ProjectionMode3D projection_mode;
    float fov_radians;
    float ortho_demi_hauteur;
    float plan_proche;
    float plan_lointain;

    /* Culling */
    DUAL_CullMode cull_mode;

    /* Éclairage */
    DUAL_Vec3  ambient_light;
    DUAL_Light lights[DUAL_MAX_LIGHTS_3D];
};

typedef struct {
    DUAL_Vec3 position;
    DUAL_Vec3 normale;
    DUAL_Vec2 tex_coords;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
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
 * LOGIQUE D'ANIMATION
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

        // POSITIONS
        bone->m_NumPositions = channel->mNumPositionKeys;
        bone->m_Positions = malloc(sizeof(KeyPosition) * bone->m_NumPositions);
        for(int j=0; j < bone->m_NumPositions; j++) {
            bone->m_Positions[j].position = (DUAL_Vec3){channel->mPositionKeys[j].mValue.x, channel->mPositionKeys[j].mValue.y, channel->mPositionKeys[j].mValue.z};
            bone->m_Positions[j].timeStamp = (float)channel->mPositionKeys[j].mTime;
        }

        // ROTATIONS
        bone->m_NumRotations = channel->mNumRotationKeys;
        bone->m_Rotations = malloc(sizeof(KeyRotation) * bone->m_NumRotations);
        for(int j=0; j < bone->m_NumRotations; j++) {
            bone->m_Rotations[j].orientation = (DUAL_Quat){channel->mRotationKeys[j].mValue.x, channel->mRotationKeys[j].mValue.y, channel->mRotationKeys[j].mValue.z, channel->mRotationKeys[j].mValue.w};
            bone->m_Rotations[j].timeStamp = (float)channel->mRotationKeys[j].mTime;
        }

        // SCALES
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
            // FIX 5: Vérifier que boneID ne dépasse pas MAX_BONES (100)
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

/* ============================================================================
 * SHADERS 3D
 * ========================================================================== */

static const char* vertex_shader_unlit_src =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"
    "layout (location = 2) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "uniform mat4 uProjection;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uModel;\n"
    "void main() {\n"
    "   gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);\n"
    "   TexCoord = aTexCoord;\n"
    "}\n";

static const char* fragment_shader_unlit_src =
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTexture;\n"
    "void main() {\n"
    "   FragColor = texture(uTexture, TexCoord);\n"
    "}\n";

static const char* vertex_shader_lit_src =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"
    "layout (location = 2) in vec2 aTexCoord;\n"
    "out vec3 FragPos;\n"
    "out vec3 Normal;\n"
    "out vec2 TexCoord;\n"
    "uniform mat4 uProjection;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uModel;\n"
    "void main() {\n"
    "   FragPos = vec3(uModel * vec4(aPos, 1.0));\n"
    "   Normal = mat3(transpose(inverse(uModel))) * aNormal;\n"
    "   TexCoord = aTexCoord;\n"
    "   gl_Position = uProjection * uView * vec4(FragPos, 1.0);\n"
    "}\n";

static const char* vertex_shader_skeleton_lit_src =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec3 aNormal;\n"
    "layout(location = 2) in vec2 aTexCoord;\n"
    "layout(location = 3) in ivec4 aBoneIds;\n"
    "layout(location = 4) in vec4 aWeights;\n"
    "uniform mat4 uProjection;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uModel;\n"
    "const int MAX_BONES = 100;\n"
    "const int MAX_BONE_INFLUENCE = 4;\n"
    "uniform mat4 finalBonesMatrices[MAX_BONES];\n"
    "out vec3 FragPos;\n"
    "out vec3 Normal;\n"
    "out vec2 TexCoord;\n"
    "void main()\n"
    "{\n"
    "    vec4 totalPosition = vec4(0.0);\n"
    "    vec3 totalNormal = vec3(0.0);\n"
    "    int bonesApplied = 0;\n"
    "    for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)\n"
    "    {\n"
    "        if(aBoneIds[i] == -1)\n"
    "            continue;\n"
    "        if(aBoneIds[i] >= MAX_BONES)\n"
    "        {\n"
    "            totalPosition = vec4(aPos, 1.0);\n"
    "            totalNormal = aNormal;\n"
    "            bonesApplied++;\n"
    "            break;\n"
    "        }\n"
    "        vec4 localPosition = finalBonesMatrices[aBoneIds[i]] * vec4(aPos, 1.0);\n"
    "        totalPosition += localPosition * aWeights[i];\n"
    "        vec3 localNormal = mat3(finalBonesMatrices[aBoneIds[i]]) * aNormal;\n"
    "        totalNormal += localNormal * aWeights[i];\n"
    "        bonesApplied++;\n"
    "    }\n"
    "    if (bonesApplied == 0 || totalPosition.w == 0.0) {\n"
    "        totalPosition = vec4(aPos, 1.0);\n"
    "        totalNormal = aNormal;\n"
    "    }\n"
    "    FragPos = vec3(uModel * totalPosition);\n"
    "    Normal = mat3(transpose(inverse(uModel))) * totalNormal;\n"
    "    TexCoord = aTexCoord;\n"
    "    gl_Position = uProjection * uView * vec4(FragPos, 1.0);\n"
    "}\n";

static const char* fragment_shader_lit_src =
    "#version 330 core\n"
    "in vec3 FragPos;\n"
    "in vec3 Normal;\n"
    "in vec2 TexCoord;\n"
    "out vec4 FragColor;\n"
    "struct Material {\n"
    "    sampler2D texture_diffuse;\n"
    "    float shininess;\n"
    "};\n"
    "struct Light {\n"
    "    int type;\n"
    "    vec3 position;\n"
    "    vec3 direction;\n"
    "    vec3 color;\n"
    "    float intensity;\n"
    "    float range;\n"
    "};\n"
    "#define MAX_LIGHTS 4\n"
    "uniform Material uMaterial;\n"
    "uniform Light uLights[MAX_LIGHTS];\n"
    "uniform vec3 uAmbientLight;\n"
    "uniform vec3 uViewPos;\n"
    "void main() {\n"
    "   vec4 texColor = texture(uMaterial.texture_diffuse, TexCoord);\n"
    "   vec3 ambient = uAmbientLight * texColor.rgb;\n"
    "   vec3 norm = normalize(Normal);\n"
    "   vec3 viewDir = normalize(uViewPos - FragPos);\n"
    "   vec3 diffuseAccum = vec3(0.0);\n"
    "   vec3 specularAccum = vec3(0.0);\n"
    "   for (int i = 0; i < MAX_LIGHTS; ++i) {\n"
    "       if (uLights[i].intensity <= 0.0) continue;\n"
    "       vec3 lightDir;\n"
    "       float attenuation = 1.0;\n"
    "       if (uLights[i].type == 0) {\n"
    "           lightDir = normalize(-uLights[i].direction);\n"
    "       } else {\n"
    "           vec3 lightVec = uLights[i].position - FragPos;\n"
    "           float distance = length(lightVec);\n"
    "           lightDir = normalize(lightVec);\n"
    "           if (uLights[i].range > 0.0) {\n"
    "               float ratio = distance / uLights[i].range;\n"
    "               attenuation = 1.0 / (1.0 + 2.0 * ratio + ratio * ratio);\n"
    "               if (distance > uLights[i].range) attenuation = 0.0;\n"
    "           }\n"
    "       }\n"
    "       float diff = max(dot(norm, lightDir), 0.0);\n"
    "       diffuseAccum += uLights[i].color * uLights[i].intensity * diff * texColor.rgb * attenuation;\n"
    "       vec3 reflectDir = reflect(-lightDir, norm);\n"
    "       float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shininess);\n"
    "       specularAccum += uLights[i].color * uLights[i].intensity * spec * attenuation;\n"
    "   }\n"
    "   vec3 result = ambient + diffuseAccum + specularAccum;\n"
    "   FragColor = vec4(result, texColor.a);\n"
    "}\n";

static GLuint CompileShader3D(GLenum type, const char* source) {
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
 * GESTION ET CHARGEMENT DES MODÈLES (MAINTENANT MESH PAR MESH)
 * ========================================================================== */

DUAL_Result DUAL_Model_Load(DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Model** out_model, unsigned int* out_material_count, DUAL_Material*** out_materials) {
    if (!chemin_fichier || !out_model) return DUAL_ERROR_INVALID_ARG;

    const unsigned int flags_assimp = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_FlipUVs;
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

    // FIX 1: Utilisation de calloc au lieu de malloc pour initialiser tous les pointeurs à NULL
    model->materials = calloc(scene->mNumMaterials, sizeof(DUAL_Material*));
    model->material_count = scene->mNumMaterials;

    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        const struct aiMaterial* material = scene->mMaterials[i];

        struct aiString matName;
        if (aiGetMaterialString(material, AI_MATKEY_NAME, &matName) == aiReturn_SUCCESS) {
            DUAL_Log(DUAL_LOG_INFO, "Matériau [%u] : %s", i, matName.data);
        }

        struct aiString texPath;
        char texPathBuffer[512] = {0};

        if (aiGetMaterialTexture(material, aiTextureType_DIFFUSE, 0, &texPath, NULL, NULL, NULL, NULL, NULL, NULL) == aiReturn_SUCCESS) {
            DUAL_Log(DUAL_LOG_INFO, "  -> Texture diffuse trouvée : %s", texPath.data);

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
            DUAL_Result result = DUAL_Texture_LoadFromFile(resources, texPathBuffer, DUAL_FILTER_NEAREST, &texture);
            if (result != DUAL_OK) {
                DUAL_Log(DUAL_LOG_WARNING, "Failed to load %s", texPathBuffer);
                continue;
            }

            DUAL_Material_Create(resources, texture, &model->materials[i]);
        } else {
            DUAL_Log(DUAL_LOG_INFO, "  -> Aucune texture diffuse");
        }
    }

    // FIX 2: Dereferencer proprement le pointeur de sortie pour affecter la variable du caller
    if (out_material_count) *out_material_count = model->material_count;
    if (out_materials) *out_materials = model->materials;

    // Chargement indépendant de chaque Mesh
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

        glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex3D), (void*)offsetof(Vertex3D, m_BoneIDs)); glEnableVertexAttribArray(3);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, m_Weights)); glEnableVertexAttribArray(4);

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
    mat->brillance = 32.0f;
    mat->handle = NULL;
    *out_material = mat;
    return DUAL_OK;
}

void DUAL_Material_Destroy(DUAL_ResourceManager* resources, DUAL_Material* material) {
    (void)resources;
    if (material) {
        free(material);
    }
}

void DUAL_Material_SetShininess(DUAL_Material* material, float brillance) {
    if (material) {
        material->brillance = brillance;
    }
}

/* ============================================================================
 * RENDERER 3D, CONFIGURATION ET CAMÉRA
 * ========================================================================== */

DUAL_Result DUAL_Renderer3D_Create(DUAL_App* app, DUAL_Renderer3D** out_renderer) {
    if (!app || !out_renderer) return DUAL_ERROR_INVALID_ARG;
    DUAL_Renderer3D* renderer = malloc(sizeof(struct DUAL_Renderer3D));
    memset(renderer, 0, sizeof(struct DUAL_Renderer3D));
    renderer->app = app;

    GLuint vs_lit = CompileShader3D(GL_VERTEX_SHADER, vertex_shader_lit_src);
    GLuint fs_lit = CompileShader3D(GL_FRAGMENT_SHADER, fragment_shader_lit_src);
    renderer->shader_lit = glCreateProgram();
    glAttachShader(renderer->shader_lit, vs_lit); glAttachShader(renderer->shader_lit, fs_lit); glLinkProgram(renderer->shader_lit);
    glDeleteShader(vs_lit); glDeleteShader(fs_lit);

    GLuint vs_unlit = CompileShader3D(GL_VERTEX_SHADER, vertex_shader_unlit_src);
    GLuint fs_unlit = CompileShader3D(GL_FRAGMENT_SHADER, fragment_shader_unlit_src);
    renderer->shader_unlit = glCreateProgram();
    glAttachShader(renderer->shader_unlit, vs_unlit); glAttachShader(renderer->shader_unlit, fs_unlit); glLinkProgram(renderer->shader_unlit);
    glDeleteShader(vs_unlit); glDeleteShader(fs_unlit);

    GLuint vs_skel = CompileShader3D(GL_VERTEX_SHADER, vertex_shader_skeleton_lit_src);
    renderer->shader_skeleton = glCreateProgram();
    glAttachShader(renderer->shader_skeleton, vs_skel); glAttachShader(renderer->shader_skeleton, fs_lit); glLinkProgram(renderer->shader_skeleton);
    renderer->shader_skeleton_unlit = glCreateProgram();
    glAttachShader(renderer->shader_skeleton_unlit, vs_skel); glAttachShader(renderer->shader_skeleton_unlit, fs_unlit); glLinkProgram(renderer->shader_skeleton_unlit);
    glDeleteShader(vs_skel);

    renderer->render_mode = DUAL_RENDER_LIT;
    renderer->view = DUAL_Mat4_Identity();
    renderer->camera_position = (DUAL_Vec3){ 0.0f, 0.0f, 0.0f };
    renderer->projection_mode = DUAL_PROJECTION_PERSPECTIVE;
    renderer->fov_radians = 1.0471975512f;
    renderer->ortho_demi_hauteur = 5.0f;
    renderer->plan_proche = 0.1f;
    renderer->plan_lointain = 100.0f;
    renderer->cull_mode = DUAL_CULL_BACK;

    *out_renderer = renderer;
    return DUAL_OK;
}

void DUAL_Renderer3D_Destroy(DUAL_Renderer3D* renderer) {
    if (!renderer) return;
    if (renderer->shader_lit) glDeleteProgram(renderer->shader_lit);
    if (renderer->shader_unlit) glDeleteProgram(renderer->shader_unlit);
    if (renderer->shader_skeleton) glDeleteProgram(renderer->shader_skeleton);
    if (renderer->shader_skeleton_unlit) glDeleteProgram(renderer->shader_skeleton_unlit);
    free(renderer);
}

void DUAL_Renderer3D_SetCameraLookAt(DUAL_Renderer3D* renderer, DUAL_Vec3 position, DUAL_Vec3 cible, DUAL_Vec3 haut) {
    if (!renderer) return;
    renderer->camera_position = position;
    renderer->view = DUAL_Mat4_LookAt(position, cible, haut);
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

/* ============================================================================
 * ÉCLAIRAGE
 * ========================================================================== */

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

    if (renderer->render_mode == DUAL_RENDER_WIREFRAME) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    int32_t w = 0, h = 0;
    DUAL_Internal_GetScreenDimensions(renderer->app, &w, &h);
    float aspect = (h != 0) ? ((float)w / (float)h) : 1.0f;

    DUAL_Mat4 proj;
    if (renderer->projection_mode == DUAL_PROJECTION_ORTHOGRAPHIC) {
        float demi_h = (renderer->ortho_demi_hauteur > 0.0f) ? renderer->ortho_demi_hauteur : 5.0f;
        float demi_w = demi_h * aspect;
        proj = DUAL_Mat4_Ortho(-demi_w, demi_w, -demi_h, demi_h, renderer->plan_proche, renderer->plan_lointain);
    } else {
        proj = DUAL_Mat4_Perspective(renderer->fov_radians, aspect, renderer->plan_proche, renderer->plan_lointain);
    }

    GLuint shaders[] = { renderer->shader_lit, renderer->shader_unlit, renderer->shader_skeleton, renderer->shader_skeleton_unlit };
    for (int s = 0; s < 4; s++) {
        glUseProgram(shaders[s]);
        glUniformMatrix4fv(glGetUniformLocation(shaders[s], "uProjection"), 1, GL_FALSE, proj.m);
        glUniformMatrix4fv(glGetUniformLocation(shaders[s], "uView"), 1, GL_FALSE, renderer->view.m);

        if (shaders[s] == renderer->shader_lit || shaders[s] == renderer->shader_skeleton) {
            glUniform3f(glGetUniformLocation(shaders[s], "uAmbientLight"), renderer->ambient_light.x, renderer->ambient_light.y, renderer->ambient_light.z);
            glUniform3f(glGetUniformLocation(shaders[s], "uViewPos"), renderer->camera_position.x, renderer->camera_position.y, renderer->camera_position.z);
            char uname[64];
            for (int i = 0; i < DUAL_MAX_LIGHTS_3D; i++) {
                DUAL_Light* l = &renderer->lights[i];
                sprintf(uname, "uLights[%d].type", i); glUniform1i(glGetUniformLocation(shaders[s], uname), l->type);
                sprintf(uname, "uLights[%d].position", i); glUniform3f(glGetUniformLocation(shaders[s], uname), l->position.x, l->position.y, l->position.z);
                sprintf(uname, "uLights[%d].direction", i); glUniform3f(glGetUniformLocation(shaders[s], uname), l->direction.x, l->direction.y, l->direction.z);
                sprintf(uname, "uLights[%d].color", i); glUniform3f(glGetUniformLocation(shaders[s], uname), l->couleur.x, l->couleur.y, l->couleur.z);
                sprintf(uname, "uLights[%d].intensity", i); glUniform1f(glGetUniformLocation(shaders[s], uname), l->intensite);
                sprintf(uname, "uLights[%d].range", i); glUniform1f(glGetUniformLocation(shaders[s], uname), l->portee);
            }
        }
    }
}

void DUAL_Renderer3D_End(DUAL_Renderer3D* renderer) {
    (void)renderer;
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

/* ============================================================================
 * FONCTIONS DE DESSIN
 * ========================================================================== */

void DUAL_DrawAnimatedModel(DUAL_Renderer3D* renderer, const DUAL_Model* model, const DUAL_Material** materials, DUAL_Transform3D transform, Animator* animator) {
    if (!renderer || !model) {
        DUAL_Log(DUAL_LOG_ERROR, "DUAL_DrawAnimatedModel: renderer or model is NULL");
        return;
    }

    // FIX 3: Si 'materials' passe NULL, on bascule automatiquement sur les matériaux internes du modèle
    if (!materials) {
        materials = (const DUAL_Material**)model->materials;
    }

    GLuint active_shader;
    if (animator) {
        active_shader = (renderer->render_mode == DUAL_RENDER_UNLIT) ? renderer->shader_skeleton_unlit : renderer->shader_skeleton;
    } else {
        active_shader = (renderer->render_mode == DUAL_RENDER_UNLIT) ? renderer->shader_unlit : renderer->shader_lit;
    }

    glUseProgram(active_shader);

    DUAL_Mat4 scale   = DUAL_Mat4_Scale(transform.echelle);
    DUAL_Mat4 rot_x   = DUAL_Mat4_Rotate((DUAL_Vec3){ 1.0f, 0.0f, 0.0f }, transform.rotation_euler_radians.x);
    DUAL_Mat4 rot_y   = DUAL_Mat4_Rotate((DUAL_Vec3){ 0.0f, 1.0f, 0.0f }, transform.rotation_euler_radians.y);
    DUAL_Mat4 rot_z   = DUAL_Mat4_Rotate((DUAL_Vec3){ 0.0f, 0.0f, 1.0f }, transform.rotation_euler_radians.z);
    DUAL_Mat4 trans   = DUAL_Mat4_Translate(transform.position);
    DUAL_Mat4 modele  = DUAL_Mat4_Multiply(trans, DUAL_Mat4_Multiply(DUAL_Mat4_Multiply(rot_z, DUAL_Mat4_Multiply(rot_y, rot_x)), scale));

    glUniformMatrix4fv(glGetUniformLocation(active_shader, "uModel"), 1, GL_FALSE, modele.m);

    if (animator && (active_shader == renderer->shader_skeleton || active_shader == renderer->shader_skeleton_unlit)) {
        for (int i = 0; i < MAX_BONES; i++) {
            char matName[64];
            sprintf(matName, "finalBonesMatrices[%d]", i);
            glUniformMatrix4fv(glGetUniformLocation(active_shader, matName), 1, GL_FALSE, animator->m_FinalBoneMatrices[i].m);
        }
    }

    // Rendu individuel de chaque Mesh
    for (uint32_t i = 0; i < model->mesh_count; i++) {
        const DUAL_Material* material = NULL;
        uint32_t mat_idx = model->meshes[i].material_index;

        // FIX 4: Vérification des bornes du tableau de matériaux
        if (materials && mat_idx < model->material_count) {
            material = materials[mat_idx];
        }

        if (material && material->texture_diffuse) {
            glUniform1f(glGetUniformLocation(active_shader, "uMaterial.shininess"), material->brillance);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, DUAL_Internal_GetTextureID(material->texture_diffuse));
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glBindVertexArray(model->meshes[i].vao);
        glDrawElements(GL_TRIANGLES, (GLsizei)model->meshes[i].index_count, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
}

void DUAL_DrawModel(DUAL_Renderer3D* renderer, const DUAL_Model* model, const DUAL_Material** materials, DUAL_Transform3D transform) {
    DUAL_DrawAnimatedModel(renderer, model, materials, transform, NULL);
}