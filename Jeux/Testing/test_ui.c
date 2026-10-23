//
// Created by killian on 10/12/26.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <dlfcn.h>

#include "../../SDK/include/DUAL_Audio/dual_audio.h"
#include "../../SDK/include/DUAL_Core/dual_core.h"
#include "../../SDK/include/DUAL_FS/dual_fs.h"
#include "../../SDK/include/DUAL_Math/dual_math.h"
#include "../../SDK/include/DUAL_Graphics/dual_graphics_2d.h"
#include "../../SDK/include/DUAL_Graphics/dual_graphics_3d.h"
#include "../../SDK/include/DUAL_Resources/dual_resources.h"
#include "game_api.h"
#include "../../SDK/include/DUAL_Input/dual_input.h"
#include <unistd.h>

#include <stdio.h>
#include <string.h>

#include "dual_utils.h"


// TODO
// Ici on recupere toutes les input et donc nous empechons de recuperer les inputs
// de notre inputManager. Il faudrait donc inclure cette fonctionnalite dans notre input manager


int main() {
    DUAL_App* app = NULL;
    DUAL_AppConfig config = {
        .titre_fenetre = "Test libdual - Double Ecran",
        .largeur_ecran = 400 * 2,
        .hauteur_ecran = 240 * 2,
        .plein_ecran = true,
        .vsync_actif = true,
        .fps_cible = 60
    };

    if (DUAL_Init(&config, &app) != DUAL_OK) {
        return -1;
    }

    DUAL_Renderer2D* renderer2d = NULL;
    DUAL_Renderer2D_Create(app, &renderer2d);

    DUAL_ResourceManager* resourceManager = NULL;
    DUAL_ResourceManager_Create(app, &resourceManager);

    DUAL_Font* font = NULL;
    DUAL_Result result;
    result = DUAL_Font_LoadFromFile(resourceManager, "/home/killian/Documents/Assets/Fonts/test.ttf", 50, &font);
    DEBUG_DUAL_RESULT(result);

    DUAL_SetScreenClearColor(app, DUAL_SCREEN_BOTTOM, DUAL_COLOR_BLACK);
    DUAL_SetScreenClearColor(app, DUAL_SCREEN_TOP, DUAL_COLOR_BLACK);

    DUAL_InputManager* inputManager = NULL;
    DUAL_InputManager_Create(app, &inputManager);

    // Boucle principale
    while (DUAL_ShouldRun(app)) {
        DUAL_InputManager_Update(inputManager);

        DUAL_BeginFrame(app);
        DUAL_Renderer2D_Begin(renderer2d);

        if (DUAL_IsButtonPressed(inputManager, DUAL_BUTTON_UP)) {
            set_buffer_data_active(inputManager, !is_buffer_active(inputManager));
        }
        if (is_buffer_active(inputManager)) {
            DUAL_DrawText(renderer2d, font, get_buffer_data(inputManager), (DUAL_Vec2){100.0, 100.0}, DUAL_COLOR_WHITE);
        }

        DUAL_Renderer2D_End(renderer2d);

        DUAL_EndFrame(app);
    }

    return 0;
}