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

/* Déclarations externes pour interroger d'autres modules de libdual.
 * Même pattern que DUAL_Internal_GetGLFWWindow entre dual_core.c et
 * dual_input.c : DUAL_Texture est opaque hors de dual_graphics_2d.c, donc
 * ce module a besoin d'un accesseur pour récupérer l'id OpenGL sous-jacent
 * (ajouté à la fin de la section "GESTION DES TEXTURES" de dual_graphics_2d.c). */
extern void  DUAL_Internal_GetScreenDimensions(const DUAL_App* app, int32_t* out_w, int32_t* out_h);
extern GLuint DUAL_Internal_GetTextureID(const DUAL_Texture* texture);

#define DUAL_MAX_LIGHTS_3D 1

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
    DUAL_Texture* texture_diffuse; /* Non possédée : voir DUAL_Material_Destroy */
    float         brillance;       /* Réservé à un futur shader éclairé (voir plus bas) */
    DUAL_ResourceHandle* handle;
};

struct DUAL_Renderer3D {
    DUAL_App* app;
    GLuint    shader_program;

    /* Caméra */
    DUAL_Mat4 view;

    /* Projection : un seul mode actif à la fois, choisi via SetProjectionMode */
    DUAL_ProjectionMode3D projection_mode;
    float fov_radians;         /* Utilisé si projection_mode == DUAL_PROJECTION_PERSPECTIVE */
    float ortho_demi_hauteur;  /* Utilisé si projection_mode == DUAL_PROJECTION_ORTHOGRAPHIC */
    float plan_proche;
    float plan_lointain;

    /* Culling */
    DUAL_CullMode cull_mode;

    /* Éclairage : stocké dès maintenant pour ne pas casser l'API publique,
     * mais PAS ENCORE consommé par le shader (voir plus bas, shader unlit). */
    DUAL_Vec3  ambient_light;
    DUAL_Light lights[DUAL_MAX_LIGHTS_3D];
};

/* Format de sommet 3D envoyé au VAO. Le champ "normale" est réservé à un
 * futur shader éclairé : il est bien rempli à partir du modèle chargé et
 * bien branché sur l'attribut de vertex 1, mais le shader unlit actuel ne
 * le lit pas encore. */
typedef struct {
    DUAL_Vec3 position;
    DUAL_Vec3 normale;
    DUAL_Vec2 tex_coords;
} Vertex3D;

/* ============================================================================
 * Shader 3D "unlit" : affichage simple d'une texture, sans éclairage
 * ========================================================================== */

static const char* vertex_shader_3d_src =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n" /* branché, pas encore utilisé */
    "layout (location = 2) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "uniform mat4 uProjection;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uModel;\n"
    "void main() {\n"
    "   gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);\n"
    "   TexCoord = aTexCoord;\n"
    "}\n";

static const char* fragment_shader_3d_src =
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTexture;\n"
    "void main() {\n"
    "   FragColor = texture(uTexture, TexCoord);\n"
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

/* Applique immédiatement le mode de culling passé (utilisé à la fois par
 * DUAL_Renderer3D_SetCullMode et pour ré-appliquer l'état au début de
 * chaque passe, au cas où un autre code aurait touché GL_CULL_FACE entre
 * temps). */
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
 * GESTION DES MODÈLES (chargement via Assimp)
 * ========================================================================== */

DUAL_Result DUAL_Model_LoadFromOBJ(DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Model** out_model) {
    if (!chemin_fichier || !out_model) return DUAL_ERROR_INVALID_ARG;

    /* aiProcess_FlipUVs : DUAL_Texture_LoadFromFile (dual_graphics_2d.c) charge
     * les pixels SANS retournement vertical (stbi_set_flip_vertically_on_load(false)).
     * Par défaut Assimp donne des UV avec l'origine en BAS à gauche (pensés pour
     * une texture chargée à l'ENDROIT/retournée) ; aiProcess_FlipUVs les fait
     * repartir de l'origine HAUT-gauche, ce qui est la convention cohérente avec
     * des pixels non retournés. Sans ce flag, les textures des modèles 3D
     * apparaîtraient inversées verticalement — exactement la même famille de bug
     * que celle corrigée dans DUAL_DrawSprite (voir dual_graphics_2d.c). */
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

    /* Capturé avant aiReleaseImport : la scène ne doit plus être déréférencée
     * après coup (le log final en a besoin). */
    const unsigned int nombre_meshes = scene->mNumMeshes;

    /* Tous les meshes de la scène sont fusionnés en un seul DUAL_Model (un seul
     * DUAL_Material est appliqué à l'ensemble au dessin, voir DUAL_DrawModel).
     * Un premier passage de comptage permet une seule allocation. */
    uint32_t total_vertices = 0;
    uint32_t total_indices  = 0;
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        total_vertices += scene->mMeshes[m]->mNumVertices;
        total_indices  += scene->mMeshes[m]->mNumFaces * 3;
    }

    // Calcul de la taille requise en VRAM
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

    aiReleaseImport(scene); /* scene->mNumMeshes ne doit plus être lu après cette ligne */

    DUAL_Model* model = (DUAL_Model*)malloc(sizeof(struct DUAL_Model));
    if (!model) {
        free(vertices); free(indices); aiReleaseImport(scene);
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

    GLuint vs = CompileShader3D(GL_VERTEX_SHADER, vertex_shader_3d_src);
    GLuint fs = CompileShader3D(GL_FRAGMENT_SHADER, fragment_shader_3d_src);
    renderer->shader_program = glCreateProgram();
    glAttachShader(renderer->shader_program, vs);
    glAttachShader(renderer->shader_program, fs);
    glLinkProgram(renderer->shader_program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glUseProgram(renderer->shader_program);
    glUniform1i(glGetUniformLocation(renderer->shader_program, "uTexture"), 0);

    /* Caméra : identité par défaut, à configurer via DUAL_Renderer3D_SetCameraLookAt */
    renderer->view = DUAL_Mat4_Identity();

    /* Projection : perspective par défaut, avec des valeurs de secours pour
     * éviter une matrice dégénérée si l'appelant dessine avant d'appeler
     * SetProjection/SetOrthographic. */
    renderer->projection_mode    = DUAL_PROJECTION_PERSPECTIVE;
    renderer->fov_radians        = 1.0471975512f; /* 60 degrés */
    renderer->ortho_demi_hauteur = 5.0f;
    renderer->plan_proche        = 0.1f;
    renderer->plan_lointain      = 100.0f;

    /* Culling : GL_CCW est déjà la valeur par défaut d'OpenGL, mais on la fixe
     * explicitement car c'est aussi la convention de sortie d'Assimp pour un
     * repère droit Y-vers-le-haut (voir DUAL_Model_LoadFromOBJ).
     *
     * NOTE : on ne fait QUE stocker cull_mode ici, sans appeler ApplyCullMode3D().
     * glEnable(GL_CULL_FACE) est un état OpenGL global et permanent : l'appeler
     * dès la création du renderer (donc potentiellement avant tout Begin/Draw 3D,
     * voire si aucun rendu 3D n'a encore lieu) activait le culling pour TOUS les
     * draw calls suivants, y compris ceux du renderer 2D. Les quads 2D de
     * dual_graphics_2d.c sont enroulés dans le sens horaire en espace écran, donc
     * considérés "back-face" sous la convention GL_CCW ci-dessous : avec le
     * culling actif, ils étaient silencieusement éliminés avant rasterisation
     * (plus aucun sprite/texte 2D visible, alors même que DUAL_Renderer3D_Begin()
     * n'avait jamais été appelé). DUAL_Renderer3D_Begin() réapplique déjà
     * correctement ApplyCullMode3D() à chaque frame, donc ce n'était de toute
     * façon pas nécessaire ici. */
    renderer->cull_mode = DUAL_CULL_BACK;
    glFrontFace(GL_CCW);

    /* Éclairage : stocké avec des valeurs neutres, non consommé par le shader
     * unlit actuel (voir la structure DUAL_Renderer3D plus haut). */
    renderer->ambient_light = (DUAL_Vec3){ 1.0f, 1.0f, 1.0f };
    for (int i = 0; i < DUAL_MAX_LIGHTS_3D; i++) {
        renderer->lights[i].type      = DUAL_LIGHT_DIRECTIONAL;
        renderer->lights[i].position  = (DUAL_Vec3){ 0.0f, 0.0f, 0.0f };
        renderer->lights[i].direction = (DUAL_Vec3){ 0.0f, -1.0f, 0.0f };
        renderer->lights[i].couleur   = (DUAL_Vec3){ 1.0f, 1.0f, 1.0f };
        renderer->lights[i].intensite = 0.0f;
    }

    *out_renderer = renderer;
    return DUAL_OK;
}

void DUAL_Renderer3D_Destroy(DUAL_Renderer3D* renderer) {
    if (renderer) {
        glDeleteProgram(renderer->shader_program);
        free(renderer);
    }
}

void DUAL_Renderer3D_SetCameraLookAt(DUAL_Renderer3D* renderer, DUAL_Vec3 position, DUAL_Vec3 cible, DUAL_Vec3 haut) {
    if (!renderer) return;
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
    renderer->plan_proche        = plan_proche;   /* Champs partagés avec la perspective : un seul mode actif à la fois */
    renderer->plan_lointain      = plan_lointain;
}

void DUAL_Renderer3D_SetCullMode(DUAL_Renderer3D* renderer, DUAL_CullMode mode) {
    if (!renderer) return;
    renderer->cull_mode = mode;
    ApplyCullMode3D(mode);
}

void DUAL_Renderer3D_Begin(DUAL_Renderer3D* renderer) {
    if (!renderer) return;

    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT); /* Respecte le scissor de l'écran actif, comme le clear couleur de DUAL_BeginFrame */
    ApplyCullMode3D(renderer->cull_mode);

    int32_t w = 0, h = 0;
    DUAL_Internal_GetScreenDimensions(renderer->app, &w, &h);
    float aspect = (h != 0) ? ((float)w / (float)h) : 1.0f;

    DUAL_Mat4 projection;
    if (renderer->projection_mode == DUAL_PROJECTION_ORTHOGRAPHIC) {
        float demi_h = renderer->ortho_demi_hauteur;
        float demi_w = demi_h * aspect;
        /* Repère MONDE standard, Y-vers-le-haut (à ne pas confondre avec le
         * repère écran Y-vers-le-bas utilisé par le module 2D) : haut = +demi_h,
         * bas = -demi_h, sans échange des paramètres. */
        projection = DUAL_Mat4_Ortho(-demi_w, demi_w, -demi_h, demi_h, renderer->plan_proche, renderer->plan_lointain);
    } else {
        projection = DUAL_Mat4_Perspective(renderer->fov_radians, aspect, renderer->plan_proche, renderer->plan_lointain);
    }

    glUseProgram(renderer->shader_program);
    glUniformMatrix4fv(glGetUniformLocation(renderer->shader_program, "uProjection"), 1, GL_FALSE, projection.m);
    glUniformMatrix4fv(glGetUniformLocation(renderer->shader_program, "uView"), 1, GL_FALSE, renderer->view.m);
}

void DUAL_Renderer3D_End(DUAL_Renderer3D* renderer) {
    if (!renderer) return;
    /* Désactivés en sortie pour ne pas affecter un éventuel passage 2D dessiné
     * juste après sur le même écran (dual_graphics_2d.c ne touche jamais au
     * depth test, et ses quads sont enroulés dans le sens horaire donc seraient
     * éliminés par un GL_CULL_FACE resté actif — voir la note dans
     * DUAL_Renderer3D_Create()). */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

/* ============================================================================
 * ÉCLAIRAGE
 * ========================================================================== */

void DUAL_Renderer3D_SetLight(DUAL_Renderer3D* renderer, int32_t index, DUAL_Light light) {
    if (!renderer || index < 0 || index >= DUAL_MAX_LIGHTS_3D) return;
    /* Stocké pour un futur shader éclairé : le shader unlit actuel (voir
     * vertex_shader_3d_src / fragment_shader_3d_src plus haut) ne lit pas
     * encore ces données. */
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

    glUseProgram(renderer->shader_program);

    /* Matrice modèle : Echelle -> Rotation (X puis Y puis Z) -> Translation.
     * DUAL_Mat4_Multiply(a, b) = a*b avec b appliquée en premier (voir
     * dual_math.h), donc on imbrique de l'intérieur (échelle) vers l'extérieur
     * (translation). Si ton pipeline attend un autre ordre d'Euler, c'est ici
     * qu'il faut changer l'imbrication de mat_rot_x/y/z. */
    DUAL_Mat4 mat_echelle   = DUAL_Mat4_Scale(transform.echelle);
    DUAL_Mat4 mat_rot_x     = DUAL_Mat4_Rotate((DUAL_Vec3){ 1.0f, 0.0f, 0.0f }, transform.rotation_euler_radians.x);
    DUAL_Mat4 mat_rot_y     = DUAL_Mat4_Rotate((DUAL_Vec3){ 0.0f, 1.0f, 0.0f }, transform.rotation_euler_radians.y);
    DUAL_Mat4 mat_rot_z     = DUAL_Mat4_Rotate((DUAL_Vec3){ 0.0f, 0.0f, 1.0f }, transform.rotation_euler_radians.z);
    DUAL_Mat4 mat_translate = DUAL_Mat4_Translate(transform.position);

    DUAL_Mat4 rotation = DUAL_Mat4_Multiply(mat_rot_z, DUAL_Mat4_Multiply(mat_rot_y, mat_rot_x));
    DUAL_Mat4 modele   = DUAL_Mat4_Multiply(mat_translate, DUAL_Mat4_Multiply(rotation, mat_echelle));

    glUniformMatrix4fv(glGetUniformLocation(renderer->shader_program, "uModel"), 1, GL_FALSE, modele.m);

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