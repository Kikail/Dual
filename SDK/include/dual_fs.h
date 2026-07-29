/**
 * @file dual_fs.h
 * @brief Module système de fichiers de libdual : gestion des sauvegardes
 *        persistantes et lecture des données de cartouches physiques.
 *
 * Ce module abstrait les deux supports de stockage de la console : la mémoire
 * de sauvegarde interne (utilisée pour les profils joueur et progression) et
 * la cartouche physique en lecture seule sur laquelle le jeu est distribué.
 *
 * @note Toutes les opérations de ce module sont synchrones et non thread-safe :
 *       à utiliser depuis le thread principal du jeu. Pour des fichiers de
 *       sauvegarde volumineux, le développeur est encouragé à fragmenter les
 *       écritures pour éviter de bloquer la boucle de jeu trop longtemps.
 *
 * @note Écriture atomique : toute ouverture en écriture (DUAL_SAVE_MODE_ECRITURE
 *       ou DUAL_SAVE_MODE_LECTURE_ECRITURE) passe par un fichier temporaire.
 *       Le fichier de sauvegarde final n'est remplacé qu'à la fermeture réussie
 *       (DUAL_SaveFile_Close), ce qui garantit qu'une sauvegarde existante
 *       n'est jamais corrompue par un crash, une coupure de courant ou un
 *       retrait de cartouche en cours d'écriture.
 *
 * @note Sandboxing de la cartouche : DUAL_Cartridge_ReadFile() et
 *       DUAL_Cartridge_GetFileSize() refusent tout chemin contenant ".." afin
 *       d'empêcher un jeu (malveillant ou buggé) de lire des fichiers en
 *       dehors du dossier de la cartouche.
 */

#ifndef DUAL_FS_H
#define DUAL_FS_H

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
 * @brief Représente un fichier de sauvegarde ouvert, persistant entre les
 *        sessions de jeu.
 */
typedef struct DUAL_SaveFile DUAL_SaveFile;

/**
 * @brief Représente un handle d'accès en lecture à la cartouche physique
 *        insérée dans la console.
 */
typedef struct DUAL_Cartridge DUAL_Cartridge;

/* ============================================================================
 *  Enumérations
 * ========================================================================== */

/**
 * @brief Mode d'ouverture d'un fichier de sauvegarde.
 */
typedef enum DUAL_SaveMode {
    DUAL_SAVE_MODE_LECTURE           = 0, /**< Ouverture en lecture seule. */
    DUAL_SAVE_MODE_ECRITURE          = 1, /**< Ouverture en écriture, écrase le contenu existant (de façon atomique, voir note du fichier). */
    DUAL_SAVE_MODE_LECTURE_ECRITURE  = 2  /**< Ouverture en lecture et écriture, en préservant le contenu existant. */
} DUAL_SaveMode;

/**
 * @brief État de présence et de validité de la cartouche physique.
 */
typedef enum DUAL_CartridgeStatus {
    DUAL_CARTRIDGE_ABSENTE     = 0, /**< Aucune cartouche détectée dans le lecteur. */
    DUAL_CARTRIDGE_VALIDE      = 1, /**< Cartouche détectée et lisible. */
    DUAL_CARTRIDGE_CORROMPUE   = 2  /**< Cartouche détectée mais données illisibles ou invalides. */
} DUAL_CartridgeStatus;

/* ============================================================================
 *  Structures
 * ========================================================================== */

/**
 * @brief Métadonnées d'un emplacement de sauvegarde, utilisées notamment pour
 *        afficher un écran de sélection de sauvegarde au joueur.
 */
typedef struct DUAL_SaveSlotInfo {
    int32_t  index_slot;                /**< Index de l'emplacement de sauvegarde. */
    bool     existe;                    /**< Indique si une sauvegarde existe déjà à cet emplacement. */
    uint64_t taille_octets;             /**< Taille du fichier de sauvegarde, en octets. */
    int64_t  horodatage_dernier_acces;  /**< Timestamp Unix du dernier accès en écriture. */
} DUAL_SaveSlotInfo;

/* ============================================================================
 *  Configuration du module
 * ========================================================================== */

/**
 * @brief Définit le dossier utilisé pour stocker les fichiers de sauvegarde.
 *
 * À appeler une seule fois au démarrage du jeu, avant tout appel à
 * DUAL_SaveFile_Open(). Si cette fonction n'est jamais appelée, le dossier
 * par défaut "saves/" (relatif au dossier d'exécution) est utilisé.
 *
 * Le dossier est créé automatiquement s'il n'existe pas déjà (un seul niveau :
 * le dossier parent doit déjà exister).
 *
 * @param chemin_dossier Chemin du dossier de sauvegarde (le séparateur final
 *                        '/' est optionnel).
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon
 *         (chemin invalide/trop long, ou dossier impossible à créer).
 */

#ifdef __cplusplus
}
#endif

#endif /* DUAL_FS_H */