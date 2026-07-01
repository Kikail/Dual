/**
 * @file dual_graphics_3d.h
 * @brief Module de rendu 3D de libdual : chargement de modèles (format OBJ),
 *        gestion des matériaux, éclairage de base et caméra 3D.
 *
 * Ce module encapsule le pipeline OpenGL 3D (VBO/VAO/EBO, shaders, depth buffer)
 * derrière une API orientée jeu vidéo. Toutes les opérations de dessin doivent
 * être effectuées entre DUAL_BeginFrame() et DUAL_EndFrame() (voir dual_core.h),
 * après avoir sélectionné l'écran cible avec DUAL_SetActiveScreen().
 */

#ifndef DUAL_GRAPHICS_3D_H
#define DUAL_GRAPHICS_3D_H

#include <stdint.h>
#include <stdbool.h>
#include "dual_core.h"
#include "dual_math.h"
#include "dual_resources.h"
#include "dual_graphics_2d.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  Types opaques
 * ========================================================================== */

/**
 * @brief Représente un modèle 3D chargé en VRAM (géométrie, normales, UVs),
 *        prêt à être dessiné.
 */
typedef struct DUAL_Model DUAL_Model;

/**
 * @brief Représente un matériau appliqué à un modèle 3D : texture diffuse,
 *        propriétés de réflexion de la lumière.
 */
typedef struct DUAL_Material DUAL_Material;

/**
 * @brief Contexte de rendu 3D, responsable de la gestion de la caméra,
 *        des lumières et de la soumission des modèles au GPU.
 */
typedef struct DUAL_Renderer3D DUAL_Renderer3D;

/* ============================================================================
 *  Enumérations
 * ========================================================================== */

/**
 * @brief Type de source lumineuse supportée par le pipeline d'éclairage 3D.
 */
typedef enum DUAL_LightType {
    DUAL_LIGHT_DIRECTIONAL = 0, /**< Lumière directionnelle (ex: soleil), sans position, avec une direction. */
    DUAL_LIGHT_POINT       = 1  /**< Lumière ponctuelle, émettant dans toutes les directions depuis une position. */
} DUAL_LightType;

/**
 * @brief Mode de culling des faces appliqué lors du rendu d'un modèle.
 */
typedef enum DUAL_CullMode {
    DUAL_CULL_BACK  = 0, /**< Élimine les faces arrière (comportement standard). */
    DUAL_CULL_FRONT = 1, /**< Élimine les faces avant. */
    DUAL_CULL_NONE  = 2  /**< Désactive le culling, toutes les faces sont dessinées. */
} DUAL_CullMode;

/* ============================================================================
 *  Structures
 * ========================================================================== */

/**
 * @brief Définit une source de lumière dans la scène 3D.
 */
typedef struct DUAL_Light {
    DUAL_LightType type;       /**< Type de lumière (directionnelle ou ponctuelle). */
    DUAL_Vec3       position;  /**< Position de la lumière (ignorée si directionnelle). */
    DUAL_Vec3       direction; /**< Direction de la lumière (ignorée si ponctuelle). */
    DUAL_Vec3       couleur;   /**< Couleur émise par la lumière (composantes RGB entre 0.0 et 1.0). */
    float           intensite; /**< Intensité lumineuse, généralement entre 0.0 et 1.0 ou plus. */
} DUAL_Light;

/**
 * @brief Représente la transformation spatiale complète d'un objet 3D dans la scène.
 */
typedef struct DUAL_Transform3D {
    DUAL_Vec3 position; /**< Position de l'objet dans l'espace monde. */
    DUAL_Vec3 rotation_euler_radians; /**< Rotation de l'objet, en angles d'Euler (radians) sur chaque axe. */
    DUAL_Vec3 echelle;  /**< Échelle de l'objet sur chaque axe. */
} DUAL_Transform3D;

/* ============================================================================
 *  Chargement et gestion des modèles
 * ========================================================================== */

/**
 * @brief Charge un modèle 3D depuis un fichier au format OBJ.
 *
 * Le chargeur suppose un fichier OBJ standard avec positions, normales et
 * coordonnées de texture triangulées.
 *
 * @param resources Gestionnaire de ressources utilisé pour suivre la consommation VRAM.
 * @param chemin_fichier Chemin du fichier .obj à charger.
 * @param out_model Pointeur recevant le modèle chargé en cas de succès.
 * @return DUAL_OK en cas de succès, DUAL_ERROR_NOT_FOUND si le fichier est introuvable,
 *         ou un autre code d'erreur DUAL_Result en cas d'échec de parsing.
 */
DUAL_Result DUAL_Model_LoadFromOBJ(DUAL_ResourceManager* resources,
                                    const char* chemin_fichier,
                                    DUAL_Model** out_model);

/**
 * @brief Libère un modèle 3D et la mémoire VRAM associée.
 *
 * @param resources Gestionnaire de ressources ayant suivi l'allocation.
 * @param model Modèle à libérer.
 */
void DUAL_Model_Destroy(DUAL_ResourceManager* resources, DUAL_Model* model);

/**
 * @brief Calcule la boîte englobante (AABB) locale d'un modèle chargé.
 *
 * Utile pour les tests de collision ou le frustum culling.
 *
 * @param model Modèle à analyser.
 * @return Boîte englobante du modèle, en espace local (avant transformation).
 */
DUAL_AABB DUAL_Model_GetBoundingBox(const DUAL_Model* model);

/* ============================================================================
 *  Matériaux
 * ========================================================================== */

/**
 * @brief Crée un matériau simple à partir d'une texture diffuse.
 *
 * @param resources Gestionnaire de ressources utilisé pour suivre l'allocation.
 * @param texture_diffuse Texture appliquée à la surface du modèle.
 * @param out_material Pointeur recevant le matériau créé en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_Material_Create(DUAL_ResourceManager* resources,
                                  DUAL_Texture* texture_diffuse,
                                  DUAL_Material** out_material);

/**
 * @brief Libère un matériau. Ne libère pas la texture associée, qui doit être
 *        détruite séparément via DUAL_Texture_Destroy().
 *
 * @param resources Gestionnaire de ressources ayant suivi l'allocation.
 * @param material Matériau à libérer.
 */
void DUAL_Material_Destroy(DUAL_ResourceManager* resources, DUAL_Material* material);

/**
 * @brief Définit le facteur de brillance (composante spéculaire) d'un matériau.
 *
 * @param material Matériau à modifier.
 * @param brillance Valeur de brillance, plus élevée pour des surfaces plus réfléchissantes.
 */
void DUAL_Material_SetShininess(DUAL_Material* material, float brillance);

/* ============================================================================
 *  Renderer 3D et caméra
 * ========================================================================== */

/**
 * @brief Crée un nouveau contexte de rendu 3D, associé à l'application.
 *
 * @param app Instance de l'application libdual.
 * @param out_renderer Pointeur recevant le renderer créé en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_Renderer3D_Create(DUAL_App* app, DUAL_Renderer3D** out_renderer);

/**
 * @brief Détruit un contexte de rendu 3D et libère ses ressources internes.
 *
 * @param renderer Renderer à détruire.
 */
void DUAL_Renderer3D_Destroy(DUAL_Renderer3D* renderer);

/**
 * @brief Définit la matrice de vue de la caméra 3D à partir d'une position,
 *        d'une cible et d'un vecteur "haut".
 *
 * @param renderer Contexte de rendu 3D.
 * @param position Position de la caméra dans l'espace monde.
 * @param cible Point regardé par la caméra.
 * @param haut Vecteur "haut" du monde (généralement {0,1,0}).
 */
void DUAL_Renderer3D_SetCameraLookAt(DUAL_Renderer3D* renderer, DUAL_Vec3 position,
                                     DUAL_Vec3 cible, DUAL_Vec3 haut);

/**
 * @brief Configure les paramètres de projection en perspective de la caméra 3D.
 *
 * @param renderer Contexte de rendu 3D.
 * @param fov_radians Champ de vision vertical, en radians.
 * @param plan_proche Distance du plan de clipping proche.
 * @param plan_lointain Distance du plan de clipping lointain.
 */
void DUAL_Renderer3D_SetProjection(DUAL_Renderer3D* renderer, float fov_radians,
                                    float plan_proche, float plan_lointain);

/**
 * @brief Définit le mode de culling des faces utilisé pour les dessins suivants.
 *
 * @param renderer Contexte de rendu 3D.
 * @param mode Mode de culling à appliquer.
 */
void DUAL_Renderer3D_SetCullMode(DUAL_Renderer3D* renderer, DUAL_CullMode mode);

/**
 * @brief Démarre une passe de rendu 3D pour l'écran actif.
 *
 * Doit être appelée après DUAL_SetActiveScreen() et avant tout appel à
 * DUAL_DrawModel().
 *
 * @param renderer Contexte de rendu 3D.
 */
void DUAL_Renderer3D_Begin(DUAL_Renderer3D* renderer);

/**
 * @brief Termine la passe de rendu 3D courante.
 *
 * @param renderer Contexte de rendu 3D.
 */
void DUAL_Renderer3D_End(DUAL_Renderer3D* renderer);

/* ============================================================================
 *  Éclairage
 * ========================================================================== */

/**
 * @brief Ajoute ou met à jour une source de lumière dans la scène, identifiée
 *        par un index.
 *
 * @param renderer Contexte de rendu 3D.
 * @param index Index de l'emplacement de lumière à définir (commence à 0).
 * @param light Paramètres de la lumière à appliquer.
 */
void DUAL_Renderer3D_SetLight(DUAL_Renderer3D* renderer, int32_t index, DUAL_Light light);

/**
 * @brief Définit la couleur de la lumière ambiante globale de la scène.
 *
 * @param renderer Contexte de rendu 3D.
 * @param couleur_ambiante Couleur ambiante appliquée uniformément à toute la scène.
 */
void DUAL_Renderer3D_SetAmbientLight(DUAL_Renderer3D* renderer, DUAL_Vec3 couleur_ambiante);

/* ============================================================================
 *  Dessin
 * ========================================================================== */

/**
 * @brief Dessine un modèle 3D avec un matériau et une transformation donnés.
 *
 * @param renderer Contexte de rendu 3D, doit être entre Begin() et End().
 * @param model Modèle à dessiner.
 * @param material Matériau appliqué au modèle pendant ce dessin.
 * @param transform Transformation spatiale (position, rotation, échelle) à appliquer.
 */
void DUAL_DrawModel(DUAL_Renderer3D* renderer, const DUAL_Model* model,
                     const DUAL_Material* material, DUAL_Transform3D transform);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_GRAPHICS_3D_H */
