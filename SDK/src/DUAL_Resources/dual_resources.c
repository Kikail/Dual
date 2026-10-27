#include "../../include/DUAL_Resources/dual_resources.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "DUAL_Audio/dual_audio.h"
#include "DUAL_Graphics/dual_graphics_2d.h"
#include "DUAL_Graphics/dual_graphics_3d.h"



DUAL_Result DUAL_ResourceManager_InitHashTables(DUAL_ResourceManager* resource_manager) {
    resource_manager->texture_collection = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)DUAL_Texture_Destroy);
    resource_manager->model_collection = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)DUAL_Model_Destroy);
    resource_manager->animated_models_collection = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)DUAL_AnimatedModel_Destroy);
    resource_manager->sound_collection = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)DUAL_Sound_Destroy);
    resource_manager->font_collection = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)DUAL_Font_Destroy);
}

void DUAL_ResourceManager_AddTexture(DUAL_ResourceManager* manager, DUAL_Texture* resource_ptr) {
    gboolean existe = g_hash_table_contains(manager->texture_collection, resource_ptr->texture_name);
    if (existe) {
        DUAL_Log(DUAL_LOG_WARNING, "%s est deja charge", resource_ptr->texture_name);
        return;
    }
    g_hash_table_insert(manager->texture_collection, strdup(resource_ptr->texture_name), resource_ptr);
}
void DUAL_ResourceManager_AddModel(DUAL_ResourceManager* manager, DUAL_Model* resource_ptr) {
    gboolean existe = g_hash_table_contains(manager->model_collection, resource_ptr->model_name);
    if (existe) {
        DUAL_Log(DUAL_LOG_WARNING, "%s est deja charge", resource_ptr->model_name);
        return;
    }
    g_hash_table_insert(manager->model_collection, strdup(resource_ptr->model_name), resource_ptr);
}
void DUAL_ResourceManager_AddAnimatedModel(DUAL_ResourceManager* manager, DUAL_Animated_Model* resource_ptr) {
    gboolean existe = g_hash_table_contains(manager->animated_models_collection, resource_ptr->model_name);
    if (existe) {
        DUAL_Log(DUAL_LOG_WARNING, "%s est deja charge", resource_ptr->model_name);
        return;
    }
    g_hash_table_insert(manager->animated_models_collection, strdup(resource_ptr->model_name), resource_ptr);
}
void DUAL_ResourceManager_AddSound(DUAL_ResourceManager* manager, DUAL_Sound* resource_ptr) {
    gboolean existe = g_hash_table_contains(manager->sound_collection, resource_ptr->sound_name);
    if (existe) {
        DUAL_Log(DUAL_LOG_WARNING, "%s est deja charge", resource_ptr->sound_name);
        return;
    }
    g_hash_table_insert(manager->sound_collection, strdup(resource_ptr->sound_name), resource_ptr);
}
void DUAL_ResourceManager_AddFont(DUAL_ResourceManager* manager, DUAL_Font* resource_ptr) {
    gboolean existe = g_hash_table_contains(manager->font_collection, resource_ptr->font_name);
    if (existe) {
        DUAL_Log(DUAL_LOG_WARNING, "%s est deja charge", resource_ptr->font_name);
        return;
    }
    g_hash_table_insert(manager->font_collection, strdup(resource_ptr->font_name), resource_ptr);
}

void DUAL_ResourceManager_AddResource(DUAL_ResourceManager* manager, void* resource_ptr, DUAL_ResourceCategory resource_type) {
    switch (resource_type) {
        case DUAL_RESOURCE_TEXTURE:
            DUAL_ResourceManager_AddTexture(manager, resource_ptr);
            break;
        case DUAL_RESOURCE_MODEL_3D:
            DUAL_ResourceManager_AddModel(manager, resource_ptr);
            break;
        case DUAL_RESOURCE_ANIMATED_MODEL:
            DUAL_ResourceManager_AddAnimatedModel(manager, resource_ptr);
            break;
        case DUAL_RESOURCE_SOUND:
            DUAL_ResourceManager_AddSound(manager, resource_ptr);
            break;
        case DUAL_RESOURCE_FONT:
            DUAL_ResourceManager_AddFont(manager, resource_ptr);
            break;
    }
}

void* DUAL_ResourceManager_GetTexture(DUAL_ResourceManager* resource_manager, char* resource_name) {
    return g_hash_table_lookup(resource_manager->texture_collection, resource_name);
}
void* DUAL_ResourceManager_GetModel(DUAL_ResourceManager* resource_manager, char* resource_name) {
    return g_hash_table_lookup(resource_manager->model_collection, resource_name);
}
void* DUAL_ResourceManager_GetAnimatedModel(DUAL_ResourceManager* resource_manager, char* resource_name) {
    return g_hash_table_lookup(resource_manager->animated_models_collection, resource_name);
}
void* DUAL_ResourceManager_GetFont(DUAL_ResourceManager* resource_manager, char* resource_name) {
    return g_hash_table_lookup(resource_manager->font_collection, resource_name);
}
void* DUAL_ResourceManager_GetSound(DUAL_ResourceManager* resource_manager, char* resource_name) {
    return g_hash_table_lookup(resource_manager->sound_collection, resource_name);
}

void* DUAL_ResourceManager_GetResource(DUAL_ResourceManager* resource_manager, char* resource_name, DUAL_ResourceCategory resource_type) {
    switch (resource_type) {
        case DUAL_RESOURCE_TEXTURE:
            return DUAL_ResourceManager_GetTexture(resource_manager, resource_name);
            break;
        case DUAL_RESOURCE_MODEL_3D:
            return DUAL_ResourceManager_GetModel(resource_manager, resource_name);
            break;
        case DUAL_RESOURCE_ANIMATED_MODEL:
            return DUAL_ResourceManager_GetAnimatedModel(resource_manager, resource_name);
            break;
        case DUAL_RESOURCE_SOUND:
            return DUAL_ResourceManager_GetSound(resource_manager, resource_name);
            break;
        case DUAL_RESOURCE_FONT:
            return DUAL_ResourceManager_GetFont(resource_manager, resource_name);
            break;
    }
}

void DUAL_GetFilenameWithoutExtension(const char* filepath, char* output_buffer, size_t buffer_size) {
    if (!filepath || !output_buffer) return;
    const char* filename = strrchr(filepath, '/');
    if (filename == NULL) {
        filename = filepath;
    } else {
        filename++;
    }
    strncpy(output_buffer, filename, buffer_size);
    output_buffer[buffer_size - 1] = '\0';
    char* dot = strrchr(output_buffer, '.');
    if (dot != NULL) {
        *dot = '\0';
    }
}

static char* DuplicateString(const char* src) {
    if (!src) return NULL;
    size_t len = strlen(src) + 1;
    char* dest = (char*)malloc(len);
    if (dest) memcpy(dest, src, len);
    return dest;
}

DUAL_Result DUAL_ResourceManager_Create(DUAL_App* app, DUAL_ResourceManager** out_manager) {
    if (!out_manager) return DUAL_ERROR_INVALID_ARG;

    DUAL_ResourceManager* manager = (DUAL_ResourceManager*)malloc(sizeof(DUAL_ResourceManager));
    if (!manager) return DUAL_ERROR_OUT_OF_MEMORY;

    manager->app = app;
    manager->vram_utilisee = 0;
    manager->vram_totale = 128 * 1024 * 1024; // Budget VRAM par défaut : 128 Mo
    manager->ram_utilisee = 0;
    manager->ram_totale = 256 * 1024 * 1024;  // Budget RAM par défaut : 256 Mo
    manager->nombre_ressources = 0;

    DUAL_ResourceManager_InitHashTables(manager);

    *out_manager = manager;
    return DUAL_OK;
}

void DUAL_ResourceManager_Destroy(DUAL_ResourceManager* manager) {
    if (!manager) return;

   // A CHANGER AVEC LES HASHMAPS

    free(manager);
}

void DUAL_ResourceManager_PurgeAll(DUAL_ResourceManager* manager) {
    if (!manager) return;

    // A CHANGER AVEC LES HASHMAPS
}


void DUAL_ResourceManager_GetStats(const DUAL_ResourceManager* manager, DUAL_MemoryStats* out_stats) {
    if (!manager || !out_stats) return;
    out_stats->vram_utilisee_octets = manager->vram_utilisee;
    out_stats->vram_totale_octets = manager->vram_totale;
    out_stats->ram_utilisee_octets = manager->ram_utilisee;
    out_stats->ram_totale_octets = manager->ram_totale;
    out_stats->nombre_ressources_actives = manager->nombre_ressources;
}

void DUAL_ResourceManager_SetVRAMBudget(DUAL_ResourceManager* manager, uint64_t limite_octets) {
    if (manager) manager->vram_totale = limite_octets;
}

void DUAL_ResourceManager_SetRAMBudget(DUAL_ResourceManager* manager, uint64_t limite_octets) {
    if (manager) manager->ram_totale = limite_octets;
}

void DUAL_ResourceManager_PurgeCategory(DUAL_ResourceManager* manager, DUAL_ResourceCategory categorie) {
    if (!manager) return;

    // A CHANGER AVEC LES HASHMAPS
}


void DUAL_ResourceManager_Log(DUAL_ResourceManager* manager) {
    if (!manager) return;

    DUAL_Log(DUAL_LOG_DEBUG, "========== DUAL_ResourceManager_Log ==========");
    DUAL_Log(DUAL_LOG_DEBUG, "Resources[%u]", manager->nombre_ressources);
    DUAL_Log(DUAL_LOG_DEBUG, "RAM[%lu/%lu](octets)", manager->ram_utilisee, manager->ram_totale);
    DUAL_Log(DUAL_LOG_DEBUG, "VRAM[%lu/%lu](octets)", manager->vram_utilisee, manager->vram_totale);

}