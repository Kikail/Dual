#include <stddef.h>
#include <glad/glad.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Inclusion de nos modules */
#include "dual_graphics_2d.h"
#include "dual_math.h"

/* Configuration et inclusion de STB Image */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCAL
#include "stb_image.h"

/* Configuration et inclusion de STB Truetype */
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

/* Déclarations externes pour interroger la configuration de l'écran */
extern void DUAL_Internal_GetScreenDimensions(const DUAL_App* app, int32_t* out_w, int32_t* out_h);

/* ============================================================================
 * Définition des structures opaques
 * ========================================================================== */

struct DUAL_Texture {
    GLuint id_opengl;
    int32_t largeur;
    int32_t hauteur;
};

struct DUAL_Font {
    DUAL_Texture* texture_atlas;
    float taille_pixels;
    stbtt_bakedchar données_caractères[96]; // Stocke les données ASCII 32 à 126
};

typedef struct {
    DUAL_Vec2 position;
    DUAL_Vec2 tex_coords;
    DUAL_Color couleur;
} Vertex2D;

struct DUAL_Renderer2D {
    DUAL_App* app;
    GLuint vao, vbo;
    GLuint shader_program;

    // Caméra
    DUAL_Vec2 camera_pos;
    float camera_zoom;
    float camera_rotation;

    // Système de Batching
    Vertex2D* vertex_buffer;
    uint32_t vertex_count;
    uint32_t max_vertices;
    DUAL_Texture* texture_courante; // Texture active pour le batch actuel
};

/* Shaders 2D intégrés */
const char* vertex_shader_src =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "layout (location = 2) in vec4 aColor;\n"
    "out vec2 TexCoord;\n"
    "out vec4 VertexColor;\n"
    "uniform mat4 uProjection;\n"
    "uniform mat4 uView;\n"
    "void main() {\n"
    "   gl_Position = uProjection * uView * vec4(aPos, 0.0, 1.0);\n"
    "   TexCoord = aTexCoord;\n"
    "   VertexColor = aColor;\n"
    "}\n";

const char* fragment_shader_src =
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "in vec4 VertexColor;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTexture;\n"
    "void main() {\n"
    "   FragColor = texture(uTexture, TexCoord) * VertexColor;\n"
    "}\n";

static GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("[ERREUR SHADER] %s\n", infoLog);
    }
    return shader;
}

/* ============================================================================
 * GESTION DES TEXTURES (stb_image)
 * ========================================================================== */

DUAL_Result DUAL_Texture_LoadFromFile(DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_TextureFilter filtre, DUAL_Texture** out_texture) {
    (void)resources; // Utilisé plus tard si tu implémentes un tracker de VRAM personnalisé

    int largeur, hauteur, canaux;

    // CORRECTION : Pas de retournement vertical pour harmoniser avec le texte
    stbi_set_flip_vertically_on_load(false);
    uint8_t* pixels = stbi_load(chemin_fichier, &largeur, &hauteur, &canaux, STBI_rgb_alpha);

    if (!pixels) {
        printf("[ERREUR TEXTURE] Impossible de charger : %s\n", chemin_fichier);
        return DUAL_ERROR_NOT_FOUND;
    }

    DUAL_Result resultat = DUAL_Texture_LoadFromMemory(resources, pixels, largeur, hauteur, filtre, out_texture);
    stbi_image_free(pixels);
    return resultat;
}

DUAL_Result DUAL_Texture_LoadFromMemory(DUAL_ResourceManager* resources, const uint8_t* pixels, int32_t largeur, int32_t hauteur, DUAL_TextureFilter filtre, DUAL_Texture** out_texture) {
    (void)resources;
    if (!pixels || !out_texture) return DUAL_ERROR_INVALID_ARG;

    DUAL_Texture* tex = (DUAL_Texture*)malloc(sizeof(struct DUAL_Texture));
    if (!tex) return DUAL_ERROR_OUT_OF_MEMORY;

    tex->largeur = largeur;
    tex->hauteur = hauteur;

    glGenTextures(1, &tex->id_opengl);
    glBindTexture(GL_TEXTURE_2D, tex->id_opengl);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, largeur, hauteur, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    GLint gl_filtre = (filtre == DUAL_FILTER_NEAREST) ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filtre);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filtre);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    *out_texture = tex;
    return DUAL_OK;
}

void DUAL_Texture_Destroy(DUAL_ResourceManager* resources, DUAL_Texture* texture) {
    (void)resources;
    if (texture) {
        glDeleteTextures(1, &texture->id_opengl);
        free(texture);
    }
}

void DUAL_Texture_GetSize(const DUAL_Texture* texture, int32_t* out_largeur, int32_t* out_hauteur) {
    if (texture) {
        if (out_largeur) *out_largeur = texture->largeur;
        if (out_hauteur) *out_hauteur = texture->hauteur;
    }
}

/* Accesseur interne : DUAL_Texture est opaque en dehors de ce fichier, mais
 * dual_graphics_3d.c a besoin de l'id OpenGL pour binder la texture diffuse
 * d'un DUAL_Material. Même pattern que DUAL_Internal_GetGLFWWindow entre
 * dual_core.c et dual_input.c. */
GLuint DUAL_Internal_GetTextureID(const DUAL_Texture* texture) {
    return texture ? texture->id_opengl : 0;
}

/* ============================================================================
 * RENDU GENERAL ET BATCHING
 * ========================================================================== */

DUAL_Result DUAL_Renderer2D_Create(DUAL_App* app, DUAL_Renderer2D** out_renderer) {
    if (!app || !out_renderer) return DUAL_ERROR_INVALID_ARG;

    DUAL_Renderer2D* renderer = (DUAL_Renderer2D*)malloc(sizeof(struct DUAL_Renderer2D));
    if (!renderer) return DUAL_ERROR_OUT_OF_MEMORY;

    renderer->app = app;
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragment_shader_src);
    renderer->shader_program = glCreateProgram();
    glAttachShader(renderer->shader_program, vs);
    glAttachShader(renderer->shader_program, fs);
    glLinkProgram(renderer->shader_program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    renderer->max_vertices = 2000 * 4; // Allocation pour lots de sprites
    renderer->vertex_count = 0;
    renderer->vertex_buffer = (Vertex2D*)malloc(sizeof(Vertex2D) * renderer->max_vertices);
    renderer->texture_courante = NULL;

    glGenVertexArrays(1, &renderer->vao);
    glGenBuffers(1, &renderer->vbo);
    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex2D) * renderer->max_vertices, NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, tex_coords));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, couleur));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    renderer->camera_pos = (DUAL_Vec2){0.0f, 0.0f};
    renderer->camera_zoom = 1.0f;
    renderer->camera_rotation = 0.0f;

    *out_renderer = renderer;
    return DUAL_OK;
}

void DUAL_Renderer2D_Destroy(DUAL_Renderer2D* renderer) {
    if (renderer) {
        glDeleteVertexArrays(1, &renderer->vao);
        glDeleteBuffers(1, &renderer->vbo);
        glDeleteProgram(renderer->shader_program);
        free(renderer->vertex_buffer);
        free(renderer);
    }
}

// Envoie les géométries accumulées au GPU dès que la texture change ou que le buffer est plein
static void FlushBatch(DUAL_Renderer2D* renderer) {
    if (renderer->vertex_count == 0) return;

    glUseProgram(renderer->shader_program);

    // Calcul automatique des matrices selon l'écran virtuel ciblé (ex: 400x240)
    int32_t w, h;
    DUAL_Internal_GetScreenDimensions(renderer->app, &w, &h);
    DUAL_Mat4 projection = DUAL_Mat4_Ortho(0.0f, (float)w, (float)h, 0.0f, -1.0f, 1.0f);

    // Calcul de la matrice de vue caméra avec une origine standard (en haut à gauche)
    DUAL_Mat4 view = DUAL_Mat4_Identity();
    view = DUAL_Mat4_Multiply(view, DUAL_Mat4_Translate((DUAL_Vec3){-renderer->camera_pos.x, -renderer->camera_pos.y, 0.0f}));

    glUniformMatrix4fv(glGetUniformLocation(renderer->shader_program, "uProjection"), 1, GL_FALSE, projection.m);
    glUniformMatrix4fv(glGetUniformLocation(renderer->shader_program, "uView"), 1, GL_FALSE, view.m);

    if (renderer->texture_courante) {
        glBindTexture(GL_TEXTURE_2D, renderer->texture_courante->id_opengl);
    }

    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex2D) * renderer->vertex_count, renderer->vertex_buffer);

    glDrawArrays(GL_TRIANGLES, 0, renderer->vertex_count);

    glBindVertexArray(0);
    renderer->vertex_count = 0;
}

void DUAL_Renderer2D_Begin(DUAL_Renderer2D* renderer) {
    if (renderer) {
        renderer->vertex_count = 0;
        renderer->texture_courante = NULL;
        /* Défensif : le module 2D ne doit jamais dépendre du fait qu'un autre
         * module (dual_graphics_3d.c notamment) ait bien désactivé le culling
         * en sortie. Les quads 2D sont enroulés dans le sens horaire en espace
         * écran ; si GL_CULL_FACE restait actif ici, ils seraient tous éliminés
         * silencieusement (aucun sprite/texte visible). */
        glDisable(GL_CULL_FACE);
    }
}

void DUAL_Renderer2D_End(DUAL_Renderer2D* renderer) {
    if (renderer) {
        FlushBatch(renderer);
    }
}

void DUAL_DrawSprite(DUAL_Renderer2D* renderer, const DUAL_SpriteParams* params) {
    if (!renderer || !params || !params->texture) return;

    // Si changement de texture, on vide le lot actuel
    if (renderer->texture_courante != params->texture) {
        FlushBatch(renderer);
        renderer->texture_courante = params->texture;
    }

    // Gestion du mode de mélange (Blending)
    if (params->mode_melange == DUAL_BLEND_ALPHA) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else if (params->mode_melange == DUAL_BLEND_ADDITIVE) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    } else {
        glDisable(GL_BLEND);
    }

    // Sécurité débordement buffer
    if (renderer->vertex_count + 6 >= renderer->max_vertices) {
        FlushBatch(renderer);
    }

    // Calcul des dimensions (Correction du bug de hauteur appliquée)
    float w = (params->rect_source.largeur > 0) ? params->rect_source.largeur : (float)params->texture->largeur;
    float h = (params->rect_source.hauteur > 0) ? params->rect_source.hauteur : (float)params->texture->hauteur;

    w *= params->echelle.x;
    h *= params->echelle.y;


    // Positions des 4 coins locaux
    DUAL_Vec2 p0 = { -params->origine.x, -params->origine.y };
    DUAL_Vec2 p1 = { w - params->origine.x, -params->origine.y };
    DUAL_Vec2 p2 = { w - params->origine.x, h - params->origine.y };
    DUAL_Vec2 p3 = { -params->origine.x, h - params->origine.y };

    // Application de la rotation si nécessaire
    if (params->rotation_radians != 0.0f) {
        float cos_r = cosf(params->rotation_radians);
        float sin_r = sinf(params->rotation_radians);
        #define ROTATE(p) (DUAL_Vec2){ p.x * cos_r - p.y * sin_r, p.x * sin_r + p.y * cos_r }
        p0 = ROTATE(p0); p1 = ROTATE(p1); p2 = ROTATE(p2); p3 = ROTATE(p3);
    }

    // Translation globale vers la position Monde
    p0.x += params->position.x; p0.y += params->position.y;
    p1.x += params->position.x; p1.y += params->position.y;
    p2.x += params->position.x; p2.y += params->position.y;
    p3.x += params->position.x; p3.y += params->position.y;

    // Calcul des coordonnées UV
    float u0 = params->rect_source.x / params->texture->largeur;
    float v0 = params->rect_source.y / params->texture->hauteur;
    float u1 = (params->rect_source.x + ((params->rect_source.largeur > 0) ? params->rect_source.largeur : params->texture->largeur)) / params->texture->largeur;
    float v1 = (params->rect_source.y + ((params->rect_source.hauteur > 0) ? params->rect_source.hauteur : params->texture->hauteur)) / params->texture->hauteur;

    /* Pas d'inversion ici : les textures (sprites via stb_image ET atlas de
     * police via stb_truetype) sont TOUTES chargées sans retournement
     * vertical (voir DUAL_Texture_LoadFromFile plus haut), et ce moteur
     * utilise un repère écran Y-vers-le-bas (voir DUAL_Mat4_Ortho dans
     * FlushBatch : haut=0, bas=hauteur_ecran). Avec cette combinaison,
     * v = y_source / hauteur_texture est DÉJÀ la bonne formule.
     * L'ancienne ligne "v0 = 1.0f - v0; v1 = 1.0f - v1;" inversait ce
     * résultat : chaque glyphe (et le sprite du logo) allait échantillonner
     * une zone de la texture totalement différente de celle voulue — pour
     * le texte en particulier, comme les 96 caractères sont bakés tout en
     * haut de l'atlas 1024x1024, l'inversion les envoyait lire la toute fin
     * de l'atlas, une zone jamais écrite par stb_truetype. D'où un texte
     * illisible (voire invisible) et, accessoirement, un logo affiché à
     * l'envers. */

    // Ajout des 2 triangles (6 sommets) au buffer linéaire
    uint32_t i = renderer->vertex_count;
    renderer->vertex_buffer[i++] = (Vertex2D){ p0, {u0, v0}, params->teinte };
    renderer->vertex_buffer[i++] = (Vertex2D){ p1, {u1, v0}, params->teinte };
    renderer->vertex_buffer[i++] = (Vertex2D){ p2, {u1, v1}, params->teinte };

    renderer->vertex_buffer[i++] = (Vertex2D){ p0, {u0, v0}, params->teinte };
    renderer->vertex_buffer[i++] = (Vertex2D){ p2, {u1, v1}, params->teinte };
    renderer->vertex_buffer[i++] = (Vertex2D){ p3, {u0, v1}, params->teinte };

    renderer->vertex_count = i;
}

/* ============================================================================
 * GESTION DES POLICES ET DU TEXTE (stb_truetype)
 * ========================================================================== */

DUAL_Result DUAL_Font_LoadFromFile(DUAL_ResourceManager* resources, const char* chemin_fichier, int32_t taille_pixels, DUAL_Font** out_font) {
    if (!chemin_fichier || !out_font) return DUAL_ERROR_INVALID_ARG;

    FILE* f = fopen(chemin_fichier, "rb");
    if (!f) return DUAL_ERROR_NOT_FOUND;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) {
        fclose(f);
        return DUAL_ERROR_NOT_FOUND;
    }
    uint8_t* ttf_buffer = (uint8_t*)malloc((size_t)size);
    if (!ttf_buffer) {
        fclose(f);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }
    size_t lus = fread(ttf_buffer, 1, (size_t)size, f);
    fclose(f);
    if (lus != (size_t)size) {
        free(ttf_buffer);
        return DUAL_ERROR_NOT_FOUND;
    }

    int atlas_w = 1024;
    int atlas_h = 1024;
    /* calloc et non malloc : toute la zone de l'atlas non écrite par
     * stbtt_BakeFontBitmap (généralement la grande majorité des 1024x1024
     * pixels) doit valoir 0, sinon elle contient de la mémoire non
     * initialisée qui finit copiée telle quelle dans le canal alpha du
     * bitmap RGBA quelques lignes plus bas. */
    uint8_t* bitmap_mono = (uint8_t*)calloc((size_t)atlas_w * (size_t)atlas_h, 1);
    if (!bitmap_mono) {
        free(ttf_buffer);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }

    DUAL_Font* font = (DUAL_Font*)malloc(sizeof(struct DUAL_Font));
    if (!font) {
        free(ttf_buffer);
        free(bitmap_mono);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }
    font->taille_pixels = (float)taille_pixels;

    int r = stbtt_BakeFontBitmap(ttf_buffer, 0, font->taille_pixels, bitmap_mono, atlas_w, atlas_h, 32, 96, font->données_caractères);
    free(ttf_buffer);

    if (r <= 0) {
        /* r == 0  : aucun caractère n'a pu être placé dans l'atlas
         * r <  0  : seuls (-r) caractères ont pu être placés avant que
         *           l'atlas 1024x1024 soit plein (police trop grande pour
         *           la taille de l'atlas, ou taille_pixels trop élevée) */
        DUAL_Log(DUAL_LOG_ERROR, "Échec du bake de la police '%s' (code stb_truetype: %d) - atlas %dx%d trop petit pour la taille demandée (%d px).",
                 chemin_fichier, r, atlas_w, atlas_h, taille_pixels);
        free(bitmap_mono);
        free(font);
        return DUAL_ERROR_INIT_FAILED;
    }
    /* r > 0 : succès. r correspond à la première ligne non utilisée de
     * l'atlas, donc plus r est petit, plus il reste de marge. */
    DUAL_Log(DUAL_LOG_DEBUG, "Police '%s' bakée avec succès (%d/%d lignes de l'atlas utilisées).", chemin_fichier, r, atlas_h);

    uint8_t* bitmap_rgba = (uint8_t*)malloc(atlas_w * atlas_h * 4);
    if (!bitmap_rgba) {
        free(bitmap_mono);
        free(font);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }
    for (int idx = 0; idx < atlas_w * atlas_h; idx++) {
        bitmap_rgba[idx*4 + 0] = 255; // R
        bitmap_rgba[idx*4 + 1] = 255; // G
        bitmap_rgba[idx*4 + 2] = 255; // B
        bitmap_rgba[idx*4 + 3] = bitmap_mono[idx]; // Alpha
    }

    DUAL_Texture_LoadFromMemory(resources, bitmap_rgba, atlas_w, atlas_h, DUAL_FILTER_LINEAR, &font->texture_atlas);

    free(bitmap_mono);
    free(bitmap_rgba);

    *out_font = font;
    return DUAL_OK;
}

void DUAL_Font_Destroy(DUAL_ResourceManager* resources, DUAL_Font* font) {
    if (font) {
        DUAL_Texture_Destroy(resources, font->texture_atlas);
        free(font);
    }
}

void DUAL_DrawText(DUAL_Renderer2D* renderer, const DUAL_Font* font, const char* texte, DUAL_Vec2 position, DUAL_Color couleur) {
    if (!renderer || !font || !texte) return;

    float x = position.x;
    float y = position.y + font->taille_pixels; // Alignement sur la baseline standard

    while (*texte) {
        char c = *texte;
        if (c >= 32 && c < 128) {
            stbtt_bakedchar* bc = &font->données_caractères[c - 32];

            /* CORRECTION BUG : certains glyphes (l'espace, notamment) ont une
             * bounding box vide (x1==x0 et/ou y1==y0) dans l'atlas stb_truetype.
             * Si on laissait passer un rect_source de largeur/hauteur nulle à
             * DUAL_DrawSprite(), son mécanisme de fallback ("largeur <= 0 => on
             * dessine la texture entière") se déclenchait et affichait l'atlas
             * complet (tous les caractères ASCII) comme un seul énorme sprite
             * à chaque espace. On saute simplement le dessin pour ces glyphes
             * vides, tout en avançant quand même le curseur.
             */
            if (bc->x1 > bc->x0 && bc->y1 > bc->y0) {
                DUAL_SpriteParams p = {
                    .texture = font->texture_atlas,
                    .position = { x + bc->xoff, y + bc->yoff },
                    .origine = {0,0},
                    .echelle = {1.0f, 1.0f},
                    .rect_source = { (float)bc->x0, (float)bc->y0, (float)(bc->x1 - bc->x0), (float)(bc->y1 - bc->y0) },
                    .rotation_radians = 0.0f,
                    .teinte = couleur,
                    .mode_melange = DUAL_BLEND_ALPHA,
                    .profondeur = 0
                };

                DUAL_DrawSprite(renderer, &p);
            }
            x += bc->xadvance;
        }
        texte++;
    }
}

void DUAL_MeasureText(const DUAL_Font* font, const char* texte, float* out_largeur, float* out_hauteur) {
    if (!font || !texte) return;
    float x = 0.0f;
    while (*texte) {
        char c = *texte;
        if (c >= 32 && c < 128) {
            x += font->données_caractères[c - 32].xadvance;
        }
        texte++;
    }
    if (out_largeur) *out_largeur = x;
    if (out_hauteur) *out_hauteur = font->taille_pixels;
}

/* ============================================================================
 * PRÉMICES DE DÉBOGAGE (Rectangles & cercles vides)
 * ========================================================================== */

void DUAL_DrawRect(DUAL_Renderer2D* renderer, DUAL_Rect rect, DUAL_Color couleur) {
    (void)renderer; (void)rect; (void)couleur;
}

void DUAL_DrawCircleOutline(DUAL_Renderer2D* renderer, DUAL_Circle cercle, DUAL_Color couleur, int32_t segments) {
    (void)renderer; (void)cercle; (void)couleur; (void)segments;
}