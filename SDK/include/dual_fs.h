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
DUAL_Result DUAL_FS_SetSaveDirectory(const char* chemin_dossier);

/**
 * @brief Définit le dossier représentant la cartouche physique insérée.
 *
 * À appeler une seule fois au démarrage du jeu. Si cette fonction n'est jamais
 * appelée, le dossier par défaut "cartridge_data/" (relatif au dossier
 * d'exécution) est utilisé. Contrairement au dossier de sauvegarde, ce dossier
 * n'est jamais créé automatiquement : son absence signifie simplement
 * "aucune cartouche insérée" (voir DUAL_Cartridge_GetStatus()).
 *
 * @param chemin_dossier Chemin du dossier représentant la cartouche.
 * @return DUAL_OK en cas de succès, ou DUAL_ERROR_INVALID_ARG si le chemin
 *         est invalide ou trop long.
 */
DUAL_Result DUAL_FS_SetCartridgeRoot(const char* chemin_dossier);

/**
 * @brief Retourne le dossier de sauvegarde actuellement configuré.
 * @return Chaîne interne en lecture seule, valide jusqu'au prochain appel à
 *         DUAL_FS_SetSaveDirectory().
 */
const char* DUAL_FS_GetSaveDirectory(void);

/**
 * @brief Retourne le dossier de cartouche actuellement configuré.
 * @return Chaîne interne en lecture seule, valide jusqu'au prochain appel à
 *         DUAL_FS_SetCartridgeRoot().
 */
const char* DUAL_FS_GetCartridgeRoot(void);

/* ============================================================================
 *  Sauvegardes
 * ========================================================================== */

/**
 * @brief Ouvre (ou crée) un fichier de sauvegarde identifié par un numéro
 *        d'emplacement (slot).
 *
 * En mode DUAL_SAVE_MODE_ECRITURE ou DUAL_SAVE_MODE_LECTURE_ECRITURE, les
 * écritures sont effectuées dans un fichier temporaire et ne remplacent le
 * fichier de sauvegarde final qu'à la fermeture (voir DUAL_SaveFile_Close()).
 *
 * @param index_slot Index de l'emplacement de sauvegarde (généralement entre 0 et N-1).
 * @param mode Mode d'ouverture souhaité.
 * @param out_save Pointeur recevant le fichier de sauvegarde ouvert en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_SaveFile_Open(int32_t index_slot, DUAL_SaveMode mode, DUAL_SaveFile** out_save);

/**
 * @brief Ferme un fichier de sauvegarde et finalise l'écriture éventuelle.
 *
 * Si le fichier a été ouvert en écriture, cette fonction remplace le fichier
 * de sauvegarde final par le contenu écrit, de façon atomique. C'est
 * uniquement à cet instant que la sauvegarde devient visible pour
 * DUAL_SaveFile_GetSlotInfo() et les prochaines ouvertures en lecture.
 *
 * @param save Fichier de sauvegarde à fermer.
 * @return DUAL_OK si la fermeture (et la finalisation de l'écriture, le cas
 *         échéant) a réussi. DUAL_ERROR_UNKNOWN si l'écriture n'a pas pu être
 *         finalisée : IMPORTANT, un retour d'erreur ici signifie que la
 *         sauvegarde n'a PAS été mise à jour, à vérifier systématiquement
 *         pour toute donnée importante. DUAL_ERROR_INVALID_ARG si `save` est NULL.
 */
DUAL_Result DUAL_SaveFile_Close(DUAL_SaveFile* save);

/**
 * @brief Écrit un bloc de données brutes dans le fichier de sauvegarde, à la
 *        position courante.
 *
 * @param save Fichier de sauvegarde ouvert en écriture.
 * @param donnees Pointeur vers les données à écrire.
 * @param taille_octets Taille des données à écrire, en octets.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_SaveFile_Write(DUAL_SaveFile* save, const void* donnees, uint64_t taille_octets);

/**
 * @brief Lit un bloc de données brutes depuis le fichier de sauvegarde, à
 *        partir de la position courante.
 *
 * @param save Fichier de sauvegarde ouvert en lecture.
 * @param out_buffer Buffer destination recevant les données lues.
 * @param taille_octets Nombre d'octets à lire.
 * @param out_octets_lus Pointeur recevant le nombre réel d'octets lus (peut être NULL).
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_SaveFile_Read(DUAL_SaveFile* save, void* out_buffer, uint64_t taille_octets,
                                uint64_t* out_octets_lus);

/**
 * @brief Déplace la position de lecture/écriture courante au sein du fichier de sauvegarde.
 *
 * @param save Fichier de sauvegarde ouvert.
 * @param offset_octets Nouvelle position, en octets, depuis le début du fichier.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_SaveFile_Seek(DUAL_SaveFile* save, uint64_t offset_octets);

/**
 * @brief Récupère la taille actuelle du fichier de sauvegarde ouvert.
 *
 * Utile pour allouer un buffer de lecture lorsque la taille exacte des
 * données sauvegardées n'est pas connue à l'avance. Préserve la position de
 * lecture/écriture courante.
 *
 * @param save Fichier de sauvegarde ouvert.
 * @param out_taille_octets Pointeur recevant la taille du fichier, en octets.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_SaveFile_GetSize(DUAL_SaveFile* save, uint64_t* out_taille_octets);

/**
 * @brief Supprime définitivement le fichier de sauvegarde d'un emplacement donné.
 *
 * @param index_slot Index de l'emplacement de sauvegarde à supprimer.
 * @return DUAL_OK en cas de succès, DUAL_ERROR_NOT_FOUND si l'emplacement était déjà vide.
 */
DUAL_Result DUAL_SaveFile_Delete(int32_t index_slot);

/**
 * @brief Récupère les métadonnées d'un emplacement de sauvegarde sans l'ouvrir.
 *
 * Utile pour construire un écran de sélection de sauvegarde affichant la
 * taille et la date de dernière modification de chaque emplacement.
 *
 * @param index_slot Index de l'emplacement de sauvegarde à interroger.
 * @param out_info Pointeur vers une structure DUAL_SaveSlotInfo à remplir.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_SaveFile_GetSlotInfo(int32_t index_slot, DUAL_SaveSlotInfo* out_info);

/* ============================================================================
 *  Cartouche physique
 * ========================================================================== */

/**
 * @brief Interroge l'état actuel du lecteur de cartouche physique.
 *
 * @return Statut courant de la cartouche (absente, valide, ou corrompue).
 */
DUAL_CartridgeStatus DUAL_Cartridge_GetStatus(void);

/**
 * @brief Ouvre un handle d'accès en lecture à la cartouche physique insérée.
 *
 * @param out_cartridge Pointeur recevant le handle ouvert en cas de succès.
 * @return DUAL_OK en cas de succès, DUAL_ERROR_NOT_FOUND si aucune cartouche
 *         n'est insérée, ou un autre code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_Cartridge_Open(DUAL_Cartridge** out_cartridge);

/**
 * @brief Ferme un handle d'accès à la cartouche physique.
 *
 * @param cartridge Handle à fermer.
 */
void DUAL_Cartridge_Close(DUAL_Cartridge* cartridge);

/**
 * @brief Lit un fichier de données présent sur la cartouche physique vers un
 *        buffer mémoire.
 *
 * @param cartridge Handle de cartouche ouvert.
 * @param chemin_dans_cartouche Chemin du fichier au sein du système de fichiers
 *        virtuel de la cartouche. Les chemins contenant ".." sont refusés.
 * @param out_buffer Buffer destination recevant les données lues.
 * @param taille_buffer Taille disponible du buffer destination, en octets.
 * @param out_octets_lus Pointeur recevant le nombre réel d'octets lus (peut être NULL).
 * @return DUAL_OK en cas de succès, DUAL_ERROR_NOT_FOUND si le fichier est
 *         introuvable sur la cartouche, DUAL_ERROR_INVALID_ARG si le chemin
 *         est invalide ou tente de sortir du dossier de la cartouche.
 */
DUAL_Result DUAL_Cartridge_ReadFile(DUAL_Cartridge* cartridge, const char* chemin_dans_cartouche,
                                     void* out_buffer, uint64_t taille_buffer,
                                     uint64_t* out_octets_lus);

/**
 * @brief Récupère la taille d'un fichier présent sur la cartouche physique,
 *        sans le lire intégralement.
 *
 * Utile pour allouer un buffer de taille exacte avant d'appeler
 * DUAL_Cartridge_ReadFile().
 *
 * @param cartridge Handle de cartouche ouvert.
 * @param chemin_dans_cartouche Chemin du fichier au sein du système de fichiers
 *        virtuel de la cartouche. Les chemins contenant ".." sont refusés.
 * @param out_taille_octets Pointeur recevant la taille du fichier, en octets.
 * @return DUAL_OK en cas de succès, DUAL_ERROR_NOT_FOUND si le fichier est
 *         introuvable, DUAL_ERROR_INVALID_ARG si le chemin est invalide.
 */
DUAL_Result DUAL_Cartridge_GetFileSize(DUAL_Cartridge* cartridge, const char* chemin_dans_cartouche,
                                        uint64_t* out_taille_octets);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_FS_H */