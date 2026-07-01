#include "dual_core.h"
#include "dual_input.h"
#include <stdio.h>

int main() {
    DUAL_AppConfig config = {
        .titre_fenetre = "Console Dual - Mode Gameplay & Tactile",
        .largeur_ecran = 800,
        .hauteur_ecran = 480,
        .plein_ecran = false,
        .vsync_actif = true
    };

    DUAL_App* app = NULL;
    DUAL_InputManager* input = NULL;

    if (DUAL_Init(&config, &app) == DUAL_OK) {
        /* Liaison de notre gestionnaire d'inputs */
        DUAL_InputManager_Create(app, &input);

        while (DUAL_ShouldRun(app)) {
            DUAL_BeginFrame(app);

            /* ⚠️ TRÈS IMPORTANT : Mettre à jour les inputs au début de la frame */
            DUAL_InputManager_Update(input);

            // 1. Test Bouton Instantané (Appui unique)
            if (DUAL_IsButtonPressed(input, DUAL_BUTTON_A)) {
                DUAL_Log(DUAL_LOG_INFO, "Bouton A pressé ! Lancement d'une action.");
            }

            // 2. Test Bouton Continu (Maintenu)
            if (DUAL_IsButtonDown(input, DUAL_BUTTON_START)) {
                DUAL_Log(DUAL_LOG_WARNING, "Bouton START maintenu. Demande de fermeture...");
                DUAL_RequestExit(app);
            }

            // 3. Test du Stick Analogique (Touches ZQSD)
            DUAL_Vec2 stick = DUAL_GetJoystickAxis(input, DUAL_JOYSTICK_PRINCIPAL);
            if (stick.x != 0.0f || stick.y != 0.0f) {
                printf("[STICK] Position : X: %.2f | Y: %.2f\n", stick.x, stick.y);
            }

            // 4. Test de l'Écran Tactile (Clic souris sur l'écran inférieur uniquement)
            if (DUAL_IsTouching(input)) {
                DUAL_TouchState tactile = DUAL_GetTouchState(input);
                printf("[TACTILE] Phase: %d | Coordonnées Écran Bas locales -> X: %.1f, Y: %.1f\n",
                       tactile.phase, tactile.position.x, tactile.position.y);
            }

            DUAL_EndFrame(app);
        }

        /* Nettoyage obligatoire */
        DUAL_InputManager_Destroy(input);
    }

    DUAL_Shutdown(app);
    return 0;
}