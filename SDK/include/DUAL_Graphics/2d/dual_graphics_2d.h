/**
 * @file dual_graphics_2d.h
 * @brief Module de rendu 2D de libdual : chargement de textures, dessin de
 *        sprites, gestion de la caméra 2D et du texte.
 *
 * Ce module encapsule entièrement les détails OpenGL (shaders, VBO, VAO,
 * bindings de textures) derrière une API orientée jeu vidéo 2D. Toutes les
 * opérations de dessin doivent être effectuées entre DUAL_BeginFrame() et
 * DUAL_EndFrame() (voir dual_core.h), après avoir sélectionné l'écran cible
 * avec DUAL_SetActiveScreen().
 */

#ifndef DUAL_GRAPHICS_2D_H
#define DUAL_GRAPHICS_2D_H

#include <stdint.h>
#include <stdbool.h>

#include "stb_truetype.h"
#include "../../DUAL_Core/dual_core.h"
#include "../../DUAL_Core/dual_enums.h"
#include "../../DUAL_Math/dual_math.h"
#include "camera2d.h"
#include "../shader.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  Types opaques
 * ========================================================================== */

typedef struct DUAL_ResourceManager DUAL_ResourceManager;
typedef struct DUAL_Texture DUAL_Texture;

typedef struct Vertex2D{
    DUAL_Vec2 position;
    DUAL_Vec2 tex_coords;
    DUAL_Color couleur;
} Vertex2D;


typedef struct Internal_Shaders2d_s{
    DUAL_Shader base_shader;
}Internal_Shaders2d_s ;

typedef struct DUAL_Renderer2D{
    DUAL_App* app;
    GLuint vao, vbo;

    DUAL_Shader* current_shader;
    Internal_Shaders2d_s shaders;

    // Caméra
    DUAL_Camera2D camera;

    // Système de Batching
    Vertex2D* vertex_buffer;
    uint32_t vertex_count;
    uint32_t max_vertices;
    DUAL_Texture* texture_courante;
}DUAL_Renderer2D;

/**
 * @brief Crée un nouveau contexte de rendu 2D, associé à l'application.
 *
 * @param app Instance de l'application libdual.
 * @param out_renderer Pointeur recevant le renderer créé en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_Renderer2D_Create(DUAL_App* app, DUAL_Renderer2D** out_renderer);

/**
 * @brief Détruit un contexte de rendu 2D et libère ses ressources internes.
 *
 * @param renderer Renderer à détruire.
 */
void DUAL_Renderer2D_Destroy(DUAL_Renderer2D* renderer);

DUAL_Camera2D* DUAL_Renderer2D_GetCamera(DUAL_Renderer2D* renderer);

/**
 * @brief Démarre un lot (batch) de dessin de sprites pour l'écran actif.
 *
 * Doit être appelée après DUAL_SetActiveScreen() et avant tout appel à
 * DUAL_DrawSprite() ou DUAL_DrawText().
 *
 * @param renderer Contexte de rendu 2D.
 */
void DUAL_Renderer2D_Begin(DUAL_Renderer2D* renderer);

/**
 * @brief Termine le lot de dessin courant et soumet les sprites accumulés au GPU.
 *
 * @param renderer Contexte de rendu 2D.
 */
void DUAL_Renderer2D_End(DUAL_Renderer2D* renderer);

/**
 * @brief Dessine un rectangle plein, sans texture, de la couleur spécifiée.
 *
 * Pratique pour le débogage des zones de collision ou les éléments d'interface
 * simples ne nécessitant pas de texture.
 *
 * @param renderer Contexte de rendu 2D, doit être entre Begin() et End().
 * @param rect Rectangle à dessiner, en coordonnées monde.
 * @param couleur Couleur de remplissage du rectangle.
 */
void DUAL_DrawRect(DUAL_Renderer2D* renderer, DUAL_Rect rect, DUAL_Color couleur);

/**
 * @brief Dessine le contour d'un cercle, sans remplissage.
 *
 * Pratique pour visualiser des zones de collision circulaires en mode débogage.
 *
 * @param renderer Contexte de rendu 2D, doit être entre Begin() et End().
 * @param cercle Cercle à dessiner.
 * @param couleur Couleur du contour.
 * @param segments Nombre de segments utilisés pour approximer le cercle.
 */
void DUAL_DrawCircleOutline(DUAL_Renderer2D* renderer, DUAL_Circle cercle,
                             DUAL_Color couleur, int32_t segments);

void DUAL_Renderer2D_FlushBatch(DUAL_Renderer2D* renderer);

void DUAL_Renderer2D_UseShader(DUAL_Renderer2D* renderer, DUAL_Renderer2d_Shaders shader);
void DUAL_Renderer2D_UseCustomShader(DUAL_Renderer2D* renderer, DUAL_Shader* shader);


#ifdef __cplusplus
}
#endif

#endif /* DUAL_GRAPHICS_2D_H */
