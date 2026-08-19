//
// Created by killian on 7/9/26.
//
#include <stdio.h>
#include <stdlib.h>

#include "../../SDK/include/DUAL_Core/dual_core.h"
#include "../../SDK/include/DUAL_FS/dual_fs.h"
#include "dual_utils.h"

typedef struct Sauvegarde {
    float health;
    int coins;
    char name[16];
    bool unlocked;
}Sauvegarde;

typedef struct Donnees {
    char texte[128];
}Donnees;

#define DEBUG_SAUVEGARDE(x) DUAL_Log(DUAL_LOG_INFO, "Sauvegarde(%s)[health:%f, coins:%d, name:%s, unlocked:&d]", #x, x.health, x.coins, x.name, x.unlocked)
#define DEBUG_CARTRIDGE_STATUS(x) if(x==DUAL_CARTRIDGE_ABSENTE){\
    DUAL_Log(DUAL_LOG_ERROR, "Cartridge status: ABSENTE");\
}\
else if(x==DUAL_CARTRIDGE_VALIDE){\
    DUAL_Log(DUAL_LOG_INFO, "Cartridge status: VALIDE");\
}\
else{\
    DUAL_Log(DUAL_LOG_ERROR, "Cartridge status: CORROMPUE");\
}\

int main() {
    DUAL_Log(DUAL_LOG_INFO, "Test FILESAVER started !");

    // On initialise l'application
    DUAL_App* app = NULL;
    DUAL_AppConfig config = {.largeur_ecran = 800,.hauteur_ecran = 480,.plein_ecran = false,.fps_cible = 60,.titre_fenetre = "DUAL Core Testing",.vsync_actif = false};
    DUAL_Result result = DUAL_Init(&config, &app);
    DEBUG_DUAL_RESULT(result);

    // On peut changer la couleur de fond
    DUAL_SetScreenClearColor(app, DUAL_SCREEN_BOTTOM, DUAL_COLOR_WHITE);
    DUAL_SetScreenClearColor(app, DUAL_SCREEN_TOP, DUAL_COLOR_BLACK);


    // On change les emplacements de sauvegarde et donnees de jeu
    DUAL_FS_SetCartridgeRoot("/media/killian/692B-8D17/cartridge_data/");
    DUAL_FS_SetSaveDirectory("/media/killian/692B-8D17/saves/");
    DUAL_Log(DUAL_LOG_INFO,"Cartridge root: %s",DUAL_FS_GetCartridgeRoot());
    DUAL_Log(DUAL_LOG_INFO,"Save directory: %s",DUAL_FS_GetSaveDirectory());


    // On creer une fausse sauvegarde pour tester
    Sauvegarde sauvegarde_jeu_creer = {
        .health = 10.5,
        .coins = 50,
        .name = "Zelda",
        .unlocked = false
    };
    DEBUG_SAUVEGARDE(sauvegarde_jeu_creer);

    // On creer une sauvegarde
    DUAL_SaveFile* save = NULL;
    result = DUAL_SaveFile_Open(0,DUAL_SAVE_MODE_ECRITURE, &save);
    DEBUG_DUAL_RESULT(result);
    result = DUAL_SaveFile_Write(save, &sauvegarde_jeu_creer, sizeof(Sauvegarde));
    DEBUG_DUAL_RESULT(result);
    result = DUAL_SaveFile_Close(save);
    DEBUG_DUAL_RESULT(result);

    // On va maintenant lire ces donnees
    result = DUAL_SaveFile_Open(0, DUAL_SAVE_MODE_LECTURE, &save);
    DEBUG_DUAL_RESULT(result);
    uint64_t size;
    result = DUAL_SaveFile_GetSize(save, &size);
    DEBUG_DUAL_RESULT(result);
    DUAL_Log(DUAL_LOG_INFO, "File size: %lu (octets)", size);
    Sauvegarde sauvegarde_jeu_lue;
    result = DUAL_SaveFile_Read(save, &sauvegarde_jeu_lue, sizeof(Sauvegarde), &size);
    DEBUG_DUAL_RESULT(result);
    DEBUG_SAUVEGARDE(sauvegarde_jeu_lue);
    result = DUAL_SaveFile_Close(save);
    DEBUG_DUAL_RESULT(result);

    // On recupere les donnees du slot
    DUAL_SaveSlotInfo slot_info;
    result = DUAL_SaveFile_GetSlotInfo(0, &slot_info);
    DEBUG_DUAL_RESULT(result);


    // On passe a la partie de donnees de cartridge
    DUAL_CartridgeStatus cartridgeStatus = DUAL_Cartridge_GetStatus();
    DEBUG_CARTRIDGE_STATUS(cartridgeStatus)

    DUAL_Cartridge* cartridge;
    result = DUAL_Cartridge_Open(&cartridge);
    DEBUG_DUAL_RESULT(result);

    char* nom_fichier = "donnees.dat";
    result = DUAL_Cartridge_GetFileSize(cartridge, nom_fichier, &size);
    DEBUG_DUAL_RESULT(result);
    DUAL_Log(DUAL_LOG_INFO, "%s size: %lu (octets)", nom_fichier, size);

    Donnees donnees;
    result = DUAL_Cartridge_ReadFile(cartridge, nom_fichier, &donnees, sizeof(Donnees), &size);
    DEBUG_DUAL_RESULT(result);
    DUAL_Log(DUAL_LOG_INFO, "Donnees lues: %s", donnees.texte);
    DUAL_Cartridge_Close(cartridge);

    // On ferme proprement l'application
    DUAL_Shutdown(app);

    return EXIT_SUCCESS;
}
