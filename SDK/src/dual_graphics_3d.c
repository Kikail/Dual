#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>

#include <glad/glad.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

/* Inclusion de nos modules */
#include "dual_graphics_3d.h"
#include "dual_graphics_2d.h"
#include "dual_math.h"

extern void  DUAL_Internal_GetScreenDimensions(const DUAL_App* app, int32_t* out_w, int32_t* out_h);
extern GLuint DUAL_Internal_GetTextureID(const DUAL_Texture* texture);

#define DUAL_MAX_LIGHTS_3D 4

/* ============================================================================
 * Définition des structures opaques
 * ========================================================================== */

struct DUAL_Model {
    GLuint   vao, vbo, ebo;
    uint32_t index_count;
    DUAL_AABB bounding_box_local;
    DUAL_ResourceHandle* handle;
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
} Vertex3D;

/* ============================================================================
 * Shaders 3D
 * ========================================================================== */

/* --- SHADER UNLIT --- */
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


/* --- SHADER LIT (Éclairage de Phong Multi-Lumières avec Atténuation) --- */
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
    "    float range;\n" // Ajout de la portée (distance max de la lumière)
    "};\n"
    "#define MAX_LIGHTS 4\n"
    "uniform Material uMaterial;\n"
    "uniform Light uLights[MAX_LIGHTS];\n"
    "uniform vec3 uAmbientLight;\n"
    "uniform vec3 uViewPos;\n"
    "void main() {\n"
    "   vec4 texColor = texture(uMaterial.texture_diffuse, TexCoord);\n"
    "   \n"
    "   // 1. Lumière Ambiante globale\n"
    "   vec3 ambient = uAmbientLight * texColor.rgb;\n"
    "   \n"
    "   vec3 norm = normalize(Normal);\n"
    "   vec3 viewDir = normalize(uViewPos - FragPos);\n"
    "   vec3 diffuseAccum = vec3(0.0);\n"
    "   vec3 specularAccum = vec3(0.0);\n"
    "   \n"
    "   // 2. Traitement de toutes les lumières actives\n"
    "   for (int i = 0; i < MAX_LIGHTS; ++i) {\n"
    "       if (uLights[i].intensity <= 0.0) continue;\n"
    "       \n"
    "       vec3 lightDir;\n"
    "       float attenuation = 1.0;\n"
    "       \n"
    "       if (uLights[i].type == 0) { // DIRECTIONNELLE\n"
    "           lightDir = normalize(-uLights[i].direction);\n"
    "       } else { // PONCTUELLE\n"
    "           vec3 lightVec = uLights[i].position - FragPos;\n"
    "           float distance = length(lightVec);\n"
    "           lightDir = normalize(lightVec);\n"
    "           \n"
    "           // Atténuation douce basée sur la portée (uLights[i].range)\n"
    "           if (uLights[i].range > 0.0) {\n"
    "               float ratio = distance / uLights[i].range;\n"
    "               // Formule quadratique d'atténuation classique et douce\n"
    "               attenuation = 1.0 / (1.0 + 2.0 * ratio + ratio * ratio);\n"
    "               // Coupe nette à la limite de portée\n"
    "               if (distance > uLights[i].range) attenuation = 0.0;\n"
    "           }\n"
    "       }\n"
    "       \n"
    "       // Calcul Diffus\n"
    "       float diff = max(dot(norm, lightDir), 0.0);\n"
    "       diffuseAccum += uLights[i].color * uLights[i].intensity * diff * texColor.rgb * attenuation;\n"
    "       \n"
    "       // Calcul Spéculaire\n"
    "       vec3 reflectDir = reflect(-lightDir, norm);\n"
    "       float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shininess);\n"
    "       specularAccum += uLights[i].color * uLights[i].intensity * spec * attenuation;\n"
    "   }\n"
    "   \n"
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
        case DUAL_CULL_BACK:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        case DUAL_CULL_FRONT:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case DUAL_CULL_NONE:
        default:
            glDisable(GL_CULL_FACE);
            break;
    }
}

/* ============================================================================
 * GESTION DES MODÈLES
 * ========================================================================== */

DUAL_Result DUAL_Model_LoadFromOBJ(DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Model** out_model) {
    if (!chemin_fichier || !out_model) return DUAL_ERROR_INVALID_ARG;

    const unsigned int flags_assimp = aiProcess_Triangulate
                                     | aiProcess_JoinIdenticalVertices
                                     | aiProcess_GenSmoothNormals
                                     | aiProcess_FlipUVs;

    const struct aiScene* scene = aiImportFile(chemin_fichier, flags_assimp);

    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        DUAL_Log(DUAL_LOG_ERROR, "Assimp n'a pas pu charger '%s' : %s", chemin_fichier, aiGetErrorString());
        if (scene) aiReleaseImport(scene);
        return DUAL_ERROR_NOT_FOUND;
    }

    if (scene->mNumMeshes == 0) {
        DUAL_Log(DUAL_LOG_ERROR, "'%s' ne contient aucun mesh exploitable.", chemin_fichier);
        aiReleaseImport(scene);
        return DUAL_ERROR_INIT_FAILED;
    }

    const unsigned int nombre_meshes = scene->mNumMeshes;

    uint32_t total_vertices = 0;
    uint32_t total_indices  = 0;
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        total_vertices += scene->mMeshes[m]->mNumVertices;
        total_indices  += scene->mMeshes[m]->mNumFaces * 3;
    }

    uint64_t taille_vram = ((uint64_t)total_vertices * sizeof(Vertex3D)) + ((uint64_t)total_indices * sizeof(uint32_t));
    DUAL_ResourceHandle* model_handle = NULL;

    if (resources) {
        DUAL_Result res = DUAL_ResourceManager_Track(resources, DUAL_MEMORY_VRAM, DUAL_RESOURCE_MODEL_3D, taille_vram, chemin_fichier, &model_handle);
        if (res != DUAL_OK) {
            aiReleaseImport(scene);
            return res;
        }
    }

    Vertex3D* vertices = (Vertex3D*)malloc(sizeof(Vertex3D) * total_vertices);
    uint32_t* indices  = (uint32_t*)malloc(sizeof(uint32_t) * total_indices);
    if (!vertices || !indices) {
        free(vertices);
        free(indices);
        aiReleaseImport(scene);
        if (resources && model_handle) DUAL_ResourceManager_Untrack(resources, model_handle);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }

    DUAL_Vec3 bb_min = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
    DUAL_Vec3 bb_max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    uint32_t vertex_cursor = 0;
    uint32_t index_cursor  = 0;

    for (unsigned int m = 0; m < nombre_meshes; m++) {
        const struct aiMesh* mesh = scene->mMeshes[m];
        const uint32_t base_vertex = vertex_cursor;

        for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
            DUAL_Vec3 pos = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };

            DUAL_Vec3 normale = { 0.0f, 1.0f, 0.0f };
            if (mesh->mNormals) {
                normale.x = mesh->mNormals[v].x;
                normale.y = mesh->mNormals[v].y;
                normale.z = mesh->mNormals[v].z;
            }

            DUAL_Vec2 uv = { 0.0f, 0.0f };
            if (mesh->mTextureCoords[0]) {
                uv.x = mesh->mTextureCoords[0][v].x;
                uv.y = mesh->mTextureCoords[0][v].y;
            }

            vertices[vertex_cursor].position   = pos;
            vertices[vertex_cursor].normale    = normale;
            vertices[vertex_cursor].tex_coords = uv;

            if (pos.x < bb_min.x) bb_min.x = pos.x;
            if (pos.y < bb_min.y) bb_min.y = pos.y;
            if (pos.z < bb_min.z) bb_min.z = pos.z;
            if (pos.x > bb_max.x) bb_max.x = pos.x;
            if (pos.y > bb_max.y) bb_max.y = pos.y;
            if (pos.z > bb_max.z) bb_max.z = pos.z;

            vertex_cursor++;
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
            const struct aiFace* face = &mesh->mFaces[f];
            for (unsigned int k = 0; k < face->mNumIndices; k++) {
                indices[index_cursor++] = base_vertex + face->mIndices[k];
            }
        }
    }

    aiReleaseImport(scene);

    DUAL_Model* model = (DUAL_Model*)malloc(sizeof(struct DUAL_Model));
    if (!model) {
        free(vertices); free(indices);
        if (resources && model_handle) DUAL_ResourceManager_Untrack(resources, model_handle);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }

    model->index_count = index_cursor;
    model->bounding_box_local.min = bb_min;
    model->bounding_box_local.max = bb_max;
    model->handle = model_handle;

    if (resources && model_handle) {
        extern void DUAL_Internal_ResourceHandle_SetCallback(DUAL_ResourceHandle* handle, void* ptr, void (*cb)(DUAL_ResourceManager*, void*));
        DUAL_Internal_ResourceHandle_SetCallback(model_handle, model, (void(*)(DUAL_ResourceManager*, void*))DUAL_Model_Destroy);
    }

    glGenVertexArrays(1, &model->vao);
    glGenBuffers(1, &model->vbo);
    glGenBuffers(1, &model->ebo);

    glBindVertexArray(model->vao);

    glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(Vertex3D) * vertex_cursor), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(sizeof(uint32_t) * index_cursor), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, normale));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, tex_coords));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    free(vertices);
    free(indices);

    *out_model = model;
    DUAL_Log(DUAL_LOG_INFO, "Modèle '%s' chargé via Assimp (%u sommets, %u indices, %u mesh(es)).",
             chemin_fichier, vertex_cursor, index_cursor, nombre_meshes);
    return DUAL_OK;
}

void DUAL_Model_Destroy(DUAL_ResourceManager* resources, DUAL_Model* model) {
    if (model) {
        glDeleteVertexArrays(1, &model->vao);
        glDeleteBuffers(1, &model->vbo);
        glDeleteBuffers(1, &model->ebo);
        if (resources && model->handle) {
            DUAL_ResourceManager_Untrack(resources, model->handle);
        }
        free(model);
    }
}

DUAL_AABB DUAL_Model_GetBoundingBox(const DUAL_Model* model) {
    DUAL_AABB vide = { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    if (!model) return vide;
    return model->bounding_box_local;
}

/* ============================================================================
 * GESTION DES MATÉRIAUX
 * ========================================================================== */

DUAL_Result DUAL_Material_Create(DUAL_ResourceManager* resources, DUAL_Texture* texture_diffuse, DUAL_Material** out_material) {
    if (!out_material) return DUAL_ERROR_INVALID_ARG;

    uint64_t taille_ram = sizeof(struct DUAL_Material);
    DUAL_ResourceHandle* mat_handle = NULL;

    if (resources) {
        DUAL_Result res = DUAL_ResourceManager_Track(resources, DUAL_MEMORY_RAM, DUAL_RESOURCE_AUTRE, taille_ram, "Material_3D", &mat_handle);
        if (res != DUAL_OK) return res;
    }

    DUAL_Material* mat = (DUAL_Material*)malloc(sizeof(struct DUAL_Material));
    if (!mat) {
        if (resources && mat_handle) DUAL_ResourceManager_Untrack(resources, mat_handle);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }

    mat->texture_diffuse = texture_diffuse;
    mat->brillance = 32.0f;
    mat->handle = mat_handle;

    if (resources && mat_handle) {
        extern void DUAL_Internal_ResourceHandle_SetCallback(DUAL_ResourceHandle* handle, void* ptr, void (*cb)(DUAL_ResourceManager*, void*));
        DUAL_Internal_ResourceHandle_SetCallback(mat_handle, mat, (void(*)(DUAL_ResourceManager*, void*))DUAL_Material_Destroy);
    }

    *out_material = mat;
    return DUAL_OK;
}

void DUAL_Material_Destroy(DUAL_ResourceManager* resources, DUAL_Material* material) {
    if (material) {
        if (resources && material->handle) {
            DUAL_ResourceManager_Untrack(resources, material->handle);
        }
        free(material);
    }
}

void DUAL_Material_SetShininess(DUAL_Material* material, float brillance) {
    if (material) material->brillance = brillance;
}

/* ============================================================================
 * RENDERER 3D ET CAMÉRA
 * ========================================================================== */

DUAL_Result DUAL_Renderer3D_Create(DUAL_App* app, DUAL_Renderer3D** out_renderer) {
    if (!app || !out_renderer) return DUAL_ERROR_INVALID_ARG;

    DUAL_Renderer3D* renderer = (DUAL_Renderer3D*)malloc(sizeof(struct DUAL_Renderer3D));
    if (!renderer) return DUAL_ERROR_OUT_OF_MEMORY;

    renderer->app = app;

    /* Compilation du Shader LIT */
    GLuint vs_lit = CompileShader3D(GL_VERTEX_SHADER, vertex_shader_lit_src);
    GLuint fs_lit = CompileShader3D(GL_FRAGMENT_SHADER, fragment_shader_lit_src);
    renderer->shader_lit = glCreateProgram();
    glAttachShader(renderer->shader_lit, vs_lit);
    glAttachShader(renderer->shader_lit, fs_lit);
    glLinkProgram(renderer->shader_lit);
    glDeleteShader(vs_lit);
    glDeleteShader(fs_lit);

    /* Compilation du Shader UNLIT */
    GLuint vs_unlit = CompileShader3D(GL_VERTEX_SHADER, vertex_shader_unlit_src);
    GLuint fs_unlit = CompileShader3D(GL_FRAGMENT_SHADER, fragment_shader_unlit_src);
    renderer->shader_unlit = glCreateProgram();
    glAttachShader(renderer->shader_unlit, vs_unlit);
    glAttachShader(renderer->shader_unlit, fs_unlit);
    glLinkProgram(renderer->shader_unlit);
    glDeleteShader(vs_unlit);
    glDeleteShader(fs_unlit);

    /* Configuration par défaut des textures */
    glUseProgram(renderer->shader_lit);
    glUniform1i(glGetUniformLocation(renderer->shader_lit, "uMaterial.texture_diffuse"), 0);

    glUseProgram(renderer->shader_unlit);
    glUniform1i(glGetUniformLocation(renderer->shader_unlit, "uTexture"), 0);

    /* Rendu par défaut */
    renderer->render_mode = DUAL_RENDER_LIT;

    /* Caméra */
    renderer->view = DUAL_Mat4_Identity();
    renderer->camera_position = (DUAL_Vec3){ 0.0f, 0.0f, 0.0f };

    /* Projection */
    renderer->projection_mode    = DUAL_PROJECTION_PERSPECTIVE;
    renderer->fov_radians        = 1.0471975512f;
    renderer->ortho_demi_hauteur = 5.0f;
    renderer->plan_proche        = 0.1f;
    renderer->plan_lointain      = 100.0f;

    /* Culling */
    renderer->cull_mode = DUAL_CULL_BACK;
    glFrontFace(GL_CCW);

    /* Éclairage par défaut */
    renderer->ambient_light = (DUAL_Vec3){ 1.0f, 1.0f, 1.0f };
    for (int i = 0; i < DUAL_MAX_LIGHTS_3D; i++) {
        renderer->lights[i].type      = DUAL_LIGHT_DIRECTIONAL;
        renderer->lights[i].position  = (DUAL_Vec3){ 0.0f, 0.0f, 0.0f };
        renderer->lights[i].direction = (DUAL_Vec3){ 0.0f, -1.0f, 0.0f };
        renderer->lights[i].couleur   = (DUAL_Vec3){ 1.0f, 1.0f, 1.0f };
        renderer->lights[i].intensite = 0.0f;
        renderer->lights[i].portee    = 10.0f; // Portée par défaut
    }

    *out_renderer = renderer;
    return DUAL_OK;
}

void DUAL_Renderer3D_Destroy(DUAL_Renderer3D* renderer) {
    if (renderer) {
        glDeleteProgram(renderer->shader_lit);
        glDeleteProgram(renderer->shader_unlit);
        free(renderer);
    }
}

void DUAL_Renderer3D_SetCameraLookAt(DUAL_Renderer3D* renderer, DUAL_Vec3 position, DUAL_Vec3 cible, DUAL_Vec3 haut) {
    if (!renderer) return;
    renderer->camera_position = position;
    renderer->view = DUAL_Mat4_LookAt(position, cible, haut);
}

void DUAL_Renderer3D_SetProjection(DUAL_Renderer3D* renderer, float fov_radians, float plan_proche, float plan_lointain) {
    if (!renderer) return;
    renderer->fov_radians   = fov_radians;
    renderer->plan_proche   = plan_proche;
    renderer->plan_lointain = plan_lointain;
}

void DUAL_Renderer3D_SetProjectionMode(DUAL_Renderer3D* renderer, DUAL_ProjectionMode3D mode) {
    if (renderer) renderer->projection_mode = mode;
}

void DUAL_Renderer3D_SetOrthographic(DUAL_Renderer3D* renderer, float demi_hauteur, float plan_proche, float plan_lointain) {
    if (!renderer) return;
    renderer->ortho_demi_hauteur = demi_hauteur;
    renderer->plan_proche        = plan_proche;
    renderer->plan_lointain      = plan_lointain;
}

void DUAL_Renderer3D_SetCullMode(DUAL_Renderer3D* renderer, DUAL_CullMode mode) {
    if (!renderer) return;
    renderer->cull_mode = mode;
    ApplyCullMode3D(mode);
}

void DUAL_Renderer3D_SetRenderMode(DUAL_Renderer3D* renderer, DUAL_RenderMode3D mode) {
    if (!renderer) return;
    renderer->render_mode = mode;
}

void DUAL_Renderer3D_Begin(DUAL_Renderer3D* renderer) {
    if (!renderer) return;

    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    ApplyCullMode3D(renderer->cull_mode);

    int32_t w = 0, h = 0;
    DUAL_Internal_GetScreenDimensions(renderer->app, &w, &h);
    float aspect = (h != 0) ? ((float)w / (float)h) : 1.0f;

    DUAL_Mat4 projection;
    if (renderer->projection_mode == DUAL_PROJECTION_ORTHOGRAPHIC) {
        float demi_h = renderer->ortho_demi_hauteur;
        float demi_w = demi_h * aspect;
        projection = DUAL_Mat4_Ortho(-demi_w, demi_w, -demi_h, demi_h, renderer->plan_proche, renderer->plan_lointain);
    } else {
        projection = DUAL_Mat4_Perspective(renderer->fov_radians, aspect, renderer->plan_proche, renderer->plan_lointain);
    }

    /* On envoie les données caméras au shader LIT */
    glUseProgram(renderer->shader_lit);
    glUniformMatrix4fv(glGetUniformLocation(renderer->shader_lit, "uProjection"), 1, GL_FALSE, projection.m);
    glUniformMatrix4fv(glGetUniformLocation(renderer->shader_lit, "uView"), 1, GL_FALSE, renderer->view.m);
    glUniform3f(glGetUniformLocation(renderer->shader_lit, "uAmbientLight"), renderer->ambient_light.x, renderer->ambient_light.y, renderer->ambient_light.z);
    glUniform3f(glGetUniformLocation(renderer->shader_lit, "uViewPos"), renderer->camera_position.x, renderer->camera_position.y, renderer->camera_position.z);

    /* Envoi de TOUTES les lumières actives (Boucle sur DUAL_MAX_LIGHTS_3D) */
    char uniform_name[64];
    for (int i = 0; i < DUAL_MAX_LIGHTS_3D; i++) {
        DUAL_Light* light = &renderer->lights[i];

        sprintf(uniform_name, "uLights[%d].type", i);
        glUniform1i(glGetUniformLocation(renderer->shader_lit, uniform_name), light->type);

        sprintf(uniform_name, "uLights[%d].position", i);
        glUniform3f(glGetUniformLocation(renderer->shader_lit, uniform_name), light->position.x, light->position.y, light->position.z);

        sprintf(uniform_name, "uLights[%d].direction", i);
        glUniform3f(glGetUniformLocation(renderer->shader_lit, uniform_name), light->direction.x, light->direction.y, light->direction.z);

        sprintf(uniform_name, "uLights[%d].color", i);
        glUniform3f(glGetUniformLocation(renderer->shader_lit, uniform_name), light->couleur.x, light->couleur.y, light->couleur.z);

        sprintf(uniform_name, "uLights[%d].intensity", i);
        glUniform1f(glGetUniformLocation(renderer->shader_lit, uniform_name), light->intensite);

        sprintf(uniform_name, "uLights[%d].range", i);
        glUniform1f(glGetUniformLocation(renderer->shader_lit, uniform_name), light->portee);
    }

    /* On envoie les données caméras au shader UNLIT */
    glUseProgram(renderer->shader_unlit);
    glUniformMatrix4fv(glGetUniformLocation(renderer->shader_unlit, "uProjection"), 1, GL_FALSE, projection.m);
    glUniformMatrix4fv(glGetUniformLocation(renderer->shader_unlit, "uView"), 1, GL_FALSE, renderer->view.m);
}

void DUAL_Renderer3D_End(DUAL_Renderer3D* renderer) {
    if (!renderer) return;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
 * DESSIN
 * ========================================================================== */

void DUAL_DrawModel(DUAL_Renderer3D* renderer, const DUAL_Model* model, const DUAL_Material* material, DUAL_Transform3D transform) {
    if (!renderer || !model) return;

    GLuint active_shader = renderer->shader_lit;

    if (renderer->render_mode == DUAL_RENDER_WIREFRAME) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        active_shader = renderer->shader_unlit;
    } else if (renderer->render_mode == DUAL_RENDER_UNLIT) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        active_shader = renderer->shader_unlit;
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        active_shader = renderer->shader_lit;
    }

    glUseProgram(active_shader);

    DUAL_Mat4 mat_echelle   = DUAL_Mat4_Scale(transform.echelle);
    DUAL_Mat4 mat_rot_x     = DUAL_Mat4_Rotate((DUAL_Vec3){ 1.0f, 0.0f, 0.0f }, transform.rotation_euler_radians.x);
    DUAL_Mat4 mat_rot_y     = DUAL_Mat4_Rotate((DUAL_Vec3){ 0.0f, 1.0f, 0.0f }, transform.rotation_euler_radians.y);
    DUAL_Mat4 mat_rot_z     = DUAL_Mat4_Rotate((DUAL_Vec3){ 0.0f, 0.0f, 1.0f }, transform.rotation_euler_radians.z);
    DUAL_Mat4 mat_translate = DUAL_Mat4_Translate(transform.position);

    DUAL_Mat4 rotation = DUAL_Mat4_Multiply(mat_rot_z, DUAL_Mat4_Multiply(mat_rot_y, mat_rot_x));
    DUAL_Mat4 modele   = DUAL_Mat4_Multiply(mat_translate, DUAL_Mat4_Multiply(rotation, mat_echelle));

    glUniformMatrix4fv(glGetUniformLocation(active_shader, "uModel"), 1, GL_FALSE, modele.m);

    if (active_shader == renderer->shader_lit) {
        if (material) {
            glUniform1f(glGetUniformLocation(active_shader, "uMaterial.shininess"), material->brillance);
        } else {
            glUniform1f(glGetUniformLocation(active_shader, "uMaterial.shininess"), 32.0f);
        }
    }

    glActiveTexture(GL_TEXTURE0);
    if (material && material->texture_diffuse) {
        glBindTexture(GL_TEXTURE_2D, DUAL_Internal_GetTextureID(material->texture_diffuse));
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glBindVertexArray(model->vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)model->index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}