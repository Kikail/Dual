//
// Created by killian on 7/9/26.
//
#include <stdio.h>
#include <stdlib.h>

#include "dual_audio.h"
#include "dual_core.h"
#include "dual_input.h"
#include "dual_resources.h"
#include "dual_utils.h"

#define SOUND_FX_1 "/home/killian/CLionProjects/Dual/Jeux/resources/sounds/sfx/jingles_NES00.mp3"
#define SOUND_FX_2 "/home/killian/CLionProjects/Dual/Jeux/resources/sounds/sfx/jingles_NES12.mp3"
#define SOUND_FX_3 "/home/killian/CLionProjects/Dual/Jeux/resources/sounds/sfx/jingles_NES13.mp3"
#define SOUND_VOICE_1 "/home/killian/CLionProjects/Dual/Jeux/resources/sounds/voice/game_over.mp3"
#define SOUND_VOICE_2 "/home/killian/CLionProjects/Dual/Jeux/resources/sounds/voice/level_up.mp3"
#define SOUND_VOICE_3 "/home/killian/CLionProjects/Dual/Jeux/resources/sounds/voice/objective_achieved.mp3"
#define SOUND_MUSIC_1 "/home/killian/CLionProjects/Dual/Jeux/resources/sounds/music/Flowing Rocks.mp3"
#define SOUND_MUSIC_2 "/home/killian/CLionProjects/Dual/Jeux/resources/sounds/music/Game Over.mp3"
#define SOUND_MUSIC_3 "/home/killian/CLionProjects/Dual/Jeux/resources/sounds/music/Infinite Descent.mp3"

int main() {
    DUAL_Log(DUAL_LOG_INFO, "Test Audio started !");

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

    // On peut changer la couleur de fond
    DUAL_SetScreenClearColor(app, DUAL_SCREEN_BOTTOM, DUAL_COLOR_WHITE);
    DUAL_SetScreenClearColor(app, DUAL_SCREEN_TOP, DUAL_COLOR_BLACK);

    // Creation d'un resource manager
    DUAL_ResourceManager* resources = NULL;
    result = DUAL_ResourceManager_Create(app, &resources);
    DEBUG_DUAL_RESULT(result);

    // On va charger les sons
    DUAL_AudioManager* audioManager = NULL;
    DUAL_AudioManager_Create(app, &audioManager);

    // Tout les audio charge
    DUAL_Sound* sound_fx[3];
    DUAL_Sound* sound_voice[3];
    DUAL_Music* sound_music[3];
    result = DUAL_Sound_LoadFromFile(audioManager, resources, SOUND_FX_1, &sound_fx[0]);
    DEBUG_DUAL_RESULT(result);
    DUAL_Sound_LoadFromFile(audioManager, resources, SOUND_FX_2, &sound_fx[1]);
    DUAL_Sound_LoadFromFile(audioManager, resources, SOUND_FX_3, &sound_fx[2]);
    result = DUAL_Sound_LoadFromFile(audioManager, resources, SOUND_VOICE_1, &sound_voice[0]);
    DEBUG_DUAL_RESULT(result);
    DUAL_Sound_LoadFromFile(audioManager, resources, SOUND_VOICE_2, &sound_voice[1]);
    DUAL_Sound_LoadFromFile(audioManager, resources, SOUND_VOICE_3, &sound_voice[2]);
    result = DUAL_Music_OpenFromFile(audioManager, resources, SOUND_MUSIC_1, &sound_music[0]);
    DEBUG_DUAL_RESULT(result);
    DUAL_Music_OpenFromFile(audioManager, resources, SOUND_MUSIC_2, &sound_music[1]);
    DUAL_Music_OpenFromFile(audioManager, resources, SOUND_MUSIC_3, &sound_music[2]);

    // On joue les sons
    DUAL_Music_Play(audioManager, sound_music[0], true);

    // On fait un systeme d'input pour tout controller facilement
    DUAL_InputManager* inputManager = NULL;
    result = DUAL_InputManager_Create(app, &inputManager);
    DEBUG_DUAL_RESULT(result);
    uint8_t selection_sfx = 0;
    uint8_t selection_voice = 0;
    uint8_t selection_music = 0;
    float volume = 1.0f;

    // Boucle du jeu principal
    while (DUAL_ShouldRun(app)) {
        DUAL_BeginFrame(app);

        DUAL_InputManager_Update(inputManager);

        // On change les audio et on les joue ici comme on veut
        if (DUAL_IsButtonPressed(inputManager, DUAL_BUTTON_LEFT)) {
            DUAL_Sound_Play(audioManager, sound_voice[selection_voice], volume, 1.0);
            selection_voice += 1;
            if (selection_voice == 3) selection_voice = 0;
        }
        if (DUAL_IsButtonPressed(inputManager, DUAL_BUTTON_RIGHT)) {
            DUAL_Sound_Play(audioManager, sound_fx[selection_sfx], volume, 1.0);
            selection_sfx += 1;
            if (selection_sfx == 3) selection_sfx = 0;
        }
        if (DUAL_IsButtonPressed(inputManager, DUAL_BUTTON_UP)) {
            volume += 0.1f;
            if (volume > 1.0f) volume = 1.0f;
            DUAL_AudioManager_SetChannelVolume(audioManager, DUAL_CHANNEL_MASTER, volume);
        }
        if (DUAL_IsButtonPressed(inputManager, DUAL_BUTTON_DOWN)) {
            volume -= 0.1f;
            if (volume < 0.0f) volume = 0.0f;
            DUAL_AudioManager_SetChannelVolume(audioManager, DUAL_CHANNEL_MASTER, volume);
        }

        DUAL_AudioManager_Update(audioManager);

        DUAL_EndFrame(app);
    }

    // On ferme proprement l'application
    DUAL_Shutdown(app);

    return EXIT_SUCCESS;
}
