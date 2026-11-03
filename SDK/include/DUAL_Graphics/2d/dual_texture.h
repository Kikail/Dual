//
// Created by killian on 11/3/26.
//

#ifndef DUAL_DUAL_TEXTURE_H
#define DUAL_DUAL_TEXTURE_H

#include <stdint.h>

#include "DUAL_Core/dual_color.h"
#include "DUAL_Core/dual_enums.h"
#include "DUAL_Math/dual_math.h"
#include "dual_graphics_2d.h"
#include "DUAL_Core/dual_file.h"
#include "glad/glad.h"

typedef struct DUAL_ResourceManager DUAL_ResourceManager;

typedef struct DUAL_Texture{
    GLuint id_opengl;
    int32_t largeur;
    int32_t hauteur;
    char texture_name[32];
    char texture_path[128];
}DUAL_Texture;

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

/**
 * @brief Charge une texture depuis un fichier image sur le système de fichiers.
 *
 * @param resources Gestionnaire de ressources utilisé pour suivre la consommation VRAM.
 * @param chemin_fichier Chemin du fichier image (PNG recommandé) à charger.
 * @param filtre Mode de filtrage à appliquer à la texture.
 * @param out_texture Pointeur recevant la texture chargée en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_Texture_LoadFromFile(DUAL_ResourceManager* resources,const char* chemin_fichier,DUAL_TextureFilter filtre,DUAL_Texture** out_texture);

DUAL_Result DUAL_Texture_LoadFromVFS(DUAL_ResourceManager* resources, DUAL_VFS_File* file, DUAL_TextureFilter filtre, DUAL_Texture** out_texture);

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
DUAL_Result DUAL_Texture_LoadFromMemory(DUAL_ResourceManager* resources,const uint8_t* pixels,int32_t largeur,int32_t hauteur,DUAL_TextureFilter filtre,DUAL_Texture** out_texture);

/**
 * @brief Libère une texture et la mémoire VRAM associée.
 *
 * @param resources Gestionnaire de ressources ayant suivi l'allocation.
 * @param texture Texture à libérer.
 */
void DUAL_Texture_Destroy(void* texture);

/**
 * @brief Retourne les dimensions natives d'une texture chargée.
 *
 * @param texture Texture à interroger.
 * @param out_largeur Pointeur recevant la largeur en pixels.
 * @param out_hauteur Pointeur recevant la hauteur en pixels.
 */
void DUAL_Texture_GetSize(const DUAL_Texture* texture, int32_t* out_largeur, int32_t* out_hauteur);

/**
 * @brief Dessine un sprite selon les paramètres fournis.
 *
 * @param renderer Contexte de rendu 2D, doit être entre Begin() et End().
 * @param params Paramètres complets de dessin du sprite.
 */
void DUAL_DrawSprite(DUAL_Renderer2D* renderer, const DUAL_SpriteParams* params);

GLuint DUAL_Texture_GetTextureID(const DUAL_Texture* texture);

#endif //DUAL_DUAL_TEXTURE_H
