#define MINIAUDIO_IMPLEMENTATION
#define MA_ENABLE_OPUS
#include "../../include/DUAL_Audio/dual_audio.h"
#include "../../include/DUAL_Resources/dual_resources.h"

#include <stdlib.h>
#include <string.h>


/* ============================================================================
 * Utilitaires internes
 * ========================================================================== */

static ma_sound_group* DUAL_Internal_GetGroupForChannel(DUAL_AudioManager* audio, DUAL_AudioChannel channel) {
    switch (channel) {
        case DUAL_CHANNEL_SFX:   return &audio->sfxGroup;
        case DUAL_CHANNEL_MUSIC: return &audio->musicGroup;
        case DUAL_CHANNEL_VOICE: return &audio->voiceGroup;
        case DUAL_CHANNEL_MASTER:
        default:
            return NULL;
    }
}

static void DUAL_Internal_ReleaseInstanceSlot(DUAL_SoundInstance* slot) {
    if (!slot) return;
    if (slot->initialized) {
        ma_sound_uninit(&slot->sound);
        ma_audio_buffer_ref_uninit(&slot->bufferRef);
        slot->initialized = false;
    }
    slot->inUse = false;
    slot->source = NULL;
}

static void DUAL_Internal_Sound_DestroyCallback(DUAL_ResourceManager* resources, void* ptr) {
    DUAL_Sound_Destroy((DUAL_Sound*)ptr);
}

static void DUAL_Internal_Music_DestroyCallback(DUAL_ResourceManager* resources, void* ptr) {
    DUAL_Music_Close(resources, (DUAL_Music*)ptr);
}

/* ============================================================================
 * Cycle de vie du gestionnaire audio
 * ========================================================================== */

DUAL_Result DUAL_AudioManager_Create(DUAL_App* app, DUAL_AudioManager** out_audio) {
    if (!out_audio) return DUAL_ERROR_INVALID_ARG;
    *out_audio = NULL;

    DUAL_AudioManager* audio = (DUAL_AudioManager*)calloc(1, sizeof(DUAL_AudioManager));
    if (!audio) return DUAL_ERROR_OUT_OF_MEMORY;

    audio->app = app;

    ma_engine_config engineConfig = ma_engine_config_init();
    ma_result maResult = ma_engine_init(&engineConfig, &audio->engine);
    if (maResult != MA_SUCCESS) {
        DUAL_Log(DUAL_LOG_ERROR, "DUAL_AudioManager_Create: ma_engine_init a echoue (code %d)", (int)maResult);
        free(audio);
        return DUAL_ERROR_INIT_FAILED;
    }
    audio->engineInitialized = true;

    ma_sound_group_config sfxGroupConfig = ma_sound_group_config_init();
    maResult = ma_sound_group_init_ex(&audio->engine, &sfxGroupConfig, &audio->sfxGroup);
    if (maResult != MA_SUCCESS) {
        DUAL_Log(DUAL_LOG_ERROR, "DUAL_AudioManager_Create: creation du groupe SFX echouee (code %d)", (int)maResult);
        DUAL_AudioManager_Destroy(audio);
        return DUAL_ERROR_INIT_FAILED;
    }
    audio->sfxGroupInitialized = true;

    ma_sound_group_config musicGroupConfig = ma_sound_group_config_init();
    maResult = ma_sound_group_init_ex(&audio->engine, &musicGroupConfig, &audio->musicGroup);
    if (maResult != MA_SUCCESS) {
        DUAL_Log(DUAL_LOG_ERROR, "DUAL_AudioManager_Create: creation du groupe MUSIC echouee (code %d)", (int)maResult);
        DUAL_AudioManager_Destroy(audio);
        return DUAL_ERROR_INIT_FAILED;
    }
    audio->musicGroupInitialized = true;

    ma_sound_group_config voiceGroupConfig = ma_sound_group_config_init();
    maResult = ma_sound_group_init_ex(&audio->engine, &voiceGroupConfig, &audio->voiceGroup);
    if (maResult != MA_SUCCESS) {
        DUAL_Log(DUAL_LOG_ERROR, "DUAL_AudioManager_Create: creation du groupe VOICE echouee (code %d)", (int)maResult);
        DUAL_AudioManager_Destroy(audio);
        return DUAL_ERROR_INIT_FAILED;
    }
    audio->voiceGroupInitialized = true;

    *out_audio = audio;
    DUAL_Log(DUAL_LOG_INFO, "DUAL_AudioManager_Create: moteur audio initialise (pool de %d instances)", DUAL_MAX_SOUND_INSTANCES);
    return DUAL_OK;
}

void DUAL_AudioManager_Destroy(DUAL_AudioManager* audio) {
    if (!audio) return;

    for (int i = 0; i < DUAL_MAX_SOUND_INSTANCES; ++i) {
        DUAL_Internal_ReleaseInstanceSlot(&audio->instancePool[i]);
    }

    if (audio->sfxGroupInitialized) {
        ma_sound_group_uninit(&audio->sfxGroup);
        audio->sfxGroupInitialized = false;
    }
    if (audio->musicGroupInitialized) {
        ma_sound_group_uninit(&audio->musicGroup);
        audio->musicGroupInitialized = false;
    }
    if (audio->voiceGroupInitialized) {
        ma_sound_group_uninit(&audio->voiceGroup);
        audio->voiceGroupInitialized = false;
    }

    if (audio->engineInitialized) {
        ma_engine_uninit(&audio->engine);
        audio->engineInitialized = false;
    }

    free(audio);
}

void DUAL_AudioManager_Update(DUAL_AudioManager* audio) {
    if (!audio) return;

    for (int i = 0; i < DUAL_MAX_SOUND_INSTANCES; ++i) {
        DUAL_SoundInstance* slot = &audio->instancePool[i];
        if (slot->inUse && slot->initialized) {
            if (ma_sound_at_end(&slot->sound)) {
                DUAL_Internal_ReleaseInstanceSlot(slot);
            }
        }
    }
}

void DUAL_AudioManager_SetChannelVolume(DUAL_AudioManager* audio, DUAL_AudioChannel channel, float volume) {
    if (!audio) return;

    if (channel == DUAL_CHANNEL_MASTER) {
        ma_engine_set_volume(&audio->engine, volume);
        return;
    }

    ma_sound_group* group = DUAL_Internal_GetGroupForChannel(audio, channel);
    if (group) {
        ma_sound_group_set_volume(group, volume);
    }
}

/* ============================================================================
 * Effets sonores (SFX chargé en RAM)
 * ========================================================================== */

DUAL_Result DUAL_Sound_LoadFromFile(DUAL_AudioManager* audio, DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Sound** out_sound) {
    if (!out_sound) return DUAL_ERROR_INVALID_ARG;
    *out_sound = NULL;

    if (!audio || !resources || !chemin_fichier) {
        return DUAL_ERROR_INVALID_ARG;
    }

    ma_decoder_config decoderConfig = ma_decoder_config_init_default();
    decoderConfig.format     = ma_format_f32;
    decoderConfig.channels   = ma_engine_get_channels(&audio->engine);
    decoderConfig.sampleRate = ma_engine_get_sample_rate(&audio->engine);

    ma_decoder decoder;
    ma_result maResult = ma_decoder_init_file(chemin_fichier, &decoderConfig, &decoder);
    if (maResult != MA_SUCCESS) {
        DUAL_Log(DUAL_LOG_ERROR, "DUAL_Sound_LoadFromFile: impossible d'ouvrir '%s' (code %d)", chemin_fichier, (int)maResult);
        return DUAL_ERROR_INVALID_ARG;
    }

    ma_uint64 frameCount = 0;
    maResult = ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    if (maResult != MA_SUCCESS || frameCount == 0) {
        frameCount = 0;
    }

    ma_uint32 channels   = decoderConfig.channels;
    ma_uint32 sampleRate = decoderConfig.sampleRate;
    size_t bytesPerFrame = (size_t)channels * sizeof(float);

    void* pData = NULL;
    ma_uint64 framesDecoded = 0;

    if (frameCount > 0) {
        size_t dataSize = (size_t)frameCount * bytesPerFrame;
        pData = malloc(dataSize);
        if (!pData) {
            ma_decoder_uninit(&decoder);
            return DUAL_ERROR_OUT_OF_MEMORY;
        }
        maResult = ma_decoder_read_pcm_frames(&decoder, pData, frameCount, &framesDecoded);
        if (maResult != MA_SUCCESS && maResult != MA_AT_END) {
            DUAL_Log(DUAL_LOG_ERROR, "DUAL_Sound_LoadFromFile: echec de decodage de '%s' (code %d)", chemin_fichier, (int)maResult);
            free(pData);
            ma_decoder_uninit(&decoder);
            return DUAL_ERROR_INVALID_ARG;
        }
    } else {
        const ma_uint64 CHUNK_FRAMES = 4096;
        size_t capacityFrames = CHUNK_FRAMES;
        pData = malloc((size_t)capacityFrames * bytesPerFrame);
        if (!pData) {
            ma_decoder_uninit(&decoder);
            return DUAL_ERROR_OUT_OF_MEMORY;
        }

        for (;;) {
            if ((ma_uint64)framesDecoded + CHUNK_FRAMES > capacityFrames) {
                size_t newCapacityFrames = capacityFrames * 2;
                void* pNewData = realloc(pData, (size_t)newCapacityFrames * bytesPerFrame);
                if (!pNewData) {
                    free(pData);
                    ma_decoder_uninit(&decoder);
                    return DUAL_ERROR_OUT_OF_MEMORY;
                }
                pData = pNewData;
                capacityFrames = newCapacityFrames;
            }

            ma_uint64 framesReadThisChunk = 0;
            void* pWriteCursor = (uint8_t*)pData + (size_t)framesDecoded * bytesPerFrame;
            ma_result chunkResult = ma_decoder_read_pcm_frames(&decoder, pWriteCursor, CHUNK_FRAMES, &framesReadThisChunk);
            framesDecoded += framesReadThisChunk;

            if (chunkResult != MA_SUCCESS || framesReadThisChunk < CHUNK_FRAMES) {
                break;
            }
        }
        frameCount = framesDecoded;
    }

    ma_decoder_uninit(&decoder);

    if (framesDecoded == 0) {
        DUAL_Log(DUAL_LOG_ERROR, "DUAL_Sound_LoadFromFile: '%s' n'a produit aucune donnee audio", chemin_fichier);
        free(pData);
        return DUAL_ERROR_INVALID_ARG;
    }

    size_t finalDataSize = (size_t)framesDecoded * bytesPerFrame;


    DUAL_Sound* sound = (DUAL_Sound*)calloc(1, sizeof(DUAL_Sound));


    ma_audio_buffer_config bufferConfig = ma_audio_buffer_config_init(
        ma_format_f32,
        channels,
        framesDecoded,
        pData,
        NULL
    );
    bufferConfig.sampleRate = sampleRate;

    maResult = ma_audio_buffer_init(&bufferConfig, &sound->buffer);
    if (maResult != MA_SUCCESS) {
        DUAL_Log(DUAL_LOG_ERROR, "DUAL_Sound_LoadFromFile: ma_audio_buffer_init a echoue (code %d)", (int)maResult);
        free(pData);
        free(sound);
        return DUAL_ERROR_INIT_FAILED;
    }

    sound->estVoix = (strstr(chemin_fichier, "/voice/") != NULL) || (strstr(chemin_fichier, "\\voice\\") != NULL);

    sound->pData = pData;
    sound->dataSize = finalDataSize;
    sound->channels = channels;
    sound->sampleRate = sampleRate;
    sound->frameCount = framesDecoded;
    sound->owningAudio = audio;

    *out_sound = sound;

    strcpy((*out_sound)->sound_path, chemin_fichier);
    DUAL_GetFilenameWithoutExtension(chemin_fichier, (*out_sound)->sound_name, 32);
    DUAL_ResourceManager_AddResource(resources, *out_sound, DUAL_RESOURCE_SOUND);

    return DUAL_OK;
}

void DUAL_Sound_Destroy(DUAL_Sound* sound) {
    if (!sound) return;

    if (sound->owningAudio) {
        DUAL_AudioManager* audio = sound->owningAudio;
        for (int i = 0; i < DUAL_MAX_SOUND_INSTANCES; ++i) {
            DUAL_SoundInstance* slot = &audio->instancePool[i];
            if (slot->inUse && slot->initialized && slot->source == sound) {
                DUAL_Internal_ReleaseInstanceSlot(slot);
            }
        }
    }

    ma_audio_buffer_uninit(&sound->buffer);

    if (sound->pData) {
        free(sound->pData);
        sound->pData = NULL;
    }

    free(sound);
}

DUAL_Result DUAL_Sound_Play(DUAL_AudioManager* audio, const DUAL_Sound* sound, float volume, float pitch) {
    if (!audio || !sound) return DUAL_ERROR_INVALID_ARG;


    DUAL_SoundInstance* freeSlot = NULL;
    for (int i = 0; i < DUAL_MAX_SOUND_INSTANCES; ++i) {
        DUAL_SoundInstance* slot = &audio->instancePool[i];

        if (slot->inUse && slot->initialized && ma_sound_at_end(&slot->sound)) {
            DUAL_Internal_ReleaseInstanceSlot(slot);
        }

        if (!freeSlot && !slot->inUse) {
            freeSlot = slot;
        }
    }

    if (!freeSlot) {
        DUAL_Log(DUAL_LOG_WARNING, "DUAL_Sound_Play: pool d'instances sature (%d/%d en cours d'utilisation)", DUAL_MAX_SOUND_INSTANCES, DUAL_MAX_SOUND_INSTANCES);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }

    if (freeSlot->initialized) {
        ma_sound_uninit(&freeSlot->sound);
        ma_audio_buffer_ref_uninit(&freeSlot->bufferRef);
        freeSlot->initialized = false;
    }

    ma_sound_group* targetGroup = sound->estVoix ? &audio->voiceGroup : &audio->sfxGroup;

    ma_result maResult = ma_audio_buffer_ref_init(
        ma_format_f32,
        sound->channels,
        sound->pData,
        sound->frameCount,
        &freeSlot->bufferRef
    );
    if (maResult != MA_SUCCESS) {
        DUAL_Log(DUAL_LOG_ERROR, "DUAL_Sound_Play: ma_audio_buffer_ref_init a echoue (code %d)", (int)maResult);
        return DUAL_ERROR_INIT_FAILED;
    }

    ma_sound_config soundConfig = ma_sound_config_init();
    soundConfig.pDataSource = (ma_data_source*)&freeSlot->bufferRef;
    soundConfig.pInitialAttachment = targetGroup;
    soundConfig.flags = 0;
    soundConfig.volumeSmoothTimeInPCMFrames = 0;

    maResult = ma_sound_init_ex(&audio->engine, &soundConfig, &freeSlot->sound);
    if (maResult != MA_SUCCESS) {
        DUAL_Log(DUAL_LOG_ERROR, "DUAL_Sound_Play: ma_sound_init_ex a echoue (code %d)", (int)maResult);
        ma_audio_buffer_ref_uninit(&freeSlot->bufferRef);
        return DUAL_ERROR_INIT_FAILED;
    }

    ma_sound_seek_to_pcm_frame(&freeSlot->sound, 0);

    ma_sound_set_volume(&freeSlot->sound, volume);
    ma_sound_set_pitch(&freeSlot->sound, pitch);
    ma_sound_set_looping(&freeSlot->sound, MA_FALSE);

    maResult = ma_sound_start(&freeSlot->sound);
    if (maResult != MA_SUCCESS) {
        DUAL_Log(DUAL_LOG_ERROR, "DUAL_Sound_Play: ma_sound_start a echoue (code %d)", (int)maResult);
        ma_sound_uninit(&freeSlot->sound);
        ma_audio_buffer_ref_uninit(&freeSlot->bufferRef);
        return DUAL_ERROR_UNKNOWN;
    }

    freeSlot->initialized = true;
    freeSlot->inUse = true;
    freeSlot->source = sound;

    return DUAL_OK;
}

/* ============================================================================
 * Musique (streaming)
 * ========================================================================== */

DUAL_Result DUAL_Music_OpenFromFile(DUAL_AudioManager* audio, DUAL_ResourceManager* resources, const char* chemin_fichier, DUAL_Music** out_music) {
    if (!out_music) return DUAL_ERROR_INVALID_ARG;
    *out_music = NULL;

    if (!audio || !resources || !chemin_fichier) {
        return DUAL_ERROR_INVALID_ARG;
    }

    DUAL_Music* music = (DUAL_Music*)calloc(1, sizeof(DUAL_Music));
    if (!music) return DUAL_ERROR_OUT_OF_MEMORY;

    ma_uint32 flags = MA_SOUND_FLAG_STREAM;
    ma_result maResult = ma_sound_init_from_file(&audio->engine, chemin_fichier, flags, &audio->musicGroup, NULL, &music->sound);
    if (maResult != MA_SUCCESS) {
        DUAL_Log(DUAL_LOG_ERROR, "DUAL_Music_OpenFromFile: impossible d'ouvrir '%s' (code %d)", chemin_fichier, (int)maResult);
        free(music);
        return DUAL_ERROR_INVALID_ARG;
    }
    music->initialized = true;
    music->boucle = false;

    *out_music = music;
    return DUAL_OK;
}

void DUAL_Music_Close(DUAL_ResourceManager* resources, DUAL_Music* music) {
    if (!music) return;

    if (music->initialized) {
        ma_sound_uninit(&music->sound);
        music->initialized = false;
    }


    free(music);
}

void DUAL_Music_Play(DUAL_AudioManager* audio, DUAL_Music* music, bool boucle) {
    if (!audio || !music || !music->initialized) return;

    music->boucle = boucle;
    ma_sound_set_looping(&music->sound, boucle ? MA_TRUE : MA_FALSE);
    ma_sound_start(&music->sound);
}

void DUAL_Music_Pause(DUAL_AudioManager* audio, DUAL_Music* music) {
    (void)audio;
    if (!music || !music->initialized) return;
    ma_sound_stop(&music->sound);
}

void DUAL_Music_Stop(DUAL_AudioManager* audio, DUAL_Music* music) {
    (void)audio;
    if (!music || !music->initialized) return;
    ma_sound_stop(&music->sound);
    ma_sound_seek_to_pcm_frame(&music->sound, 0);
}

void DUAL_Music_SetVolume(DUAL_AudioManager* audio, DUAL_Music* music, float volume) {
    (void)audio;
    if (!music || !music->initialized) return;
    ma_sound_set_volume(&music->sound, volume);
}

bool DUAL_Music_IsPlaying(const DUAL_AudioManager* audio, const DUAL_Music* music) {
    (void)audio;
    if (!music || !music->initialized) return false;
    return ma_sound_is_playing(&music->sound) == MA_TRUE;
}
