/**
 * @file dual_audio.h
 * @brief Module audio de libdual : effets sonores chargés intégralement en
 * RAM et musiques diffusées en streaming depuis le système de fichiers.
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
 * Types opaques
 * ========================================================================== */

typedef struct DUAL_AudioManager DUAL_AudioManager;
typedef struct DUAL_Sound DUAL_Sound;
typedef struct DUAL_Music DUAL_Music;
typedef struct DUAL_SoundInstance DUAL_SoundInstance;

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

DUAL_Result DUAL_AudioManager_Create(DUAL_App* app, DUAL_AudioManager** out_audio);
void DUAL_AudioManager_Destroy(DUAL_AudioManager* audio);
void DUAL_AudioManager_Update(DUAL_AudioManager* audio);
void DUAL_AudioManager_SetChannelVolume(DUAL_AudioManager* audio, DUAL_AudioChannel channel, float volume);

/* ============================================================================
 * Effets sonores (SFX chargé en RAM)
 * ========================================================================== */

DUAL_Result DUAL_Sound_LoadFromFile(DUAL_AudioManager* audio, DUAL_ResourceManager* resources,
                                     const char* chemin_fichier, DUAL_Sound** out_sound);

void DUAL_Sound_Destroy(DUAL_ResourceManager* resources, DUAL_Sound* sound);

DUAL_SoundInstance* DUAL_Sound_Play(DUAL_AudioManager* audio, const DUAL_Sound* sound,
                                     float volume, float pitch);

void DUAL_SoundInstance_Play(DUAL_AudioManager* audio, DUAL_SoundInstance* instance);
void DUAL_SoundInstance_Stop(DUAL_AudioManager* audio, DUAL_SoundInstance* instance);
bool DUAL_SoundInstance_IsPlaying(const DUAL_AudioManager* audio, const DUAL_SoundInstance* instance);

/* ============================================================================
 * Musique (streaming)
 * ========================================================================== */

DUAL_Result DUAL_Music_OpenFromFile(DUAL_AudioManager* audio, DUAL_ResourceManager* resources,
                                     const char* chemin_fichier, DUAL_Music** out_music);

void DUAL_Music_Close(DUAL_ResourceManager* resources, DUAL_Music* music);

void DUAL_Music_Play(DUAL_AudioManager* audio, DUAL_Music* music, bool boucle);
void DUAL_Music_Pause(DUAL_AudioManager* audio, DUAL_Music* music);
void DUAL_Music_Stop(DUAL_AudioManager* audio, DUAL_Music* music);
void DUAL_Music_SetVolume(DUAL_AudioManager* audio, DUAL_Music* music, float volume);
bool DUAL_Music_IsPlaying(const DUAL_AudioManager* audio, const DUAL_Music* music);

#ifdef __cplusplus
}
#endif

#endif // DUAL_AUDIO_H