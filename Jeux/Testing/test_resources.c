//
// Created by killian on 7/9/26.
//
#include <stdio.h>
#include <stdlib.h>

#include "dual_core.h"
#include "dual_resources.h"
#include "dual_utils.h"

typedef struct Stats {
    int Armure;
    int ResistanceMagique;
    int Degat;
    int Lvl;
}Stats;

typedef struct Equipement {
    char nom[32];
    Stats stats;
}Equipment;

typedef struct Personnage {
    char nom[32];
    Equipment equipment[8];
}Personnage;

#define ALLOC(type) (type*)malloc(sizeof(type))

int main() {
    DUAL_Log(DUAL_LOG_INFO, "Test Resources started !");

    // On initialise l'application
    DUAL_App* app = NULL;
    DUAL_AppConfig config = {
        .largeur_ecran = 800,
        .hauteur_ecran = 480,
        .plein_ecran = false,
        .fps_cible = 60,
        .titre_fenetre = "DUAL Core Testing",
        .vsync_actif = false
    };
    DUAL_Result result = DUAL_Init(&config, &app);
    DEBUG_DUAL_RESULT(result);

    // On creer le resource manager
    DUAL_ResourceManager* resourceManager = NULL;
    DUAL_ResourceManager_Create(app, &resourceManager);
    DUAL_ResourceManager_Log(resourceManager);

    // On creer une structure et on regarde ce que ca a changer dans la memoire
    Personnage* personnage = ALLOC(Personnage);
    DUAL_ResourceHandle* handle = NULL;
    DUAL_ResourceManager_Track(resourceManager, DUAL_MEMORY_RAM, DUAL_RESOURCE_STRUCT, sizeof(Personnage), "personnage", &handle);
    DUAL_ResourceManager_Log(resourceManager);

    // On ferme proprement l'application
    DUAL_Shutdown(app);

    return EXIT_SUCCESS;
}
