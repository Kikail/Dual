#include "dual_resources.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Déclaration de la structure opaque du Handle */
struct DUAL_ResourceHandle {
    DUAL_MemoryType type;
    DUAL_ResourceCategory categorie;
    uint64_t taille_octets;
    char* nom_debug;
    void* ptr_ressource;
    void (*callback_destruction)(DUAL_ResourceManager*, void*);
    struct DUAL_ResourceHandle* prev;
    struct DUAL_ResourceHandle* next;
};

/* Déclaration de la structure opaque du Manager */
struct DUAL_ResourceManager {
    DUAL_App* app;
    uint64_t vram_utilisee;
    uint64_t vram_totale;
    uint64_t ram_utilisee;
    uint64_t ram_totale;
    uint32_t nombre_ressources;
    DUAL_ResourceHandle* head;
};

/* Fonction utilitaire pour copier proprement une chaîne de caractères */
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
    manager->head = NULL;

    *out_manager = manager;
    return DUAL_OK;
}

void DUAL_ResourceManager_Destroy(DUAL_ResourceManager* manager) {
    if (!manager) return;

    // Libère toutes les ressources encore suivies automatiquement
    DUAL_ResourceHandle* current = manager->head;
    while (current != NULL) {
        DUAL_ResourceHandle* next = current->next;

        if (current->callback_destruction && current->ptr_ressource) {
            // Le callback appellera en interne DUAL_XXX_Destroy qui déclenchera Untrack
            current->callback_destruction(manager, current->ptr_ressource);
        } else {
            if (current->nom_debug) free(current->nom_debug);
            free(current);
        }
        current = next;
    }

    free(manager);
}

void DUAL_ResourceManager_PurgeAll(DUAL_ResourceManager* manager) {
    if (!manager) return;

    // Libère toutes les ressources encore suivies automatiquement
    DUAL_ResourceHandle* current = manager->head;
    while (current != NULL) {
        DUAL_ResourceHandle* next = current->next;

        if (current->callback_destruction && current->ptr_ressource) {
            // Le callback appellera en interne DUAL_XXX_Destroy qui déclenchera Untrack
            current->callback_destruction(manager, current->ptr_ressource);
        } else {
            if (current->nom_debug) free(current->nom_debug);
            free(current);
        }
        current = next;
    }
}

DUAL_Result DUAL_ResourceManager_Track(DUAL_ResourceManager* manager,
                                        DUAL_MemoryType type,
                                        DUAL_ResourceCategory categorie,
                                        uint64_t taille_octets,
                                        const char* nom_debug,
                                        DUAL_ResourceHandle** out_handle) {
    if (!manager || !out_handle) return DUAL_ERROR_INVALID_ARG;

    // Vérification des budgets mémoire
    if (type == DUAL_MEMORY_VRAM) {
        if (manager->vram_utilisee + taille_octets > manager->vram_totale) {
            return DUAL_ERROR_OUT_OF_MEMORY;
        }
        manager->vram_utilisee += taille_octets;
    } else {
        if (manager->ram_utilisee + taille_octets > manager->ram_totale) {
            return DUAL_ERROR_OUT_OF_MEMORY;
        }
        manager->ram_utilisee += taille_octets;
    }

    DUAL_ResourceHandle* handle = (DUAL_ResourceHandle*)malloc(sizeof(DUAL_ResourceHandle));
    if (!handle) {
        // Rembobinage de l'allocation virtuelle en cas d'échec
        if (type == DUAL_MEMORY_VRAM) manager->vram_utilisee -= taille_octets;
        else manager->ram_utilisee -= taille_octets;
        return DUAL_ERROR_OUT_OF_MEMORY;
    }

    handle->type = type;
    handle->categorie = categorie;
    handle->taille_octets = taille_octets;
    handle->nom_debug = DuplicateString(nom_debug);
    handle->ptr_ressource = NULL;
    handle->callback_destruction = NULL;

    // Ajout en tête de la liste chaînée
    handle->prev = NULL;
    handle->next = manager->head;
    if (manager->head) {
        manager->head->prev = handle;
    }
    manager->head = handle;
    manager->nombre_ressources++;

    *out_handle = handle;
    return DUAL_OK;
}

void DUAL_ResourceManager_Untrack(DUAL_ResourceManager* manager, DUAL_ResourceHandle* handle) {
    if (!manager || !handle) return;

    // Libération des budgets
    if (handle->type == DUAL_MEMORY_VRAM) {
        manager->vram_utilisee = (manager->vram_utilisee > handle->taille_octets) ? 
                                 manager->vram_utilisee - handle->taille_octets : 0;
    } else {
        manager->ram_utilisee = (manager->ram_utilisee > handle->taille_octets) ? 
                                 manager->ram_utilisee - handle->taille_octets : 0;
    }

    // Retrait de la liste chaînée
    if (handle->prev) handle->prev->next = handle->next;
    else manager->head = handle->next;

    if (handle->next) handle->next->prev = handle->prev;

    if (manager->nombre_ressources > 0) manager->nombre_ressources--;

    if (handle->nom_debug) free(handle->nom_debug);
    free(handle);
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

    DUAL_ResourceHandle* current = manager->head;
    while (current != NULL) {
        DUAL_ResourceHandle* next = current->next;
        if (current->categorie == categorie && current->callback_destruction && current->ptr_ressource) {
            current->callback_destruction(manager, current->ptr_ressource);
        }
        current = next;
    }
}

/* Fonction interne (partagée via extern) pour lier l'asset au handle */
void DUAL_Internal_ResourceHandle_SetCallback(DUAL_ResourceHandle* handle, void* ptr_ressource, void (*callback_destruction)(DUAL_ResourceManager*, void*)) {
    if (handle) {
        handle->ptr_ressource = ptr_ressource;
        handle->callback_destruction = callback_destruction;
    }
}

void DUAL_ResourceManager_Log(DUAL_ResourceManager* manager) {
    if (!manager) return;

    DUAL_Log(DUAL_LOG_DEBUG, "========== DUAL_ResourceManager_Log ==========");
    DUAL_Log(DUAL_LOG_DEBUG, "Resources[%u]", manager->nombre_ressources);
    DUAL_Log(DUAL_LOG_DEBUG, "RAM[%lu/%lu](octets)", manager->ram_utilisee, manager->ram_totale);
    DUAL_Log(DUAL_LOG_DEBUG, "VRAM[%lu/%lu](octets)", manager->vram_utilisee, manager->vram_totale);

}