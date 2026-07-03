/**
 * @file dual_audio.c
 */

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "dual_audio.h"
#include <stdlib.h>
#include <string.h>

#define DUAL_MAX_SOUND_INSTANCES 256

struct DUAL_SoundInstance {
    ma_sound sound;
    bool active;
};

struct DUAL_AudioManager {
    ma_engine engine;
    ma_sound_group sfxGroup;
    ma_sound_group musicGroup;
    ma_sound_group voiceGroup;
    DUAL_SoundInstance instancePool[DUAL_MAX_SOUND_INSTANCES];
};

struct DUAL_Sound {
    void* pData;
    size_t dataSize;
    ma_audio_buffer buffer;
};

struct DUAL_Music {
    ma_sound sound;
    size_t streamBufferSize;
};

// --- Macros pour le ResourceManager (à relier à tes propres structures plus tard)
#define TRACK_RAM_ALLOC(res, size) /* TODO: res->audio_ram_usage += size; */
#define TRACK_RAM_FREE(res, size)  /* TODO: res->audio_ram_usage -= size; */


DUAL_Result DUAL_AudioManager_Create(DUAL_App* app, DUAL_AudioManager** out_audio) {
    DUAL_AudioManager* audio = (DUAL_AudioManager*)calloc(1, sizeof(DUAL_AudioManager));
    if (!audio) return -1;

    if (ma_engine_init(NULL, &audio->engine) != MA_SUCCESS) {
        free(audio);
        return -1;
    }

    ma_sound_group_init(&audio->engine, 0, NULL, &audio->sfxGroup);
    ma_sound_group_init(&audio->engine, 0, NULL, &audio->musicGroup);
    ma_sound_group_init(&audio->engine, 0, NULL, &audio->voiceGroup);

    *out_audio = audio;
    return 0;
}

void DUAL_AudioManager_Destroy(DUAL_AudioManager* audio) {
    if (!audio) return;
    ma_engine_uninit(&audio->engine); // Nettoie automatiquement les groupes et sons
    free(audio);
}

void DUAL_AudioManager_Update(DUAL_AudioManager* audio) {
    if (!audio) return;
    for (int i = 0; i < DUAL_MAX_SOUND_INSTANCES; ++i) {
        if (audio->instancePool[i].active) {
            if (ma_sound_at_end(&audio->instancePool[i].sound)) {
                ma_sound_uninit(&audio->instancePool[i].sound);
                audio->instancePool[i].active = false;
            }
        }
    }
}

void DUAL_AudioManager_SetChannelVolume(DUAL_AudioManager* audio, DUAL_AudioChannel canal, float volume) {
    if (!audio) return;
    switch (canal) {
        case DUAL_CHANNEL_MASTER: ma_engine_set_volume(&audio->engine, volume); break;
        case DUAL_CHANNEL_SFX:    ma_sound_group_set_volume(&audio->sfxGroup, volume); break;
        case DUAL_CHANNEL_MUSIC:  ma_sound_group_set_volume(&audio->musicGroup, volume); break;
        case DUAL_CHANNEL_VOICE:  ma_sound_group_set_volume(&audio->voiceGroup, volume); break;
    }
}

DUAL_Result DUAL_Sound_LoadFromFile(DUAL_AudioManager* audio, DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Sound** out_sound) {
    DUAL_Sound* sound = (DUAL_Sound*)calloc(1, sizeof(DUAL_Sound));

    ma_decoder decoder;
    if (ma_decoder_init_file(chemin_fichier, NULL, &decoder) != MA_SUCCESS) {
        free(sound);
        return -1;
    }

    ma_uint64 frameCount;
    ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    sound->dataSize = (size_t)frameCount * decoder.outputChannels * ma_get_bytes_per_sample(decoder.outputFormat);
    sound->pData = malloc(sound->dataSize);

    ma_decoder_read_pcm_frames(&decoder, sound->pData, frameCount, NULL);

    ma_audio_buffer_config bufferConfig = ma_audio_buffer_config_init(
        decoder.outputFormat, decoder.outputChannels, frameCount, sound->pData, NULL
    );
    ma_audio_buffer_init(&bufferConfig, &sound->buffer);
    ma_decoder_uninit(&decoder);

    TRACK_RAM_ALLOC(resources, sound->dataSize);

    *out_sound = sound;
    return 0;
}

void DUAL_Sound_Destroy(DUAL_ResourceManager* resources, DUAL_Sound* sound) {
    if (!sound) return;
    TRACK_RAM_FREE(resources, sound->dataSize);
    ma_audio_buffer_uninit(&sound->buffer);
    free(sound->pData);
    free(sound);
}

DUAL_SoundInstance* DUAL_Sound_Play(DUAL_AudioManager* audio, const DUAL_Sound* sound, float volume, float pitch) {
    if (!audio || !sound) return NULL;

    DUAL_SoundInstance* freeInstance = NULL;
    for (int i = 0; i < DUAL_MAX_SOUND_INSTANCES; ++i) {
        if (!audio->instancePool[i].active) {
            freeInstance = &audio->instancePool[i];
            break;
        }
    }
    if (!freeInstance) return NULL;

    // Initialise le son avec le buffer et l'attache au groupe SFX
    ma_sound_init_from_data_source(&audio->engine, &sound->buffer, 0, &audio->sfxGroup, &freeInstance->sound);
    ma_sound_set_volume(&freeInstance->sound, volume);
    ma_sound_set_pitch(&freeInstance->sound, pitch);
    ma_sound_start(&freeInstance->sound);

    freeInstance->active = true;
    return freeInstance;
}

void DUAL_SoundInstance_Stop(DUAL_AudioManager* audio, DUAL_SoundInstance* instance) {
    if (!instance || !instance->active) return;
    ma_sound_stop(&instance->sound);
    ma_sound_uninit(&instance->sound);
    instance->active = false;
}

bool DUAL_SoundInstance_IsPlaying(const DUAL_AudioManager* audio, const DUAL_SoundInstance* instance) {
    if (!instance || !instance->active) return false;
    return ma_sound_is_playing(&instance->sound);
}

DUAL_Result DUAL_Music_OpenFromFile(DUAL_AudioManager* audio, DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Music** out_music) {
    DUAL_Music* music = (DUAL_Music*)calloc(1, sizeof(DUAL_Music));
    music->streamBufferSize = 32768; // Estimation fixe pour le ResourceManager

    // MA_SOUND_FLAG_STREAM indique à miniaudio de lire en streaming, groupe MUSIC
    if (ma_sound_init_from_file(&audio->engine, chemin_fichier, MA_SOUND_FLAG_STREAM, &audio->musicGroup, NULL, &music->sound) != MA_SUCCESS) {
        free(music);
        return -1;
    }

    TRACK_RAM_ALLOC(resources, music->streamBufferSize);

    *out_music = music;
    return 0;
}

void DUAL_Music_Close(DUAL_ResourceManager* resources, DUAL_Music* music) {
    if (!music) return;
    TRACK_RAM_FREE(resources, music->streamBufferSize);
    ma_sound_uninit(&music->sound);
    free(music);
}

void DUAL_Music_Play(DUAL_AudioManager* audio, DUAL_Music* music, bool boucle) {
    if (!music){
        return;
    }
    ma_sound_set_looping(&music->sound, boucle);
    ma_sound_start(&music->sound);
}

void DUAL_Music_Pause(DUAL_AudioManager* audio, DUAL_Music* music) {
    if (!music) return;
    ma_sound_stop(&music->sound); // Stop met en pause par défaut sous miniaudio
}

void DUAL_Music_Stop(DUAL_AudioManager* audio, DUAL_Music* music) {
    if (!music) return;
    ma_sound_stop(&music->sound);
    ma_sound_seek_to_pcm_frame(&music->sound, 0); // Remise au début
}

void DUAL_Music_SetVolume(DUAL_AudioManager* audio, DUAL_Music* music, float volume) {
    if (!music) return;
    ma_sound_set_volume(&music->sound, volume);
}

bool DUAL_Music_IsPlaying(const DUAL_AudioManager* audio, const DUAL_Music* music) {
    if (!music) return false;
    return ma_sound_is_playing(&music->sound);
}