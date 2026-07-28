#define MINIAUDIO_IMPLEMENTATION
#define MA_ENABLE_OPUS
#include "miniaudio.h"

#include "dual_audio.h"
#include "dual_resources.h"

#include <stdlib.h>
#include <string.h>

#define DUAL_MAX_SOUND_INSTANCES 256

/* ============================================================================
 * Structures internes
 * ========================================================================== */

/**
 * @brief Une entrée du pool d'instances de lecture "fire-and-forget".
 *
 * `inUse` vaut true dès qu'un slot est attribué à DUAL_Sound_Play, et
 * repasse à false lorsque l'instance est recyclée (soit parce qu'elle est
 * arrivée à la fin de sa lecture, soit parce que le DUAL_Sound source est
 * détruit pendant qu'elle joue encore).
 */
typedef struct DUAL_SoundInstance {
    ma_sound sound;
    ma_audio_buffer_ref bufferRef; /* Curseur de lecture INDEPENDANT sur les données du DUAL_Sound source ; permet à plusieurs instances de jouer le même DUAL_Sound simultanément sans se marcher dessus. */
    bool inUse;
    bool initialized;   /* true si ma_sound_init_* a réussi au moins une fois sur ce slot (uninit nécessaire avant réutilisation) */
    const DUAL_Sound* source; /* Sound joué par ce slot, pour pouvoir le libérer si le Sound source est détruit avant la fin de la lecture */
} DUAL_SoundInstance;

struct DUAL_AudioManager {
    DUAL_App* app;
    ma_engine engine;
    bool engineInitialized;

    ma_sound_group sfxGroup;
    ma_sound_group musicGroup;
    ma_sound_group voiceGroup;
    bool sfxGroupInitialized;
    bool musicGroupInitialized;
    bool voiceGroupInitialized;

    DUAL_SoundInstance instancePool[DUAL_MAX_SOUND_INSTANCES];
};

/**
 * @brief Un effet sonore entièrement décodé en RAM.
 *
 * pData / dataSize décrivent l'allocation brute suivie par le
 * DUAL_ResourceManager (utile pour DUAL_ResourceManager_GetStats et pour
 * libérer l'allocation à la destruction). buffer est un ma_audio_buffer
 * "maître" construit par-dessus ce même bloc mémoire (aucune copie
 * supplémentaire) ; il n'est jamais joué directement (voir le pool
 * d'instances). owningAudio référence le DUAL_AudioManager utilisé pour le
 * chargement, afin que DUAL_Sound_Destroy puisse arrêter en sécurité toute
 * instance du pool encore en train de jouer ce son avant de libérer pData.
 */
struct DUAL_Sound {
    void* pData;
    size_t dataSize;
    ma_audio_buffer buffer;
    ma_uint32 channels;
    ma_uint32 sampleRate;
    ma_uint64 frameCount; /* nombre de frames PCM dans pData, utilise pour initialiser un ma_audio_buffer_ref par instance de lecture */
    bool estVoix; /* true si ce Sound doit etre route vers voiceGroup plutot que sfxGroup lors de la lecture */
    DUAL_AudioManager* owningAudio; /* AudioManager utilise pour le chargement ; permet a DUAL_Sound_Destroy d'arreter en securite toute instance du pool encore en train de jouer ce son, sans changer la signature publique de la fonction. */
    DUAL_ResourceHandle* resourceHandle;
};

/**
 * @brief Une musique streamée depuis le disque.
 */
struct DUAL_Music {
    ma_sound sound;
    bool initialized;
    bool boucle;
    DUAL_ResourceHandle* resourceHandle;
};

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
            return NULL; /* Le canal MASTER est géré via ma_engine directement, pas via un group */
    }
}

/**
 * @brief Libère (uninit) un slot du pool s'il a déjà été initialisé, et
 *        remet son état à "libre".
 */
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

/**
 * @brief Callback interne enregistré comme "callback de destruction" auprès
 *        du DUAL_ResourceManager pour un DUAL_Sound. Permet à
 *        DUAL_ResourceManager_Destroy / PurgeAll / PurgeCategory de détruire
 *        proprement un DUAL_Sound encore tracké sans connaître le type
 *        concret.
 */
static void DUAL_Internal_Sound_DestroyCallback(DUAL_ResourceManager* resources, void* ptr) {
    DUAL_Sound_Destroy(resources, (DUAL_Sound*)ptr);
}

static void DUAL_Internal_Music_DestroyCallback(DUAL_ResourceManager* resources, void* ptr) {
    DUAL_Music_Close(resources, (DUAL_Music*)ptr);
}

/* Déclarée dans dual_resources.c mais non exposée dans dual_resources.h :
 * lie un DUAL_ResourceHandle à son pointeur de ressource concret et à son
 * callback de destruction. On la redéclare ici en extern comme le fait le
 * reste du framework pour les autres modules producteurs de ressources
 * (graphics_2d / graphics_3d). */
extern void DUAL_Internal_ResourceHandle_SetCallback(DUAL_ResourceHandle* handle, void* ptr_ressource, void (*callback_destruction)(DUAL_ResourceManager*, void*));

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

    /* Le pool est déjà à zéro grâce à calloc : inUse = false, initialized = false, source = NULL */

    *out_audio = audio;
    DUAL_Log(DUAL_LOG_INFO, "DUAL_AudioManager_Create: moteur audio initialise (pool de %d instances)", DUAL_MAX_SOUND_INSTANCES);
    return DUAL_OK;
}

void DUAL_AudioManager_Destroy(DUAL_AudioManager* audio) {
    if (!audio) return;

    /* Arrête et libère toutes les instances actives du pool */
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

    /* --- Étape 1 : décodage complet du fichier en un buffer PCM interleaved --- */
    ma_decoder_config decoderConfig = ma_decoder_config_init_default();
    /* format/channels/sampleRate à 0 = "garder le format natif du fichier" ;
       ma_engine attend cependant que la data source expose le meme format
       que le moteur pour eviter un resampling par instance couteux. On
       force donc le format du moteur ici. */
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
        /* Certains flux (notamment MP3 avec en-tetes incomplets) ne peuvent
           pas reporter leur longueur a l'avance : on decode alors de facon
           incrementale par blocs. */
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
        /* Décodage incrémental : on ne connaît pas la longueur à l'avance. */
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
                break; /* MA_AT_END ou fin de flux atteinte */
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

    /* --- Étape 2 : réservation du budget mémoire auprès du ResourceManager --- */
    DUAL_ResourceHandle* handle = NULL;
    DUAL_Result trackResult = DUAL_ResourceManager_Track(resources, DUAL_MEMORY_RAM, DUAL_RESOURCE_SOUND, (uint64_t)finalDataSize, chemin_fichier, &handle);
    if (trackResult != DUAL_OK) {
        free(pData);
        return trackResult;
    }

    /* --- Étape 3 : allocation de la structure DUAL_Sound et construction de l'audio buffer --- */
    DUAL_Sound* sound = (DUAL_Sound*)calloc(1, sizeof(DUAL_Sound));
    if (!sound) {
        free(pData);
        DUAL_ResourceManager_Untrack(resources, handle);
        return DUAL_ERROR_OUT_OF_MEMORY;
    }

    ma_audio_buffer_config bufferConfig = ma_audio_buffer_config_init(
        ma_format_f32,
        channels,
        framesDecoded,
        pData,
        NULL /* pas d'allocateur : le buffer ne possede pas pData, on gere sa liberation nous-memes */
    );
    bufferConfig.sampleRate = sampleRate;

    maResult = ma_audio_buffer_init(&bufferConfig, &sound->buffer);
    if (maResult != MA_SUCCESS) {
        DUAL_Log(DUAL_LOG_ERROR, "DUAL_Sound_LoadFromFile: ma_audio_buffer_init a echoue (code %d)", (int)maResult);
        free(pData);
        free(sound);
        DUAL_ResourceManager_Untrack(resources, handle);
        return DUAL_ERROR_INIT_FAILED;
    }

    sound->estVoix = (strstr(chemin_fichier, "/voice/") != NULL) || (strstr(chemin_fichier, "\\voice\\") != NULL);

    sound->pData = pData;
    sound->dataSize = finalDataSize;
    sound->channels = channels;
    sound->sampleRate = sampleRate;
    sound->frameCount = framesDecoded;
    sound->owningAudio = audio;
    sound->resourceHandle = handle;

    DUAL_Internal_ResourceHandle_SetCallback(handle, sound, DUAL_Internal_Sound_DestroyCallback);

    *out_sound = sound;
    return DUAL_OK;
}

void DUAL_Sound_Destroy(DUAL_ResourceManager* resources, DUAL_Sound* sound) {
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

    if (resources && sound->resourceHandle) {
        DUAL_ResourceManager_Untrack(resources, sound->resourceHandle);
    }

    free(sound);
}

DUAL_Result DUAL_Sound_Play(DUAL_AudioManager* audio, const DUAL_Sound* sound, float volume, float pitch) {
    if (!audio || !sound) return DUAL_ERROR_INVALID_ARG;

    /* Recyclage paresseux : au cas ou DUAL_AudioManager_Update n'a pas
       encore tourne cette frame, on nettoie les instances terminees avant
       de chercher un slot libre, pour maximiser les chances d'en trouver
       un. */
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

    /* Un slot peut avoir ete precedemment initialise pour un AUTRE son :
       ma_sound_init_ex ne peut pas etre appele deux fois sur la meme
       instance sans uninit prealable, et le bufferRef precedent doit lui
       aussi etre libere avant d'en creer un nouveau. */
    if (freeSlot->initialized) {
        ma_sound_uninit(&freeSlot->sound);
        ma_audio_buffer_ref_uninit(&freeSlot->bufferRef);
        freeSlot->initialized = false;
    }

    /* Routage vers le bon groupe de canal selon la nature du son (voir le
       commentaire dans DUAL_Sound_LoadFromFile sur la detection SFX/voix). */
    ma_sound_group* targetGroup = sound->estVoix ? &audio->voiceGroup : &audio->sfxGroup;

    /* IMPORTANT : on ne branche jamais directement sound->buffer (le
       ma_audio_buffer "maitre") sur plusieurs ma_sound simultanes, car il
       n'a qu'un seul curseur de lecture interne. On initialise a la place
       un ma_audio_buffer_ref propre a ce slot, qui reference les memes
       donnees/format sans copie mais avec un curseur independant. */
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

    /* Chaque instance doit repartir du debut de sa propre reference. */
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

    /* Le flux est streame depuis le disque : l'empreinte RAM reelle se
       limite aux buffers internes de decodage de miniaudio, negligeable et
       non mesurable simplement. On tracke neanmoins une entree symbolique
       (taille de la structure de suivi) pour que la musique apparaisse
       dans les statistiques et puisse etre purgee par categorie. */
    DUAL_ResourceHandle* handle = NULL;
    DUAL_Result trackResult = DUAL_ResourceManager_Track(resources, DUAL_MEMORY_RAM, DUAL_RESOURCE_MUSIC, (uint64_t)sizeof(DUAL_Music), chemin_fichier, &handle);
    if (trackResult != DUAL_OK) {
        ma_sound_uninit(&music->sound);
        free(music);
        return trackResult;
    }

    music->resourceHandle = handle;
    DUAL_Internal_ResourceHandle_SetCallback(handle, music, DUAL_Internal_Music_DestroyCallback);

    *out_music = music;
    return DUAL_OK;
}

void DUAL_Music_Close(DUAL_ResourceManager* resources, DUAL_Music* music) {
    if (!music) return;

    if (music->initialized) {
        ma_sound_uninit(&music->sound);
        music->initialized = false;
    }

    if (resources && music->resourceHandle) {
        DUAL_ResourceManager_Untrack(resources, music->resourceHandle);
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
