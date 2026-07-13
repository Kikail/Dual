/**
 * @file dual_resources.h
 * @brief Module de gestion des ressources de libdual : allocation et suivi
 *        de la mémoire VRAM (textures, modèles) et RAM (assets génériques).
 *
 * Sur une console portable embarquée, la VRAM est une ressource critique et
 * limitée. Ce module centralise le suivi de la consommation mémoire et fournit
 * un système de cache/pool permettant aux autres modules (graphics_2d, graphics_3d,
 * audio) de charger et libérer leurs ressources de façon contrôlée.
 *
 * @note Ce module ne charge pas directement les textures ou modèles : il sert
 *       de gestionnaire bas niveau utilisé en interne par dual_graphics_2d.h,
 *       dual_graphics_3d.h et dual_audio.h. Le développeur final l'utilise
 *       principalement pour interroger ou limiter la consommation mémoire.
 */

#ifndef DUAL_RESOURCES_H
#define DUAL_RESOURCES_H

#include <stdint.h>
#include <stdbool.h>
#include "dual_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  Types opaques
 * ========================================================================== */

/**
 * @brief Gestionnaire de ressources mémoire, responsable du suivi de la VRAM
 *        et de la RAM utilisées par les assets chargés.
 *
 * Une instance unique est généralement créée au démarrage de l'application
 * et partagée par tous les modules consommateurs de ressources.
 */
typedef struct DUAL_ResourceManager DUAL_ResourceManager;

/**
 * @brief Handle opaque représentant une allocation mémoire suivie par le
 *        gestionnaire de ressources (bloc VRAM ou RAM).
 */
typedef struct DUAL_ResourceHandle DUAL_ResourceHandle;

/* ============================================================================
 *  Enumérations
 * ========================================================================== */

/**
 * @brief Catégorie de mémoire ciblée par une allocation.
 */
typedef enum DUAL_MemoryType {
    DUAL_MEMORY_VRAM = 0, /**< Mémoire vidéo : textures, buffers de modèles 3D. */
    DUAL_MEMORY_RAM  = 1  /**< Mémoire système : sons, données de jeu génériques. */
} DUAL_MemoryType;

/**
 * @brief Catégorie fonctionnelle d'une ressource suivie, utilisée pour le
 *        reporting et le déchargement sélectif.
 */
typedef enum DUAL_ResourceCategory {
    DUAL_RESOURCE_TEXTURE   = 0, /**< Texture 2D ou spritesheet. */
    DUAL_RESOURCE_MODEL_3D  = 1, /**< Modèle 3D (géométrie, OBJ importé). */
    DUAL_RESOURCE_SOUND     = 2, /**< Son chargé intégralement en mémoire. */
    DUAL_RESOURCE_MUSIC     = 3, /**< Flux musical en streaming. */
    DUAL_RESOURCE_STRUCT    = 4, /**< Structure personnalisee comme par exemple un personnage. */
    DUAL_RESOURCE_AUTRE     = 5  /**< Ressource générique non catégorisée. */
} DUAL_ResourceCategory;

/* ============================================================================
 *  Structures
 * ========================================================================== */

/**
 * @brief Statistiques globales d'utilisation mémoire à un instant donné.
 */
typedef struct DUAL_MemoryStats {
    uint64_t vram_utilisee_octets;  /**< Quantité de VRAM actuellement utilisée, en octets. */
    uint64_t vram_totale_octets;    /**< Quantité totale de VRAM disponible sur la console, en octets. */
    uint64_t ram_utilisee_octets;   /**< Quantité de RAM actuellement utilisée par les ressources, en octets. */
    uint64_t ram_totale_octets;     /**< Quantité totale de RAM disponible sur la console, en octets. */
    uint32_t nombre_ressources_actives; /**< Nombre total de ressources actuellement chargées. */
} DUAL_MemoryStats;

/* ============================================================================
 *  Cycle de vie du gestionnaire
 * ========================================================================== */

/**
 * @brief Crée un nouveau gestionnaire de ressources.
 *
 * @param app Instance de l'application libdual associée.
 * @param out_manager Pointeur recevant l'instance créée en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_ResourceManager_Create(DUAL_App* app, DUAL_ResourceManager** out_manager);

/**
 * @brief Détruit un gestionnaire de ressources et libère toutes les ressources
 *        encore actives qu'il suivait.
 *
 * @param manager Gestionnaire à détruire.
 */
void DUAL_ResourceManager_Destroy(DUAL_ResourceManager* manager);

/* ============================================================================
 *  Suivi des allocations
 * ========================================================================== */

/**
 * @brief Enregistre une nouvelle allocation auprès du gestionnaire de ressources.
 *
 * Cette fonction est utilisée en interne par les modules de chargement
 * (textures, modèles, sons) afin que la consommation mémoire globale reste
 * connue et limitable.
 *
 * @param manager Gestionnaire de ressources.
 * @param type Type de mémoire concerné (VRAM ou RAM).
 * @param categorie Catégorie fonctionnelle de la ressource.
 * @param taille_octets Taille de l'allocation, en octets.
 * @param nom_debug Nom lisible utilisé pour le débogage et le reporting (peut être NULL).
 * @param out_handle Pointeur recevant le handle de suivi créé.
 * @return DUAL_OK en cas de succès, DUAL_ERROR_OUT_OF_MEMORY si le budget mémoire est dépassé.
 */
DUAL_Result DUAL_ResourceManager_Track(DUAL_ResourceManager* manager,
                                        DUAL_MemoryType type,
                                        DUAL_ResourceCategory categorie,
                                        uint64_t taille_octets,
                                        const char* nom_debug,
                                        DUAL_ResourceHandle** out_handle);

/**
 * @brief Désenregistre une allocation précédemment suivie, libérant le budget
 *        mémoire correspondant.
 *
 * @param manager Gestionnaire de ressources.
 * @param handle Handle de la ressource à libérer. Devient invalide après l'appel.
 */
void DUAL_ResourceManager_Untrack(DUAL_ResourceManager* manager, DUAL_ResourceHandle* handle);

/* ============================================================================
 *  Requêtes et limites
 * ========================================================================== */

/**
 * @brief Récupère les statistiques mémoire courantes (VRAM et RAM).
 *
 * @param manager Gestionnaire de ressources.
 * @param out_stats Pointeur vers une structure DUAL_MemoryStats à remplir.
 */
void DUAL_ResourceManager_GetStats(const DUAL_ResourceManager* manager, DUAL_MemoryStats* out_stats);

/**
 * @brief Définit une limite stricte de VRAM utilisable par l'application.
 *
 * Toute allocation faisant dépasser cette limite échouera avec
 * DUAL_ERROR_OUT_OF_MEMORY, même si la VRAM physique réelle le permettrait,
 * afin de garantir la portabilité du jeu sur le matériel cible.
 *
 * @param manager Gestionnaire de ressources.
 * @param limite_octets Limite de VRAM, en octets.
 */
void DUAL_ResourceManager_SetVRAMBudget(DUAL_ResourceManager* manager, uint64_t limite_octets);

/**
 * @brief Définit une limite stricte de RAM utilisable par les ressources de l'application.
 *
 * @param manager Gestionnaire de ressources.
 * @param limite_octets Limite de RAM, en octets.
 */
void DUAL_ResourceManager_SetRAMBudget(DUAL_ResourceManager* manager, uint64_t limite_octets);

/**
 * @brief Force le déchargement de toutes les ressources d'une catégorie donnée.
 *
 * Utile par exemple pour vider tous les sons et textures d'un niveau avant
 * de charger le niveau suivant.
 *
 * @param manager Gestionnaire de ressources.
 * @param categorie Catégorie de ressources à décharger.
 */
void DUAL_ResourceManager_PurgeCategory(DUAL_ResourceManager* manager, DUAL_ResourceCategory categorie);

/**
 * @brief Force le déchargement de toutes les ressources.
 *
 * Utile par exemple pour vider toutes les ressources d'un niveau avant
 * de charger le niveau suivant.
 *
 * @param manager Gestionnaire de ressources.
 */
void DUAL_ResourceManager_PurgeAll(DUAL_ResourceManager* manager);

/* ============================================================================
 *  Logging
 * ========================================================================== */

/**
 * @brief Affiche toutes les ressources trackees
 *
 * Utile par exemple pour voir tout ce qui est stocker dans le resourceManager
 *
 * @param manager Gestionnaire de ressources.
 */
void DUAL_ResourceManager_Log(DUAL_ResourceManager* manager);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_RESOURCES_H */
