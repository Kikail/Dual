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

// Macros originales pour le suivi mémoire (laissées intactes)
#define TRACK_RAM_ALLOC(res, size) /* TODO: res->audio_ram_usage += size; */
#define TRACK_RAM_FREE(res, size)  /* TODO: res->audio_ram_usage -= size; */

DUAL_Result DUAL_AudioManager_Create(DUAL_App* app, DUAL_AudioManager** out_audio) {
    (void)app;
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

    for (int i = 0; i < DUAL_MAX_SOUND_INSTANCES; i++) {
        if (audio->instancePool[i].active) {
            ma_sound_uninit(&audio->instancePool[i].sound);
        }
    }

    ma_sound_group_uninit(&audio->voiceGroup);
    ma_sound_group_uninit(&audio->musicGroup);
    ma_sound_group_uninit(&audio->sfxGroup);
    ma_engine_uninit(&audio->engine);
    free(audio);
}

void DUAL_AudioManager_Update(DUAL_AudioManager* audio) {
    if (!audio) return;

    // Libération et recyclage des voix terminées en arrière-plan
    for (int i = 0; i < DUAL_MAX_SOUND_INSTANCES; i++) {
        if (audio->instancePool[i].active) {
            if (ma_sound_at_end(&audio->instancePool[i].sound)) {
                ma_sound_uninit(&audio->instancePool[i].sound);
                audio->instancePool[i].active = false;
            }
        }
    }
}

void DUAL_AudioManager_SetChannelVolume(DUAL_AudioManager* audio, DUAL_AudioChannel channel, float volume) {
    if (!audio) return;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    switch (channel) {
        case DUAL_CHANNEL_MASTER:
            ma_engine_set_volume(&audio->engine, volume);
            break;
        case DUAL_CHANNEL_SFX:
            ma_sound_group_set_volume(&audio->sfxGroup, volume);
            break;
        case DUAL_CHANNEL_MUSIC:
            ma_sound_group_set_volume(&audio->musicGroup, volume);
            break;
        case DUAL_CHANNEL_VOICE:
            ma_sound_group_set_volume(&audio->voiceGroup, volume);
            break;
    }
}

/* ============================================================================
 * Effets sonores (RAM)
 * ========================================================================== */

DUAL_Result DUAL_Sound_LoadFromFile(DUAL_AudioManager* audio, DUAL_ResourceManager* resources,
                                     const char* chemin_fichier, DUAL_Sound** out_sound) {
    if (!audio || !out_sound || !chemin_fichier) return -1;

    ma_decoder decoder;
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);

    if (ma_decoder_init_file(chemin_fichier, &decoderConfig, &decoder) != MA_SUCCESS) {
        return -1;
    }

    ma_uint64 totalFrameCount = 0;
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrameCount) != MA_SUCCESS) {
        ma_decoder_uninit(&decoder);
        return -1;
    }

    ma_uint32 channels = decoder.outputChannels;
    ma_uint32 sampleRate = decoder.outputSampleRate;
    size_t dataSize = totalFrameCount * channels * sizeof(float);

    void* pData = malloc(dataSize);
    if (!pData) {
        ma_decoder_uninit(&decoder);
        return -1;
    }

    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(&decoder, pData, totalFrameCount, &framesRead);
    ma_decoder_uninit(&decoder);

    DUAL_Sound* sound = (DUAL_Sound*)calloc(1, sizeof(DUAL_Sound));
    if (!sound) {
        free(pData);
        return -1;
    }

    sound->pData = pData;
    sound->dataSize = dataSize;

    ma_audio_buffer_config bufferConfig = ma_audio_buffer_config_init(ma_format_f32, channels, totalFrameCount, pData, NULL);
    if (ma_audio_buffer_init(&bufferConfig, &sound->buffer) != MA_SUCCESS) {
        free(sound->pData);
        free(sound);
        return -1;
    }

    TRACK_RAM_ALLOC(resources, dataSize);

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

    DUAL_SoundInstance* instance = NULL;

    // 1. Libération immédiate des instances inactives pour garantir des slots disponibles
    for (int i = 0; i < DUAL_MAX_SOUND_INSTANCES; i++) {
        if (audio->instancePool[i].active) {
            if (ma_sound_at_end(&audio->instancePool[i].sound)) {
                ma_sound_uninit(&audio->instancePool[i].sound);
                audio->instancePool[i].active = false;
            }
        }
    }

    // 2. Recherche d'un emplacement vide
    for (int i = 0; i < DUAL_MAX_SOUND_INSTANCES; i++) {
        if (!audio->instancePool[i].active) {
            instance = &audio->instancePool[i];
            break;
        }
    }

    if (!instance) return NULL;

    ma_sound_config config = ma_sound_config_init();
    config.pDataSource = (ma_data_source*)&((DUAL_Sound*)sound)->buffer;

    if (ma_sound_init_ex(&audio->engine, &config, &instance->sound) != MA_SUCCESS) {
        return NULL;
    }

    ma_sound_set_volume(&instance->sound, volume);
    ma_sound_set_pitch(&instance->sound, pitch);
    ma_sound_start(&instance->sound);
    instance->active = true;

    return instance;
}

void DUAL_SoundInstance_Play(DUAL_AudioManager* audio, DUAL_SoundInstance* instance) {
    (void)audio;
    if (!instance || !instance->active) return;
    ma_sound_seek_to_pcm_frame(&instance->sound, 0);
    ma_sound_start(&instance->sound);
}

void DUAL_SoundInstance_Stop(DUAL_AudioManager* audio, DUAL_SoundInstance* instance) {
    (void)audio;
    if (!instance || !instance->active) return;
    ma_sound_stop(&instance->sound);
}

bool DUAL_SoundInstance_IsPlaying(const DUAL_AudioManager* audio, const DUAL_SoundInstance* instance) {
    (void)audio;
    if (!instance || !instance->active) return false;
    return (bool)ma_sound_is_playing(&instance->sound);
}

/* ============================================================================
 * Musique (streaming depuis fichier)
 * ========================================================================== */

DUAL_Result DUAL_Music_OpenFromFile(DUAL_AudioManager* audio, DUAL_ResourceManager* resources,
                                     const char* chemin_fichier, DUAL_Music** out_music) {
    if (!audio || !chemin_fichier || !out_music) return -1;

    DUAL_Music* music = (DUAL_Music*)calloc(1, sizeof(DUAL_Music));
    if (!music) return -1;

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
    (void)audio;
    if (!music) return;
    ma_sound_set_looping(&music->sound, boucle);
    ma_sound_start(&music->sound);
}

void DUAL_Music_Pause(DUAL_AudioManager* audio, DUAL_Music* music) {
    (void)audio;
    if (!music) return;
    ma_sound_stop(&music->sound);
}

void DUAL_Music_Stop(DUAL_AudioManager* audio, DUAL_Music* music) {
    (void)audio;
    if (!music) return;
    ma_sound_stop(&music->sound);
    ma_sound_seek_to_pcm_frame(&music->sound, 0);
}

void DUAL_Music_SetVolume(DUAL_AudioManager* audio, DUAL_Music* music, float volume) {
    (void)audio;
    if (!music) return;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    ma_sound_set_volume(&music->sound, volume);
}

bool DUAL_Music_IsPlaying(const DUAL_AudioManager* audio, const DUAL_Music* music) {
    (void)audio;
    if (!music) return false;
    return (bool)ma_sound_is_playing(&music->sound);
}