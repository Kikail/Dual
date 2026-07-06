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
#include "dual_core.h"
#include "dual_math.h"
#include "dual_resources.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  Types opaques
 * ========================================================================== */

/**
 * @brief Représente une texture chargée en VRAM, prête à être dessinée.
 *
 * L'implémentation réelle (identifiant de texture OpenGL, format interne,
 * mipmaps) est entièrement cachée au développeur.
 */
typedef struct DUAL_Texture DUAL_Texture;

/**
 * @brief Représente une police bitmap ou vectorielle chargée, utilisée pour
 *        le rendu de texte à l'écran.
 */
typedef struct DUAL_Font DUAL_Font;

/**
 * @brief Contexte de rendu 2D, responsable du batching des sprites et de la
 *        gestion de la caméra 2D pour un écran donné.
 */
typedef struct DUAL_Renderer2D DUAL_Renderer2D;

/* ============================================================================
 *  Enumérations
 * ========================================================================== */

/**
 * @brief Mode de filtrage appliqué à une texture lors de son échantillonnage.
 */
typedef enum DUAL_TextureFilter {
    DUAL_FILTER_NEAREST = 0, /**< Filtrage au plus proche voisin (style pixel art). */
    DUAL_FILTER_LINEAR  = 1  /**< Filtrage bilinéaire (lissage des bords). */
} DUAL_TextureFilter;

/**
 * @brief Mode de mélange (blending) utilisé lors du dessin d'un sprite.
 */
typedef enum DUAL_BlendMode {
    DUAL_BLEND_ALPHA    = 0, /**< Mélange alpha standard (transparence classique). */
    DUAL_BLEND_ADDITIVE = 1, /**< Mélange additif (effets lumineux, particules). */
    DUAL_BLEND_NONE     = 2  /**< Aucun mélange, écrasement opaque des pixels. */
} DUAL_BlendMode;

/* ============================================================================
 *  Structures
 * ========================================================================== */

/**
 * @brief Couleur RGBA avec composantes flottantes normalisées entre 0.0 et 1.0.
 */
typedef struct DUAL_Color {
    float r; /**< Composante rouge, entre 0.0 et 1.0. */
    float g; /**< Composante verte, entre 0.0 et 1.0. */
    float b; /**< Composante bleue, entre 0.0 et 1.0. */
    float a; /**< Composante alpha (opacité), entre 0.0 et 1.0. */
} DUAL_Color;

#define DUAL_COLOR_RED (DUAL_Color){1.0, 0.0, 0.0, 1.0}
#define DUAL_COLOR_GREEN (DUAL_Color){0.0, 1.0, 0.0, 1.0}
#define DUAL_COLOR_BLUE (DUAL_Color){0.0, 0.0, 1.0, 1.0}
#define DUAL_COLOR_YELLOW (DUAL_Color){1.0, 1.0, 0.0, 1.0}
#define DUAL_COLOR_MAGENTA (DUAL_Color){1.0, 0.0, 1.0, 1.0}
#define DUAL_COLOR_CYAN (DUAL_Color){0.0, 1.0, 1.0, 1.0}
#define DUAL_COLOR_WHITE (DUAL_Color){1.0, 1.0, 1.0, 1.0}
#define DUAL_COLOR_BLACK (DUAL_Color){0.0, 0.0, 1.0, 1.0}

/**
 * @brief Paramètres complets de dessin d'un sprite, transmis à DUAL_DrawSprite().
 *
 * Regrouper les paramètres dans une structure plutôt qu'en arguments multiples
 * permet d'étendre l'API sans casser la compatibilité binaire des jeux existants.
 */
typedef struct DUAL_SpriteParams {
    DUAL_Texture* texture;        /**< Texture source à dessiner. */
    DUAL_Vec2     position;       /**< Position du sprite dans l'espace monde 2D. */
    DUAL_Vec2     origine;        /**< Point de pivot du sprite, en pixels, relatif au coin supérieur gauche. */
    DUAL_Vec2     echelle;        /**< Facteur d'échelle sur chaque axe (1.0 = taille native). */
    DUAL_Rect     rect_source;    /**< Sous-rectangle de la texture à dessiner (utile pour les spritesheets). */
    float         rotation_radians; /**< Angle de rotation autour de l'origine, en radians. */
    DUAL_Color    teinte;         /**< Couleur de multiplication appliquée au sprite. */
    DUAL_BlendMode mode_melange;  /**< Mode de mélange à utiliser pour ce sprite. */
    int32_t       profondeur;     /**< Ordre de dessin (z-order) ; les valeurs plus élevées sont dessinées au-dessus. */
} DUAL_SpriteParams;

/* ============================================================================
 *  Chargement et gestion des textures
 * ========================================================================== */

/**
 * @brief Charge une texture depuis un fichier image sur le système de fichiers.
 *
 * @param resources Gestionnaire de ressources utilisé pour suivre la consommation VRAM.
 * @param chemin_fichier Chemin du fichier image (PNG recommandé) à charger.
 * @param filtre Mode de filtrage à appliquer à la texture.
 * @param out_texture Pointeur recevant la texture chargée en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_Texture_LoadFromFile(DUAL_ResourceManager* resources,
                                       const char* chemin_fichier,
                                       DUAL_TextureFilter filtre,
                                       DUAL_Texture** out_texture);

/**
 * @brief Charge une texture depuis un buffer mémoire contenant des données
 *        image déjà décodées (RGBA8).
 *
 * @param resources Gestionnaire de ressources utilisé pour suivre la consommation VRAM.
 * @param pixels Pointeur vers les données de pixels au format RGBA8.
 * @param largeur Largeur de l'image, en pixels.
 * @param hauteur Hauteur de l'image, en pixels.
 * @param filtre Mode de filtrage à appliquer à la texture.
 * @param out_texture Pointeur recevant la texture créée en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_Texture_LoadFromMemory(DUAL_ResourceManager* resources,
                                         const uint8_t* pixels,
                                         int32_t largeur,
                                         int32_t hauteur,
                                         DUAL_TextureFilter filtre,
                                         DUAL_Texture** out_texture);

/**
 * @brief Libère une texture et la mémoire VRAM associée.
 *
 * @param resources Gestionnaire de ressources ayant suivi l'allocation.
 * @param texture Texture à libérer.
 */
void DUAL_Texture_Destroy(DUAL_ResourceManager* resources, DUAL_Texture* texture);

/**
 * @brief Retourne les dimensions natives d'une texture chargée.
 *
 * @param texture Texture à interroger.
 * @param out_largeur Pointeur recevant la largeur en pixels.
 * @param out_hauteur Pointeur recevant la hauteur en pixels.
 */
void DUAL_Texture_GetSize(const DUAL_Texture* texture, int32_t* out_largeur, int32_t* out_hauteur);

/* ============================================================================
 *  Renderer 2D et caméra
 * ========================================================================== */

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

/**
 * @brief Définit la position de la caméra 2D pour l'écran actif.
 *
 * @param renderer Contexte de rendu 2D.
 * @param position Nouvelle position du centre de la caméra, en coordonnées monde.
 */
void DUAL_Renderer2D_SetCameraPosition(DUAL_Renderer2D* renderer, DUAL_Vec2 position);

/**
 * @brief Définit le niveau de zoom de la caméra 2D pour l'écran actif.
 *
 * @param renderer Contexte de rendu 2D.
 * @param zoom Facteur de zoom (1.0 = échelle native, supérieur à 1.0 = rapproché).
 */
void DUAL_Renderer2D_SetCameraZoom(DUAL_Renderer2D* renderer, float zoom);

/**
 * @brief Définit la rotation de la caméra 2D pour l'écran actif.
 *
 * @param renderer Contexte de rendu 2D.
 * @param rotation_radians Angle de rotation de la caméra, en radians.
 */
void DUAL_Renderer2D_SetCameraRotation(DUAL_Renderer2D* renderer, float rotation_radians);

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

/* ============================================================================
 *  Dessin de sprites
 * ========================================================================== */

/**
 * @brief Dessine un sprite selon les paramètres fournis.
 *
 * @param renderer Contexte de rendu 2D, doit être entre Begin() et End().
 * @param params Paramètres complets de dessin du sprite.
 */
void DUAL_DrawSprite(DUAL_Renderer2D* renderer, const DUAL_SpriteParams* params);

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

/* ============================================================================
 *  Texte
 * ========================================================================== */

/**
 * @brief Charge une police depuis un fichier TTF pour le rendu de texte à l'écran.
 *
 * @param resources Gestionnaire de ressources utilisé pour suivre la consommation VRAM/RAM.
 * @param chemin_fichier Chemin du fichier de police (.ttf) à charger.
 * @param taille_pixels Taille de rendu de la police, en pixels.
 * @param out_font Pointeur recevant la police chargée en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_Font_LoadFromFile(DUAL_ResourceManager* resources,
                                    const char* chemin_fichier,
                                    int32_t taille_pixels,
                                    DUAL_Font** out_font);

/**
 * @brief Libère une police chargée et la mémoire associée.
 *
 * @param resources Gestionnaire de ressources ayant suivi l'allocation.
 * @param font Police à libérer.
 */
void DUAL_Font_Destroy(DUAL_ResourceManager* resources, DUAL_Font* font);

/**
 * @brief Dessine une chaîne de texte à une position donnée, avec une police
 *        et une couleur spécifiées.
 *
 * @param renderer Contexte de rendu 2D, doit être entre Begin() et End().
 * @param font Police à utiliser pour le rendu.
 * @param texte Chaîne de caractères à afficher (encodage UTF-8).
 * @param position Position du coin supérieur gauche du texte, en coordonnées monde.
 * @param couleur Couleur du texte.
 */
void DUAL_DrawText(DUAL_Renderer2D* renderer, const DUAL_Font* font,
                    const char* texte, DUAL_Vec2 position, DUAL_Color couleur);

/**
 * @brief Calcule la taille (largeur, hauteur) qu'occuperait une chaîne de texte
 *        si elle était dessinée avec une police donnée, sans la dessiner.
 *
 * Utile pour centrer ou aligner du texte avant de l'afficher.
 *
 * @param font Police à utiliser pour le calcul.
 * @param texte Chaîne de caractères à mesurer (encodage UTF-8).
 * @param out_largeur Pointeur recevant la largeur calculée, en pixels.
 * @param out_hauteur Pointeur recevant la hauteur calculée, en pixels.
 */
void DUAL_MeasureText(const DUAL_Font* font, const char* texte,
                       float* out_largeur, float* out_hauteur);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_GRAPHICS_2D_H */
