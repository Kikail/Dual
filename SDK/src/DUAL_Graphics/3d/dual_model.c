//
// Created by killian on 11/3/26.
//
#include "../../SDK/include/DUAL_Graphics/3d/dual_model.h"

#include <float.h>
#include <stdlib.h>

#include "DUAL_Resources/dual_resources.h"

static inline bool NameEq(const char* a, const char* b) {
    return a[0] == b[0] && strcmp(a, b) == 0;
}

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
            return;
        }
    }
}

static int FindBoneID(const DUAL_Model* model, const char* name) {
    const BoneInfo* map = model->m_BoneInfoMap;
    for (int i = 0, n = model->m_BoneCounter; i < n; i++) {
        if (NameEq(map[i].name, name)) return map[i].id;
    }
    return -1;
}

static int RegisterBone(DUAL_Model* model, const char* name, DUAL_Mat4 offset) {
    if (model->m_BoneCounter >= model->m_BoneMapCapacity) {
        int capacity = model->m_BoneMapCapacity * 2;
        BoneInfo* grown = realloc(model->m_BoneInfoMap, sizeof(BoneInfo) * capacity);
        if (!grown) return -1;
        model->m_BoneInfoMap = grown;
        model->m_BoneMapCapacity = capacity;
    }
    int id = model->m_BoneCounter++;
    model->m_BoneInfoMap[id].id = id;
    model->m_BoneInfoMap[id].name = strdup(name);
    model->m_BoneInfoMap[id].offset = offset;
    return id;
}

static DUAL_Mat4 ConvertAssimpMatrix(const struct aiMatrix4x4* from) {
    DUAL_Mat4 to;
    to.m[0]  = from->a1; to.m[1]  = from->b1; to.m[2]  = from->c1; to.m[3]  = from->d1;
    to.m[4]  = from->a2; to.m[5]  = from->b2; to.m[6]  = from->c2; to.m[7]  = from->d2;
    to.m[8]  = from->a3; to.m[9]  = from->b3; to.m[10] = from->c3; to.m[11] = from->d3;
    to.m[12] = from->a4; to.m[13] = from->b4; to.m[14] = from->c4; to.m[15] = from->d4;
    return to;
}

static void ExtractBoneWeightForVertices(DUAL_Model* model, Vertex3D* vertices, const struct aiMesh* mesh) {
    for (unsigned int b = 0; b < mesh->mNumBones; b++) {
        const struct aiBone* bone = mesh->mBones[b];
        int boneID = FindBoneID(model, bone->mName.data);

        if (boneID < 0) {
            boneID = RegisterBone(model, bone->mName.data, ConvertAssimpMatrix(&bone->mOffsetMatrix));
            if (boneID < 0) continue;
        }

        const struct aiVertexWeight* weights = bone->mWeights;
        for (unsigned int w = 0; w < bone->mNumWeights; w++) {
            SetVertexBoneData(&vertices[weights[w].mVertexId], boneID, weights[w].mWeight);
        }
    }
}

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

static inline float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) {
    float framesDiff = nextTimeStamp - lastTimeStamp;
    if (framesDiff <= 0.0f) return 0.0f;
    return (animationTime - lastTimeStamp) / framesDiff;
}

static DUAL_Mat4 Bone_InterpolatePosition(Bone* bone, float animationTime) {
    if (bone->m_NumPositions == 1) return DUAL_Mat4_Translate(bone->m_Positions[0].position);
    int p0 = Bone_GetPositionIndex(bone, animationTime);
    int p1 = p0 + 1;
    float f = GetScaleFactor(bone->m_Positions[p0].timeStamp, bone->m_Positions[p1].timeStamp, animationTime);
    return DUAL_Mat4_Translate(DUAL_Vec3_Mix(bone->m_Positions[p0].position, bone->m_Positions[p1].position, f));
}

static DUAL_Mat4 Bone_InterpolateRotation(Bone* bone, float animationTime) {
    if (bone->m_NumRotations == 1) return DUAL_Quat_ToMat4(DUAL_Quat_Normalize(bone->m_Rotations[0].orientation));
    int p0 = Bone_GetRotationIndex(bone, animationTime);
    int p1 = p0 + 1;
    float f = GetScaleFactor(bone->m_Rotations[p0].timeStamp, bone->m_Rotations[p1].timeStamp, animationTime);
    return DUAL_Quat_ToMat4(DUAL_Quat_Normalize(DUAL_Quat_Slerp(bone->m_Rotations[p0].orientation, bone->m_Rotations[p1].orientation, f)));
}

static DUAL_Mat4 Bone_InterpolateScaling(Bone* bone, float animationTime) {
    if (bone->m_NumScalings == 1) return DUAL_Mat4_Scale(bone->m_Scales[0].scale);
    int p0 = Bone_GetScaleIndex(bone, animationTime);
    int p1 = p0 + 1;
    float f = GetScaleFactor(bone->m_Scales[p0].timeStamp, bone->m_Scales[p1].timeStamp, animationTime);
    return DUAL_Mat4_Scale(DUAL_Vec3_Mix(bone->m_Scales[p0].scale, bone->m_Scales[p1].scale, f));
}

void Bone_Update(Bone* bone, float animationTime) {
    DUAL_Mat4 translation = Bone_InterpolatePosition(bone, animationTime);
    DUAL_Mat4 rotation    = Bone_InterpolateRotation(bone, animationTime);
    DUAL_Mat4 scale       = Bone_InterpolateScaling(bone, animationTime);
    bone->m_LocalTransform = DUAL_Mat4_Multiply(translation, DUAL_Mat4_Multiply(rotation, scale));
}

static Bone* Animation_FindBone(Animation* animation, const char* name) {
    for (int i = 0, n = animation->m_NumBones; i < n; i++) {
        if (NameEq(animation->m_Bones[i].m_Name, name)) return &animation->m_Bones[i];
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

static void FreeNodeData(AssimpNodeData* node) {
    for (int i = 0; i < node->childrenCount; i++) {
        FreeNodeData(&node->children[i]);
    }
    free((void*)node->children);
    free((void*)node->name);
}

static void FreeAnimation(void* a) {
    Animation* anim = (Animation*)a;
    if (!anim) return;
    for (int i = 0; i < anim->m_NumBones; i++) {
        Bone* bone = &anim->m_Bones[i];
        free((void*)bone->m_Name);
        free(bone->m_Positions);
        free(bone->m_Rotations);
        free(bone->m_Scales);
    }
    free(anim->m_Bones);
    FreeNodeData(&anim->m_RootNode);
    free(anim);
}

static Animation* BuildAnimation(const struct aiScene* scene, const struct aiAnimation* aiAnim, DUAL_Model* model, bool register_missing_bones) {
    Animation* anim = calloc(1, sizeof(Animation));
    if (!anim) return NULL;

    anim->m_Duration = (float)aiAnim->mDuration;
    anim->m_TicksPerSecond = aiAnim->mTicksPerSecond != 0 ? (int)aiAnim->mTicksPerSecond : 25;
    anim->m_LinkedModel = model;

    ReadHierarchyData(&anim->m_RootNode, scene->mRootNode);

    anim->m_NumBones = aiAnim->mNumChannels;
    anim->m_Bones = calloc(anim->m_NumBones > 0 ? anim->m_NumBones : 1, sizeof(Bone));

    for (int i = 0; i < anim->m_NumBones; i++) {
        const struct aiNodeAnim* channel = aiAnim->mChannels[i];
        Bone* bone = &anim->m_Bones[i];

        bone->m_Name = strdup(channel->mNodeName.data);

        int boneID = FindBoneID(model, bone->m_Name);
        if (boneID < 0 && register_missing_bones) {
            boneID = RegisterBone(model, bone->m_Name, DUAL_Mat4_Identity());
        }
        bone->m_ID = boneID;
        bone->m_LocalTransform = DUAL_Mat4_Identity();

        bone->m_NumPositions = channel->mNumPositionKeys;
        bone->m_Positions = malloc(sizeof(KeyPosition) * bone->m_NumPositions);
        for (int j = 0; j < bone->m_NumPositions; j++) {
            const struct aiVectorKey* k = &channel->mPositionKeys[j];
            bone->m_Positions[j].position = (DUAL_Vec3){ k->mValue.x, k->mValue.y, k->mValue.z };
            bone->m_Positions[j].timeStamp = (float)k->mTime;
        }

        bone->m_NumRotations = channel->mNumRotationKeys;
        bone->m_Rotations = malloc(sizeof(KeyRotation) * bone->m_NumRotations);
        for (int j = 0; j < bone->m_NumRotations; j++) {
            const struct aiQuatKey* k = &channel->mRotationKeys[j];
            bone->m_Rotations[j].orientation = (DUAL_Quat){ k->mValue.x, k->mValue.y, k->mValue.z, k->mValue.w };
            bone->m_Rotations[j].timeStamp = (float)k->mTime;
        }

        bone->m_NumScalings = channel->mNumScalingKeys;
        bone->m_Scales = malloc(sizeof(KeyScale) * bone->m_NumScalings);
        for (int j = 0; j < bone->m_NumScalings; j++) {
            const struct aiVectorKey* k = &channel->mScalingKeys[j];
            bone->m_Scales[j].scale = (DUAL_Vec3){ k->mValue.x, k->mValue.y, k->mValue.z };
            bone->m_Scales[j].timeStamp = (float)k->mTime;
        }
    }

    return anim;
}

DUAL_Result DUAL_Animation_Load(const char* path, DUAL_Model* model, Animation** out_animation) {
    const struct aiScene* scene = aiImportFile(path, aiProcess_Triangulate);
    if (!scene || !scene->mRootNode || scene->mNumAnimations == 0) {
        if (scene) aiReleaseImport(scene);
        return DUAL_ERROR_NOT_FOUND;
    }

    Animation* anim = BuildAnimation(scene, scene->mAnimations[0], model, true);
    aiReleaseImport(scene);
    if (!anim) return DUAL_ERROR_OUT_OF_MEMORY;

    *out_animation = anim;
    return DUAL_OK;
}

static void Animator_CalculateBoneTransform(Animator* animator, const DUAL_Model* model, const AssimpNodeData* node, DUAL_Mat4 parentTransform) {
    DUAL_Mat4 nodeTransform = node->transformation;
    Bone* bone = Animation_FindBone(animator->m_CurrentAnimation, node->name);
    int boneID = -1;

    if (bone) {
        Bone_Update(bone, animator->m_CurrentTime);
        nodeTransform = bone->m_LocalTransform;
        boneID = bone->m_ID;
    }
    if (boneID < 0) boneID = FindBoneID(model, node->name);

    DUAL_Mat4 globalTransformation = DUAL_Mat4_Multiply(parentTransform, nodeTransform);

    if (boneID >= 0 && boneID < MAX_BONES) {
        animator->m_FinalBoneMatrices[boneID] = DUAL_Mat4_Multiply(globalTransformation, model->m_BoneInfoMap[boneID].offset);
    }

    for (int i = 0; i < node->childrenCount; i++) {
        Animator_CalculateBoneTransform(animator, model, &node->children[i], globalTransformation);
    }
}

void Animator_Init(Animator* animator, Animation* animation) {
    animator->m_CurrentAnimation = animation;
    animator->m_CurrentTime = 0.0f;
    animator->m_DeltaTime = 0.0f;
    animator->m_Loop = true;
    animator->m_Finished = false;
    const DUAL_Mat4 identity = DUAL_Mat4_Identity();
    for (int i = 0; i < MAX_BONES; i++) {
        animator->m_FinalBoneMatrices[i] = identity;
    }
}

void Animator_UpdateAnimation(Animator* animator, float dt) {
    animator->m_DeltaTime = dt;
    Animation* animation = animator->m_CurrentAnimation;
    if (!animation || animator->m_Finished) return;

    float time = animator->m_CurrentTime + animation->m_TicksPerSecond * dt;

    if (time >= animation->m_Duration) {
        if (animator->m_Loop) {
            time = fmodf(time, animation->m_Duration);
        } else {
            time = animation->m_Duration;
            animator->m_Finished = true;
        }
    }

    animator->m_CurrentTime = time;
    Animator_CalculateBoneTransform(animator, animation->m_LinkedModel, &animation->m_RootNode, DUAL_Mat4_Identity());
}

static const struct aiScene* ImportModelScene(const char* path) {
    const unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_FlipUVs;
    const struct aiScene* scene = aiImportFile(path, flags);
    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        if (scene) aiReleaseImport(scene);
        return NULL;
    }
    return scene;
}

static DUAL_Result BuildModel(DUAL_ResourceManager* resources, const char* chemin_fichier, const struct aiScene* scene, DUAL_Model** out_model, unsigned int* out_material_count, DUAL_Material*** out_materials) {
    DUAL_Model* model = calloc(1, sizeof(DUAL_Model));
    if (!model) return DUAL_ERROR_OUT_OF_MEMORY;

    model->mesh_count = scene->mNumMeshes;
    model->meshes = malloc(sizeof(DUAL_Mesh) * (model->mesh_count > 0 ? model->mesh_count : 1));
    model->m_BoneMapCapacity = MAX_BONES;
    model->m_BoneInfoMap = malloc(sizeof(BoneInfo) * model->m_BoneMapCapacity);
    model->materials = calloc(scene->mNumMaterials > 0 ? scene->mNumMaterials : 1, sizeof(DUAL_Material*));
    model->material_count = scene->mNumMaterials;

    if (!model->meshes || !model->m_BoneInfoMap || !model->materials) {
        free(model->meshes);
        free(model->m_BoneInfoMap);
        free(model->materials);
        free(model);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }

    DUAL_Vec3 bb_min = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
    DUAL_Vec3 bb_max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    const char* last_slash = strrchr(chemin_fichier, '/');
    const char* last_backslash = strrchr(chemin_fichier, '\\');
    const char* last_sep = (last_slash > last_backslash) ? last_slash : last_backslash;
    size_t dir_len = last_sep ? (size_t)(last_sep - chemin_fichier + 1) : 0;

    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        struct aiString texPath;
        if (aiGetMaterialTexture(scene->mMaterials[i], aiTextureType_DIFFUSE, 0, &texPath, NULL, NULL, NULL, NULL, NULL, NULL) != aiReturn_SUCCESS) {
            continue;
        }

        /*
        char texPathBuffer[512];
        size_t len = 0;
        if (dir_len > 0 && dir_len < sizeof(texPathBuffer)) {
            memcpy(texPathBuffer, chemin_fichier, dir_len);
            len = dir_len;
        }
        snprintf(texPathBuffer + len, sizeof(texPathBuffer) - len, "%s", texPath.data);
        */

        DUAL_Texture* texture = NULL;
        if (DUAL_Texture_LoadFromFile(resources, texPath.data, DUAL_FILTER_NEAREST, &texture) == DUAL_OK) {
            DUAL_Material_Create(resources, texture, &model->materials[i]);
        }
    }

    if (out_material_count) *out_material_count = model->material_count;
    if (out_materials) *out_materials = model->materials;

    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        const struct aiMesh* mesh = scene->mMeshes[m];
        DUAL_Mesh* out_mesh = &model->meshes[m];
        const unsigned int vertex_count = mesh->mNumVertices;

        Vertex3D* vertices = malloc(sizeof(Vertex3D) * (vertex_count > 0 ? vertex_count : 1));
        uint32_t* indices = malloc(sizeof(uint32_t) * (mesh->mNumFaces * 3 + 1));

        const struct aiVector3D* normals = mesh->mNormals;
        const struct aiVector3D* uvs = mesh->mTextureCoords[0];

        for (unsigned int v = 0; v < vertex_count; v++) {
            Vertex3D* vertex = &vertices[v];
            SetVertexBoneDataToDefault(vertex);

            const struct aiVector3D* p = &mesh->mVertices[v];
            vertex->position = (DUAL_Vec3){ p->x, p->y, p->z };
            vertex->normale = normals ? (DUAL_Vec3){ normals[v].x, normals[v].y, normals[v].z } : (DUAL_Vec3){ 0.0f, 1.0f, 0.0f };
            vertex->tex_coords = uvs ? (DUAL_Vec2){ uvs[v].x, uvs[v].y } : (DUAL_Vec2){ 0.0f, 0.0f };

            if (p->x < bb_min.x) bb_min.x = p->x;
            if (p->x > bb_max.x) bb_max.x = p->x;
            if (p->y < bb_min.y) bb_min.y = p->y;
            if (p->y > bb_max.y) bb_max.y = p->y;
            if (p->z < bb_min.z) bb_min.z = p->z;
            if (p->z > bb_max.z) bb_max.z = p->z;
        }

        uint32_t index_cursor = 0;
        for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
            const struct aiFace* face = &mesh->mFaces[f];
            for (unsigned int k = 0; k < face->mNumIndices; k++) {
                indices[index_cursor++] = face->mIndices[k];
            }
        }

        ExtractBoneWeightForVertices(model, vertices, mesh);

        for (unsigned int v = 0; v < vertex_count; v++) {
            float* w = vertices[v].m_Weights;
            float total = w[0] + w[1] + w[2] + w[3];
            if (total > 0.0f) {
                float inv = 1.0f / total;
                w[0] *= inv; w[1] *= inv; w[2] *= inv; w[3] *= inv;
            }
        }

        out_mesh->index_count = index_cursor;
        out_mesh->material_index = mesh->mMaterialIndex;

        glGenVertexArrays(1, &out_mesh->vao);
        glGenBuffers(1, &out_mesh->vbo);
        glGenBuffers(1, &out_mesh->ebo);

        glBindVertexArray(out_mesh->vao);

        glBindBuffer(GL_ARRAY_BUFFER, out_mesh->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex3D) * vertex_count, vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out_mesh->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * index_cursor, indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, normale));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, tex_coords));
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, m_Weights));
        glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex3D), (void*)offsetof(Vertex3D, m_BoneIDs));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glEnableVertexAttribArray(4);

        glBindVertexArray(0);

        free(vertices);
        free(indices);
    }

    model->bounding_box_local.min = bb_min;
    model->bounding_box_local.max = bb_max;

    strcpy(model->model_path, chemin_fichier);
    DUAL_GetFilenameWithoutExtension(chemin_fichier, model->model_name, 32);
    DUAL_ResourceManager_AddResource((DUAL_ResourceManager*)resources, model, DUAL_RESOURCE_MODEL_3D);

    *out_model = model;
    return DUAL_OK;
}

DUAL_Result DUAL_Model_Load(DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Model** out_model, unsigned int* out_material_count, DUAL_Material*** out_materials) {
    if (!chemin_fichier || !out_model) return DUAL_ERROR_INVALID_ARG;

    char model_name[32];
    DUAL_GetFilenameWithoutExtension(chemin_fichier, model_name, sizeof(model_name));

    DUAL_Model* existing_model = DUAL_ResourceManager_GetModel(resources, model_name);
    if (existing_model != NULL) {
        *out_model = existing_model;
        if (out_material_count) *out_material_count = existing_model->material_count;
        if (out_materials) *out_materials = existing_model->materials;
        DUAL_Log(DUAL_LOG_WARNING, "%s est deja charge", model_name);
        return DUAL_OK;
    }

    const struct aiScene* scene = ImportModelScene(chemin_fichier);
    if (!scene) return DUAL_ERROR_NOT_FOUND;

    DUAL_Result result = BuildModel(resources, chemin_fichier, scene, out_model, out_material_count, out_materials);
    aiReleaseImport(scene);
    return result;
}

void DUAL_Model_Destroy(void* m) {
    DUAL_Model* model = (DUAL_Model*)m;
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
    return model ? model->mesh_count : 0;
}

DUAL_Result DUAL_Material_Create(DUAL_ResourceManager* resources, DUAL_Texture* texture_diffuse, DUAL_Material** out_material) {
    (void)resources;
    if (!out_material) return DUAL_ERROR_INVALID_ARG;
    DUAL_Material* mat = malloc(sizeof(DUAL_Material));
    if (!mat) return DUAL_ERROR_OUT_OF_MEMORY;
    mat->texture_diffuse = texture_diffuse;
    *out_material = mat;
    return DUAL_OK;
}

void DUAL_Material_Destroy(DUAL_Material* material) {
    free(material);
}
void DUAL_DrawModel(DUAL_Renderer3D* renderer, const DUAL_Model* model, const DUAL_Material** materials, DUAL_Transform3D transform) {
    DUAL_DrawAnimatedModel(renderer, model, materials, transform, NULL);
}
void DUAL_DrawAnimatedModel(DUAL_Renderer3D* renderer, const DUAL_Model* model, const DUAL_Material** materials, DUAL_Transform3D transform, Animator* animator) {
    if (!renderer || !model) return;

    if (!materials) materials = (const DUAL_Material**)model->materials;

    DUAL_Shader* shader = renderer->current_shader;
    DUAL_Shader_use(shader);

    DUAL_Shader_setMat4(shader, "uModel", DUAL_BuildModelMatrix(&transform));
    DUAL_Shader_setMat4(shader, "uView", DUAL_Camera3D_GetViewMatrix(&renderer->camera));
    DUAL_Shader_setMat4(shader, "uProjection", (renderer->projection_mode == DUAL_PROJECTION_PERSPECTIVE) ? renderer->perspective_projection : renderer->orthographic_projection);

    if (animator) {
        DUAL_Shader_setMat4Array(shader, "uFinalBonesMatrices", MAX_BONES, animator->m_FinalBoneMatrices);
    }

    if (shader->renderMode == DUAL_SHADER_LIT) {
        DUAL_Renderer3d_SendLightInfosToShader(renderer, shader);
    }

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(shader->shaderID, "texture_diffuse"), 0);

    GLuint last_texture = (GLuint)~0u;
    for (uint32_t i = 0; i < model->mesh_count; i++) {
        const DUAL_Mesh* mesh = &model->meshes[i];
        GLuint texture = 0;

        if (mesh->material_index < model->material_count) {
            const DUAL_Material* material = materials[mesh->material_index];
            if (material && material->texture_diffuse) {
                texture = DUAL_Texture_GetTextureID(material->texture_diffuse);
            }
        }

        if (texture != last_texture) {
            glBindTexture(GL_TEXTURE_2D, texture);
            last_texture = texture;
        }

        glBindVertexArray(mesh->vao);
        glDrawElements(GL_TRIANGLES, (GLsizei)mesh->index_count, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}
DUAL_Result DUAL_AnimatedModel_Create(DUAL_ResourceManager* resources, const char* glb_path, DUAL_Animated_Model** out_animated_model) {
    if (!glb_path || !out_animated_model) return DUAL_ERROR_INVALID_ARG;

    DUAL_Animated_Model* anim_model = calloc(1, sizeof(DUAL_Animated_Model));
    if (!anim_model) return DUAL_ERROR_OUT_OF_MEMORY;

    const struct aiScene* scene = ImportModelScene(glb_path);
    if (!scene) {
        free(anim_model);
        return DUAL_ERROR_NOT_FOUND;
    }

    DUAL_Result res = BuildModel(resources, glb_path, scene, &anim_model->model, NULL, NULL);
    if (res != DUAL_OK) {
        aiReleaseImport(scene);
        free(anim_model);
        return res;
    }

    anim_model->animation_count = scene->mNumAnimations;
    if (anim_model->animation_count > 0) {
        anim_model->animations = Hashtable_CreateEmpty();
        if (!anim_model->animations) {
            aiReleaseImport(scene);
            DUAL_Model_Destroy(anim_model->model);
            free(anim_model);
            return DUAL_ERROR_OUT_OF_MEMORY;
        }

        Animation* first_anim = NULL;
        for (unsigned int i = 0; i < anim_model->animation_count; i++) {
            Animation* anim = BuildAnimation(scene, scene->mAnimations[i], anim_model->model, false);
            const char* raw_name = scene->mAnimations[i]->mName.data;
            const char* separator = strchr(raw_name, '|');
            const char* clean_name = (separator != NULL) ? (separator + 1) : raw_name;
            char* key_copy = strdup(clean_name);
            Hashtable_Put(anim_model->animations, key_copy, anim);
            if (!first_anim) first_anim = anim;
        }

        Animator_Init(&anim_model->animator, first_anim);
    } else {
        anim_model->animations = NULL;
        Animator_Init(&anim_model->animator, NULL);
    }

    aiReleaseImport(scene);
    *out_animated_model = anim_model;

    strcpy((*out_animated_model)->model_path, glb_path);
    DUAL_GetFilenameWithoutExtension(glb_path, (*out_animated_model)->model_name, 32);
    DUAL_ResourceManager_AddResource((DUAL_ResourceManager*)resources, *out_animated_model, DUAL_RESOURCE_MODEL_3D);

    return DUAL_OK;
}

void DUAL_AnimatedModel_PlayAnimation(DUAL_Animated_Model* anim_model, const char* anim_name) {
    if (!anim_model) return;

    Animation* animation = Hashtable_Get(anim_model->animations, anim_name);
    if (!animation) return;

    Animator_Init(&anim_model->animator, animation);
}

void DUAL_AnimatedModel_Update(DUAL_Animated_Model* anim_model, float dt) {
    if (!anim_model) return;
    Animator_UpdateAnimation(&anim_model->animator, dt);
}

void DUAL_AnimatedModel_Destroy(void* am) {
    DUAL_Animated_Model* anim_model = (DUAL_Animated_Model*)am;
    if (!anim_model) return;
    if (anim_model->animations) {
        Hashtable_Clean(anim_model->animations, FreeAnimation);
        Hashtable_Destroy(anim_model->animations);
    }
    if (anim_model->model) {
        DUAL_Model_Destroy(anim_model->model);
    }
    free(anim_model);
}

DUAL_Vec3 DUAL_Mat4_TransformPoint(DUAL_Mat4 model_mat, DUAL_Vec3 corner);

