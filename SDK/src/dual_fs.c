/**
 * @file dual_fs.c
 * @brief Implémentation du système de fichiers de libdual.
 */

#include "dual_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <direct.h>
    #include <windows.h>
    #define DUAL_FS_MKDIR(chemin) _mkdir(chemin)
#else
    #define DUAL_FS_MKDIR(chemin) mkdir((chemin), 0755)
#endif

#ifndef S_ISDIR
    #define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

/* ============================================================================
 *  Définitions internes
 * ========================================================================== */

#define DUAL_FS_DIR_MAX_LEN  256
#define DUAL_FS_PATH_MAX_LEN 512

struct DUAL_SaveFile {
    FILE* handle;
    char  chemin_final[DUAL_FS_PATH_MAX_LEN];       /* Fichier de sauvegarde "officiel" */
    char  chemin_temporaire[DUAL_FS_PATH_MAX_LEN];  /* Fichier de travail (écriture atomique) */
    bool  ecriture_atomique;                        /* true si un rename() est nécessaire à la fermeture */
};

struct DUAL_Cartridge {
    char base_path[DUAL_FS_DIR_MAX_LEN];
};

/* Dossiers configurables via DUAL_FS_SetSaveDirectory() / DUAL_FS_SetCartridgeRoot().
 * Valeurs par défaut relatives au dossier d'exécution du jeu. */
static char s_chemin_sauvegardes[DUAL_FS_DIR_MAX_LEN] = "saves/";
static char s_chemin_cartouche[DUAL_FS_DIR_MAX_LEN]   = "cartridge_data/";

/* ----------------------------------------------------------------------------
 *  Helpers de chemins
 * -------------------------------------------------------------------------- */

/* Normalise un chemin de dossier fourni par l'utilisateur : garantit un '/'
 * final et vérifie qu'il tient dans le buffer de destination. */
static DUAL_Result normalize_dir_path(const char* chemin_source, char* out, size_t out_len) {
    size_t len = strlen(chemin_source);
    char dernier = (len > 0) ? chemin_source[len - 1] : '\0';
    int ecrit;

    if (dernier == '/' || dernier == '\\') {
        ecrit = snprintf(out, out_len, "%s", chemin_source);
    } else {
        ecrit = snprintf(out, out_len, "%s/", chemin_source);
    }

    if (ecrit < 0 || (size_t)ecrit >= out_len) {
        return DUAL_ERROR_INVALID_ARG; /* Chemin trop long */
    }
    return DUAL_OK;
}

/* Crée un dossier s'il n'existe pas déjà (ne crée qu'un seul niveau : le
 * dossier parent doit déjà exister). */
static DUAL_Result ensure_directory_exists(const char* chemin_dossier) {
    if (DUAL_FS_MKDIR(chemin_dossier) != 0 && errno != EEXIST) {
        return DUAL_ERROR_UNKNOWN;
    }
    return DUAL_OK;
}

/* Génère le chemin du fichier de sauvegarde final pour un slot donné. */
static void build_save_path(int32_t slot, char* out_path, size_t max_len) {
    snprintf(out_path, max_len, "%ssave_slot_%d.sav", s_chemin_sauvegardes, slot);
}

/* Génère le chemin du fichier temporaire utilisé pendant une écriture. */
static void build_save_temp_path(int32_t slot, char* out_path, size_t max_len) {
    snprintf(out_path, max_len, "%ssave_slot_%d.sav.tmp", s_chemin_sauvegardes, slot);
}

/* Construit un chemin (simulé) dans la cartouche à partir d'un chemin relatif. */
static void build_cartridge_path(const char* base, const char* local_path, char* out_path, size_t max_len) {
    /* Évite les doubles slashes si l'utilisateur met un slash au début de son chemin */
    if (local_path[0] == '/' || local_path[0] == '\\') {
        local_path++;
    }
    snprintf(out_path, max_len, "%s%s", base, local_path);
}

/* Empêche un jeu de sortir du dossier sandboxé de la cartouche via un chemin
 * du type "../../../etc/passwd". */
static bool est_chemin_cartouche_valide(const char* chemin_relatif) {
    if (!chemin_relatif || chemin_relatif[0] == '\0') return false;
    return strstr(chemin_relatif, "..") == NULL;
}

/* Remplace atomiquement le fichier final par le fichier temporaire. */
static bool atomic_replace(const char* chemin_temporaire, const char* chemin_final) {
#ifdef _WIN32
    return MoveFileExA(chemin_temporaire, chemin_final, MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return rename(chemin_temporaire, chemin_final) == 0;
#endif
}

/* ============================================================================
 *  Configuration du module
 * ========================================================================== */

DUAL_Result DUAL_FS_SetSaveDirectory(const char* chemin_dossier) {
    if (!chemin_dossier || chemin_dossier[0] == '\0') return DUAL_ERROR_INVALID_ARG;

    char temp[DUAL_FS_DIR_MAX_LEN];
    DUAL_Result resultat = normalize_dir_path(chemin_dossier, temp, sizeof(temp));
    if (resultat != DUAL_OK) return resultat;

    memcpy(s_chemin_sauvegardes, temp, sizeof(s_chemin_sauvegardes));
    return ensure_directory_exists(s_chemin_sauvegardes);
}

DUAL_Result DUAL_FS_SetCartridgeRoot(const char* chemin_dossier) {
    if (!chemin_dossier || chemin_dossier[0] == '\0') return DUAL_ERROR_INVALID_ARG;

    char temp[DUAL_FS_DIR_MAX_LEN];
    DUAL_Result resultat = normalize_dir_path(chemin_dossier, temp, sizeof(temp));
    if (resultat != DUAL_OK) return resultat;

    /* Volontairement pas de création de dossier ici : son absence signifie
     * "pas de cartouche insérée" (voir DUAL_Cartridge_GetStatus()). */
    memcpy(s_chemin_cartouche, temp, sizeof(s_chemin_cartouche));
    return DUAL_OK;
}

const char* DUAL_FS_GetSaveDirectory(void) {
    return s_chemin_sauvegardes;
}

const char* DUAL_FS_GetCartridgeRoot(void) {
    return s_chemin_cartouche;
}

/* ============================================================================
 *  Sauvegardes
 * ========================================================================== */

DUAL_Result DUAL_SaveFile_Open(int32_t index_slot, DUAL_SaveMode mode, DUAL_SaveFile** out_save) {
    if (!out_save) return DUAL_ERROR_INVALID_ARG;
    *out_save = NULL;

    DUAL_SaveFile* save = (DUAL_SaveFile*)calloc(1, sizeof(DUAL_SaveFile));
    if (!save) return DUAL_ERROR_OUT_OF_MEMORY;

    build_save_path(index_slot, save->chemin_final, sizeof(save->chemin_final));

    if (mode == DUAL_SAVE_MODE_LECTURE) {
        /* Lecture seule : pas besoin d'écriture atomique, on ouvre directement
         * le fichier final. */
        save->ecriture_atomique = false;
        save->handle = fopen(save->chemin_final, "rb");

        if (!save->handle) {
            free(save);
            return DUAL_ERROR_NOT_FOUND;
        }
    } else {
        /* DUAL_SAVE_MODE_ECRITURE ou DUAL_SAVE_MODE_LECTURE_ECRITURE :
         * on travaille sur un fichier temporaire, remplacé atomiquement à la
         * fermeture (voir DUAL_SaveFile_Close). Ainsi, un crash ou une coupure
         * de courant pendant l'écriture ne corrompt jamais la sauvegarde existante. */
        ensure_directory_exists(s_chemin_sauvegardes);
        build_save_temp_path(index_slot, save->chemin_temporaire, sizeof(save->chemin_temporaire));
        save->ecriture_atomique = true;

        if (mode == DUAL_SAVE_MODE_LECTURE_ECRITURE) {
            /* On préserve le contenu existant en le recopiant dans le fichier
             * temporaire avant de l'ouvrir en lecture/écriture. */
            FILE* source = fopen(save->chemin_final, "rb");
            FILE* dest = fopen(save->chemin_temporaire, "wb");

            if (!dest) {
                if (source) fclose(source);
                free(save);
                return DUAL_ERROR_UNKNOWN;
            }

            if (source) {
                char tampon[4096];
                size_t lus;
                while ((lus = fread(tampon, 1, sizeof(tampon), source)) > 0) {
                    fwrite(tampon, 1, lus, dest);
                }
                fclose(source);
            }
            fclose(dest);

            save->handle = fopen(save->chemin_temporaire, "r+b");
        } else {
            /* DUAL_SAVE_MODE_ECRITURE : on repart d'un fichier temporaire vide. */
            save->handle = fopen(save->chemin_temporaire, "wb");
        }

        if (!save->handle) {
            remove(save->chemin_temporaire);
            free(save);
            return DUAL_ERROR_NOT_FOUND;
        }
    }

    *out_save = save;
    return DUAL_OK;
}

DUAL_Result DUAL_SaveFile_Close(DUAL_SaveFile* save) {
    if (!save) return DUAL_ERROR_INVALID_ARG;

    DUAL_Result resultat = DUAL_OK;

    if (save->handle) {
        fflush(save->handle);
        fclose(save->handle);
        save->handle = NULL;
    }

    if (save->ecriture_atomique) {
        /* C'est ici, et seulement ici, que la sauvegarde devient "officielle". */
        if (!atomic_replace(save->chemin_temporaire, save->chemin_final)) {
            resultat = DUAL_ERROR_UNKNOWN;
            /* Le fichier temporaire est volontairement conservé : il pourra
             * servir à une récupération manuelle en cas de problème. */
        }
    }

    free(save);
    return resultat;
}

DUAL_Result DUAL_SaveFile_Write(DUAL_SaveFile* save, const void* donnees, uint64_t taille_octets) {
    if (!save || !save->handle || !donnees) return DUAL_ERROR_INVALID_ARG;

    size_t written = fwrite(donnees, 1, (size_t)taille_octets, save->handle);
    if (written != taille_octets) return DUAL_ERROR_UNKNOWN;

    return DUAL_OK;
}

DUAL_Result DUAL_SaveFile_Read(DUAL_SaveFile* save, void* out_buffer, uint64_t taille_octets, uint64_t* out_octets_lus) {
    if (!save || !save->handle || !out_buffer) return DUAL_ERROR_INVALID_ARG;

    size_t read_bytes = fread(out_buffer, 1, (size_t)taille_octets, save->handle);

    if (out_octets_lus) {
        *out_octets_lus = (uint64_t)read_bytes;
    }

    if (read_bytes != taille_octets && !feof(save->handle)) {
        return DUAL_ERROR_UNKNOWN; /* Erreur de lecture (autre que la fin de fichier) */
    }

    return DUAL_OK;
}

DUAL_Result DUAL_SaveFile_Seek(DUAL_SaveFile* save, uint64_t offset_octets) {
    if (!save || !save->handle) return DUAL_ERROR_INVALID_ARG;

    /* Note : fseek standard utilise un 'long' (limité à 2 Go sur certains
     * systèmes, notamment Windows). Suffisant pour des sauvegardes de jeu. */
    if (fseek(save->handle, (long)offset_octets, SEEK_SET) != 0) {
        return DUAL_ERROR_UNKNOWN;
    }
    return DUAL_OK;
}

DUAL_Result DUAL_SaveFile_GetSize(DUAL_SaveFile* save, uint64_t* out_taille_octets) {
    if (!save || !save->handle || !out_taille_octets) return DUAL_ERROR_INVALID_ARG;

    long position_actuelle = ftell(save->handle);
    if (position_actuelle < 0) return DUAL_ERROR_UNKNOWN;

    if (fseek(save->handle, 0, SEEK_END) != 0) return DUAL_ERROR_UNKNOWN;

    long taille = ftell(save->handle);
    if (taille < 0) return DUAL_ERROR_UNKNOWN;

    /* On restaure la position de lecture/écriture d'origine. */
    fseek(save->handle, position_actuelle, SEEK_SET);

    *out_taille_octets = (uint64_t)taille;
    return DUAL_OK;
}

DUAL_Result DUAL_SaveFile_Delete(int32_t index_slot) {
    char chemin[DUAL_FS_PATH_MAX_LEN];
    build_save_path(index_slot, chemin, sizeof(chemin));

    /* Nettoie également un éventuel fichier temporaire laissé par une écriture interrompue. */
    char chemin_temp[DUAL_FS_PATH_MAX_LEN];
    build_save_temp_path(index_slot, chemin_temp, sizeof(chemin_temp));
    remove(chemin_temp);

    if (remove(chemin) == 0) {
        return DUAL_OK;
    }
    return DUAL_ERROR_NOT_FOUND;
}

DUAL_Result DUAL_SaveFile_GetSlotInfo(int32_t index_slot, DUAL_SaveSlotInfo* out_info) {
    if (!out_info) return DUAL_ERROR_INVALID_ARG;

    char chemin[DUAL_FS_PATH_MAX_LEN];
    build_save_path(index_slot, chemin, sizeof(chemin));

    struct stat st;
    out_info->index_slot = index_slot;

    if (stat(chemin, &st) == 0) {
        out_info->existe = true;
        out_info->taille_octets = (uint64_t)st.st_size;
        out_info->horodatage_dernier_acces = (int64_t)st.st_mtime;
        return DUAL_OK;
    }

    out_info->existe = false;
    out_info->taille_octets = 0;
    out_info->horodatage_dernier_acces = 0;
    return DUAL_OK; /* L'opération a réussi (il n'y a juste pas de sauvegarde) */
}

/* ============================================================================
 *  Cartouche physique
 * ========================================================================== */

DUAL_CartridgeStatus DUAL_Cartridge_GetStatus(void) {
    struct stat st;
    if (stat(s_chemin_cartouche, &st) == 0 && S_ISDIR(st.st_mode)) {
        /* En vrai hardware, on vérifierait l'en-tête, la signature ROM, etc.
         * Ici, on vérifie juste qu'il s'agit bien d'un dossier. */
        return DUAL_CARTRIDGE_VALIDE;
    }
    return DUAL_CARTRIDGE_ABSENTE;
}

DUAL_Result DUAL_Cartridge_Open(DUAL_Cartridge** out_cartridge) {
    if (!out_cartridge) return DUAL_ERROR_INVALID_ARG;
    *out_cartridge = NULL;

    if (DUAL_Cartridge_GetStatus() != DUAL_CARTRIDGE_VALIDE) {
        return DUAL_ERROR_NOT_FOUND;
    }

    DUAL_Cartridge* cart = (DUAL_Cartridge*)malloc(sizeof(DUAL_Cartridge));
    if (!cart) return DUAL_ERROR_OUT_OF_MEMORY;

    snprintf(cart->base_path, sizeof(cart->base_path), "%s", s_chemin_cartouche);
    *out_cartridge = cart;

    return DUAL_OK;
}

void DUAL_Cartridge_Close(DUAL_Cartridge* cartridge) {
    if (cartridge) {
        free(cartridge);
    }
}

DUAL_Result DUAL_Cartridge_ReadFile(DUAL_Cartridge* cartridge, const char* chemin_dans_cartouche, void* out_buffer, uint64_t taille_buffer, uint64_t* out_octets_lus) {
    if (!cartridge || !chemin_dans_cartouche || !out_buffer) return DUAL_ERROR_INVALID_ARG;
    if (!est_chemin_cartouche_valide(chemin_dans_cartouche)) return DUAL_ERROR_INVALID_ARG;

    char chemin[DUAL_FS_PATH_MAX_LEN];
    build_cartridge_path(cartridge->base_path, chemin_dans_cartouche, chemin, sizeof(chemin));

    FILE* f = fopen(chemin, "rb");
    if (!f) return DUAL_ERROR_NOT_FOUND;

    size_t read_bytes = fread(out_buffer, 1, (size_t)taille_buffer, f);
    if (out_octets_lus) {
        *out_octets_lus = (uint64_t)read_bytes;
    }

    fclose(f);
    return DUAL_OK;
}

DUAL_Result DUAL_Cartridge_GetFileSize(DUAL_Cartridge* cartridge, const char* chemin_dans_cartouche, uint64_t* out_taille_octets) {
    if (!cartridge || !chemin_dans_cartouche || !out_taille_octets) return DUAL_ERROR_INVALID_ARG;
    if (!est_chemin_cartouche_valide(chemin_dans_cartouche)) return DUAL_ERROR_INVALID_ARG;

    char chemin[DUAL_FS_PATH_MAX_LEN];
    build_cartridge_path(cartridge->base_path, chemin_dans_cartouche, chemin, sizeof(chemin));

    struct stat st;
    if (stat(chemin, &st) == 0) {
        *out_taille_octets = (uint64_t)st.st_size;
        return DUAL_OK;
    }

    return DUAL_ERROR_NOT_FOUND;
}