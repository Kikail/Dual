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

// Structure représentant un élément de la file d'attente
typedef struct DUAL_QueuedResource {
    char* path;                         // Chemin vers le fichier
    DUAL_MemoryType type;               // VRAM ou RAM
    DUAL_ResourceCategory categorie;    // Texture, Sound, Model, etc.
    DUAL_LoadCallback load_fn;          // Pointeur vers la fonction de chargement spécifique
    void** out_resource_ptr;            // Pointeur vers où stocker la ressource chargée
    struct DUAL_QueuedResource* next;   // Élément suivant dans la file
} DUAL_QueuedResource;

struct DUAL_ResourceGroup {
    DUAL_QueuedResource* head;  // Début de la queue (premier à charger)
    DUAL_QueuedResource* tail;  // Fin de la queue (dernier ajouté)
    uint32_t total_count;       // Nombre total d'éléments au départ
    uint32_t loaded_count;      // Nombre d'éléments déjà traités
};
DUAL_Result DUAL_ResourceGroup_Create(DUAL_ResourceGroup** out_group) {
    if (!out_group) return DUAL_ERROR_INVALID_ARG;

    DUAL_ResourceGroup* group = (DUAL_ResourceGroup*)malloc(sizeof(DUAL_ResourceGroup));
    if (!group) return DUAL_ERROR_OUT_OF_MEMORY;

    group->head = NULL;
    group->tail = NULL;
    group->total_count = 0;
    group->loaded_count = 0;

    *out_group = group;
    return DUAL_OK;
}
void DUAL_ResourceGroup_Destroy(DUAL_ResourceGroup* group) {
    if (!group) return;

    DUAL_QueuedResource* current = group->head;
    while (current) {
        DUAL_QueuedResource* next = current->next;
        if (current->path) free(current->path);
        free(current);
        current = next;
    }
    free(group);
}
DUAL_Result DUAL_ResourceGroup_Add(DUAL_ResourceGroup* group,
                                   const char* path,
                                   DUAL_MemoryType type,
                                   DUAL_ResourceCategory categorie,
                                   DUAL_LoadCallback load_fn,
                                   void** out_resource_ptr) {
    if (!group || !path || !load_fn) return DUAL_ERROR_INVALID_ARG;

    DUAL_QueuedResource* node = (DUAL_QueuedResource*)malloc(sizeof(DUAL_QueuedResource));
    if (!node) return DUAL_ERROR_OUT_OF_MEMORY;

    strcpy(node->path, path);
    node->type = type;
    node->categorie = categorie;
    node->load_fn = load_fn;
    node->out_resource_ptr = out_resource_ptr;
    node->next = NULL;

    // Ajout en fin de liste (FIFO)
    if (group->tail) {
        group->tail->next = node;
        group->tail = node;
    } else {
        group->head = node;
        group->tail = node;
    }

    group->total_count++;
    return DUAL_OK;
}
/**
 * @brief Traite la prochaine ressource de la file d'attente.
 * @return DUAL_OK si une ressource a été chargée,
 *         DUAL_OK_FINISHED (ou similar) s'il n'y a plus rien à charger.
 */
DUAL_Result DUAL_ResourceGroup_Step(DUAL_ResourceManager* manager, DUAL_ResourceGroup* group) {
    if (!manager || !group) return DUAL_ERROR_INVALID_ARG;
    if (!group->head) return DUAL_OK; // La queue est vide

    // Défilement du premier élément (FIFO)
    DUAL_QueuedResource* node = group->head;
    group->head = node->next;
    if (!group->head) {
        group->tail = NULL;
    }

    // Exécution du chargement
    DUAL_Result result = node->load_fn(manager, node->path, node->out_resource_ptr);

    // Nettoyage du noeud exécuté
    free(node->path);
    free(node);

    group->loaded_count++;
    return result;
}

// Récupérer l'avancement (entre 0.0f et 1.0f) pour afficher la barre de progression
float DUAL_ResourceGroup_GetProgress(const DUAL_ResourceGroup* group) {
    if (!group || group->total_count == 0) return 1.0f;
    return (float)group->loaded_count / (float)group->total_count;
}

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