/**
 * @file dual_audio.h
 * @brief Module audio de libdual : effets sonores chargés intégralement en
 *        RAM et musiques diffusées en streaming depuis le système de fichiers.
 *
 * Le module distingue volontairement deux types de ressources audio :
 * - Les sons (DUAL_Sound) : courts, chargés entièrement en mémoire pour une
 *   latence de lecture minimale (effets de jeu).
 * - Les musiques (DUAL_Music) : longues, décodées progressivement en streaming
 *   pour limiter l'empreinte mémoire.
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

/* ============================================================================
 *  Types opaques
 * ========================================================================== */

/**
 * @brief Gestionnaire audio global, responsable du mixage et de la gestion
 *        des canaux de lecture.
 */
typedef struct DUAL_AudioManager DUAL_AudioManager;

/**
 * @brief Représente un effet sonore court, entièrement décodé et chargé en RAM.
 */
typedef struct DUAL_Sound DUAL_Sound;

/**
 * @brief Représente une piste musicale longue, diffusée en streaming depuis
 *        le système de fichiers plutôt que chargée intégralement en mémoire.
 */
typedef struct DUAL_Music DUAL_Music;

/**
 * @brief Handle représentant une instance de lecture active d'un DUAL_Sound.
 *
 * Permet de contrôler (volume, arrêt) une lecture spécifique parmi plusieurs
 * instances simultanées du même son.
 */
typedef struct DUAL_SoundInstance DUAL_SoundInstance;

/* ============================================================================
 *  Enumérations
 * ========================================================================== */

/**
 * @brief Catégorie de canal audio, utilisée pour le contrôle de volume groupé.
 */
typedef enum DUAL_AudioChannel {
    DUAL_CHANNEL_MASTER = 0, /**< Canal maître, affecte tous les autres canaux. */
    DUAL_CHANNEL_SFX    = 1, /**< Canal des effets sonores (DUAL_Sound). */
    DUAL_CHANNEL_MUSIC  = 2, /**< Canal de la musique (DUAL_Music). */
    DUAL_CHANNEL_VOICE  = 3  /**< Canal des voix/dialogues. */
} DUAL_AudioChannel;

/* ============================================================================
 *  Cycle de vie du gestionnaire audio
 * ========================================================================== */

/**
 * @brief Initialise le sous-système audio de libdual.
 *
 * @param app Instance de l'application libdual.
 * @param out_audio Pointeur recevant le gestionnaire audio créé en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_AudioManager_Create(DUAL_App* app, DUAL_AudioManager** out_audio);

/**
 * @brief Détruit le gestionnaire audio, arrêtant toutes les lectures en cours.
 *
 * @param audio Gestionnaire audio à détruire.
 */
void DUAL_AudioManager_Destroy(DUAL_AudioManager* audio);

/**
 * @brief Met à jour l'état interne du gestionnaire audio.
 *
 * Doit être appelée une fois par frame afin de permettre le décodage progressif
 * des flux musicaux en streaming et le nettoyage des instances de son terminées.
 *
 * @param audio Gestionnaire audio à mettre à jour.
 */
void DUAL_AudioManager_Update(DUAL_AudioManager* audio);

/**
 * @brief Définit le volume d'un canal audio donné.
 *
 * @param audio Gestionnaire audio.
 * @param canal Canal dont le volume doit être modifié.
 * @param volume Volume cible, entre 0.0 (silence) et 1.0 (volume maximal).
 */
void DUAL_AudioManager_SetChannelVolume(DUAL_AudioManager* audio, DUAL_AudioChannel canal,
                                          float volume);

/* ============================================================================
 *  Sons (chargés en RAM)
 * ========================================================================== */

/**
 * @brief Charge un effet sonore depuis un fichier, intégralement décodé en RAM.
 *
 * @param resources Gestionnaire de ressources utilisé pour suivre la consommation RAM.
 * @param chemin_fichier Chemin du fichier audio (WAV recommandé pour les effets courts).
 * @param out_sound Pointeur recevant le son chargé en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_Sound_LoadFromFile(DUAL_ResourceManager* resources,
                                     const char* chemin_fichier,
                                     DUAL_Sound** out_sound);

/**
 * @brief Libère un son chargé et la mémoire RAM associée.
 *
 * @param resources Gestionnaire de ressources ayant suivi l'allocation.
 * @param sound Son à libérer.
 */
void DUAL_Sound_Destroy(DUAL_ResourceManager* resources, DUAL_Sound* sound);

/**
 * @brief Joue un effet sonore une fois, en créant une nouvelle instance de lecture.
 *
 * Plusieurs instances du même DUAL_Sound peuvent être jouées simultanément
 * (par exemple plusieurs tirs successifs rapprochés).
 *
 * @param audio Gestionnaire audio.
 * @param sound Son à jouer.
 * @param volume Volume de lecture, entre 0.0 et 1.0.
 * @param pitch Hauteur de lecture, 1.0 correspondant à la hauteur native du son.
 * @return Handle de l'instance de lecture créée, ou NULL en cas d'échec.
 */
DUAL_SoundInstance* DUAL_Sound_Play(DUAL_AudioManager* audio, const DUAL_Sound* sound,
                                     float volume, float pitch);

/**
 * @brief Arrête immédiatement une instance de lecture d'un son.
 *
 * @param audio Gestionnaire audio.
 * @param instance Instance de lecture à arrêter. Devient invalide après l'appel.
 */
void DUAL_SoundInstance_Stop(DUAL_AudioManager* audio, DUAL_SoundInstance* instance);

/**
 * @brief Indique si une instance de lecture de son est toujours en cours.
 *
 * @param audio Gestionnaire audio.
 * @param instance Instance de lecture à interroger.
 * @return true si le son est encore en cours de lecture, false s'il est terminé.
 */
bool DUAL_SoundInstance_IsPlaying(const DUAL_AudioManager* audio,
                                   const DUAL_SoundInstance* instance);

/* ============================================================================
 *  Musique (streaming)
 * ========================================================================== */

/**
 * @brief Ouvre une piste musicale pour une lecture en streaming depuis le
 *        système de fichiers.
 *
 * Contrairement à DUAL_Sound_LoadFromFile(), cette fonction ne décode pas
 * l'intégralité du fichier en mémoire : les données sont lues et décodées
 * progressivement pendant la lecture.
 *
 * @param resources Gestionnaire de ressources utilisé pour suivre la consommation RAM du buffer de streaming.
 * @param chemin_fichier Chemin du fichier audio (format compressé recommandé pour les pistes longues).
 * @param out_music Pointeur recevant la musique ouverte en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_Music_OpenFromFile(DUAL_ResourceManager* resources,
                                     const char* chemin_fichier,
                                     DUAL_Music** out_music);

/**
 * @brief Ferme une piste musicale et libère le buffer de streaming associé.
 *
 * @param resources Gestionnaire de ressources ayant suivi l'allocation.
 * @param music Musique à fermer.
 */
void DUAL_Music_Close(DUAL_ResourceManager* resources, DUAL_Music* music);

/**
 * @brief Démarre ou reprend la lecture d'une piste musicale.
 *
 * @param audio Gestionnaire audio.
 * @param music Musique à jouer.
 * @param boucle Si vrai, la piste reprend automatiquement depuis le début à sa fin.
 */
void DUAL_Music_Play(DUAL_AudioManager* audio, DUAL_Music* music, bool boucle);

/**
 * @brief Met en pause la lecture d'une piste musicale.
 *
 * @param audio Gestionnaire audio.
 * @param music Musique à mettre en pause.
 */
void DUAL_Music_Pause(DUAL_AudioManager* audio, DUAL_Music* music);

/**
 * @brief Arrête complètement la lecture d'une piste musicale et remet sa
 *        position de lecture au début.
 *
 * @param audio Gestionnaire audio.
 * @param music Musique à arrêter.
 */
void DUAL_Music_Stop(DUAL_AudioManager* audio, DUAL_Music* music);

/**
 * @brief Définit le volume de lecture d'une piste musicale spécifique.
 *
 * @param audio Gestionnaire audio.
 * @param music Musique dont le volume doit être modifié.
 * @param volume Volume cible, entre 0.0 (silence) et 1.0 (volume maximal).
 */
void DUAL_Music_SetVolume(DUAL_AudioManager* audio, DUAL_Music* music, float volume);

/**
 * @brief Indique si une piste musicale est actuellement en cours de lecture.
 *
 * @param audio Gestionnaire audio.
 * @param music Musique à interroger.
 * @return true si la musique est en cours de lecture, false sinon.
 */
bool DUAL_Music_IsPlaying(const DUAL_AudioManager* audio, const DUAL_Music* music);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_AUDIO_H */
