/**
 * @file dual_audio.h
 * @brief Module audio de libdual : effets sonores chargés intégralement en
 * RAM et musiques diffusées en streaming depuis le système de fichiers.
 *
 * Backend : miniaudio (ma_engine). Les DUAL_Sound sont joués en mode
 * "fire-and-forget" via un pool interne d'instances ma_sound gérées par
 * le DUAL_AudioManager ; l'appelant ne récupère jamais de handle d'instance.
 * Les DUAL_Music sont des flux uniques (streaming disque) que l'appelant
 * pilote directement (Play/Pause/Stop/Seek).
 */

#ifndef DUAL_AUDIO_H
#define DUAL_AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include "dual_core.h"
#include "dual_resources.h"

#ifdef __cplusplus
extern "C" {
#endif

// FAIRE DES FONCTIONS POUR JOUER DES SONS SELON LA DISTANCE

/* ============================================================================
 * Types opaques
 * ========================================================================== */

typedef struct DUAL_AudioManager DUAL_AudioManager;
typedef struct DUAL_Sound DUAL_Sound;
typedef struct DUAL_Music DUAL_Music;

/* ============================================================================
 * Enumérations
 * ========================================================================== */

typedef enum DUAL_AudioChannel {
    DUAL_CHANNEL_MASTER = 0,
    DUAL_CHANNEL_SFX    = 1,
    DUAL_CHANNEL_MUSIC  = 2,
    DUAL_CHANNEL_VOICE  = 3
} DUAL_AudioChannel;

/* ============================================================================
 * Cycle de vie du gestionnaire audio
 * ========================================================================== */

/**
 * @brief Crée le gestionnaire audio (moteur miniaudio + groupes de canaux +
 *        pool d'instances de sons).
 *
 * @param app Instance de l'application libdual associée.
 * @param out_audio Pointeur recevant l'instance créée en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_AudioManager_Create(DUAL_App* app, DUAL_AudioManager** out_audio);

/**
 * @brief Détruit le gestionnaire audio : arrête toutes les instances actives
 *        du pool, détruit les groupes de canaux et le moteur miniaudio.
 *
 * @note Ne libère PAS les DUAL_Sound / DUAL_Music encore trackés par le
 *       DUAL_ResourceManager : ceux-ci doivent être détruits explicitement
 *       (ou via DUAL_ResourceManager_Destroy / PurgeAll) avant ou après,
 *       tant que le DUAL_AudioManager reste valide au moment de leur
 *       destruction.
 *
 * @param audio Gestionnaire audio à détruire.
 */
void DUAL_AudioManager_Destroy(DUAL_AudioManager* audio);

/**
 * @brief À appeler une fois par frame : recycle les instances de sons
 *        terminées dans le pool interne.
 *
 * @param audio Gestionnaire audio.
 */
void DUAL_AudioManager_Update(DUAL_AudioManager* audio);

/**
 * @brief Définit le volume d'un canal (0.0f = silence, 1.0f = volume nominal).
 *        Impacte instantanément toutes les sources rattachées à ce canal.
 *
 * @param audio Gestionnaire audio.
 * @param channel Canal ciblé.
 * @param volume Volume linéaire, typiquement dans [0.0f, 1.0f] (non clampé).
 */
void DUAL_AudioManager_SetChannelVolume(DUAL_AudioManager* audio, DUAL_AudioChannel channel, float volume);

/* ============================================================================
 * Effets sonores (SFX / Voix chargés en RAM)
 * ========================================================================== */

/**
 * @brief Décode intégralement un fichier audio en RAM (PCM brut) et
 *        l'enregistre auprès du DUAL_ResourceManager (catégorie
 *        DUAL_RESOURCE_SOUND, mémoire DUAL_MEMORY_RAM).
 *
 * Formats supportés : WAV, MP3, FLAC, OGG Vorbis, OGG Opus.
 *
 * @param audio Gestionnaire audio.
 * @param resources Gestionnaire de ressources pour le suivi mémoire.
 * @param chemin_fichier Chemin du fichier à décoder.
 * @param out_sound Pointeur recevant le son créé.
 * @return DUAL_OK en cas de succès, DUAL_ERROR_INVALID_ARG si un paramètre
 *         est NULL, DUAL_ERROR_OUT_OF_MEMORY si le budget RAM est dépassé
 *         ou si l'allocation échoue, DUAL_ERROR_IO si le fichier ne peut
 *         pas être décodé.
 */
DUAL_Result DUAL_Sound_LoadFromFile(DUAL_AudioManager* audio, DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Sound** out_sound);

/**
 * @brief Libère un DUAL_Sound : arrête toute instance du pool encore en
 *        train de le jouer, libère les données PCM et notifie le
 *        DUAL_ResourceManager (TRACK_RAM_FREE).
 *
 * @param resources Gestionnaire de ressources.
 * @param sound Son à détruire (peut être NULL).
 */
void DUAL_Sound_Destroy(DUAL_ResourceManager* resources, DUAL_Sound* sound);

/**
 * @brief Joue un son en mode fire-and-forget : une instance libre du pool
 *        interne est recyclée/attribuée, aucun handle n'est renvoyé à
 *        l'appelant.
 *
 * @param audio Gestionnaire audio.
 * @param sound Son à jouer (déjà chargé via DUAL_Sound_LoadFromFile).
 * @param volume Volume linéaire de l'instance.
 * @param pitch Pitch de l'instance (1.0f = normal).
 * @return DUAL_OK en cas de succès, DUAL_ERROR_INVALID_ARG si audio/sound est
 *         NULL, DUAL_ERROR_OUT_OF_MEMORY si le pool est saturé (aucun slot
 *         libre), ou un autre code d'erreur si miniaudio échoue en interne.
 */
DUAL_Result DUAL_Sound_Play(DUAL_AudioManager* audio, const DUAL_Sound* sound, float volume, float pitch);

/* ============================================================================
 * Musique (streaming)
 * ========================================================================== */

/**
 * @brief Ouvre un fichier audio en streaming (lecture progressive depuis le
 *        disque, sans décodage intégral en RAM) et l'enregistre auprès du
 *        DUAL_ResourceManager (catégorie DUAL_RESOURCE_MUSIC).
 *
 * Formats supportés : WAV, MP3, FLAC, OGG Vorbis, OGG Opus.
 *
 * @param audio Gestionnaire audio.
 * @param resources Gestionnaire de ressources pour le suivi mémoire.
 * @param chemin_fichier Chemin du fichier à ouvrir.
 * @param out_music Pointeur recevant la musique créée.
 * @return DUAL_OK en cas de succès, DUAL_ERROR_INVALID_ARG si un paramètre
 *         est NULL, DUAL_ERROR_OUT_OF_MEMORY si l'allocation échoue,
 *         DUAL_ERROR_IO si le fichier ne peut pas être ouvert.
 */
DUAL_Result DUAL_Music_OpenFromFile(DUAL_AudioManager* audio, DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Music** out_music);

/**
 * @brief Ferme un flux de musique et notifie le DUAL_ResourceManager
 *        (TRACK_RAM_FREE de la struct de suivi ; le flux lui-même n'est
 *        pas résident en RAM).
 *
 * @param resources Gestionnaire de ressources.
 * @param music Musique à fermer (peut être NULL).
 */
void DUAL_Music_Close(DUAL_ResourceManager* resources, DUAL_Music* music);

/**
 * @brief Démarre (ou reprend) la lecture d'une musique.
 *
 * @param audio Gestionnaire audio.
 * @param music Musique à jouer.
 * @param boucle Si vrai, la musique reboucle automatiquement à la fin.
 */
void DUAL_Music_Play(DUAL_AudioManager* audio, DUAL_Music* music, bool boucle);

/**
 * @brief Met en pause la lecture d'une musique (position conservée).
 *
 * @param audio Gestionnaire audio.
 * @param music Musique à mettre en pause.
 */
void DUAL_Music_Pause(DUAL_AudioManager* audio, DUAL_Music* music);

/**
 * @brief Arrête la lecture d'une musique et revient au début du flux.
 *
 * @param audio Gestionnaire audio.
 * @param music Musique à arrêter.
 */
void DUAL_Music_Stop(DUAL_AudioManager* audio, DUAL_Music* music);

/**
 * @brief Définit le volume d'une musique individuelle.
 *
 * @param audio Gestionnaire audio.
 * @param music Musique ciblée.
 * @param volume Volume linéaire.
 */
void DUAL_Music_SetVolume(DUAL_AudioManager* audio, DUAL_Music* music, float volume);

/**
 * @brief Indique si une musique est en cours de lecture.
 *
 * @param audio Gestionnaire audio.
 * @param music Musique interrogée.
 * @return true si la musique joue actuellement, false sinon (y compris si
 *         music est NULL).
 */
bool DUAL_Music_IsPlaying(const DUAL_AudioManager* audio, const DUAL_Music* music);

#ifdef __cplusplus
}
#endif

#endif // DUAL_AUDIO_H
