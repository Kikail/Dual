#include "dual_input.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <math.h>

/* Déclaration externe des fonctions d'accès du module core */
extern void* DUAL_Internal_GetGLFWWindow(const DUAL_App* app);
extern void  DUAL_Internal_GetScreenDimensions(const DUAL_App* app, int32_t* out_w, int32_t* out_h);

/* ============================================================================
 * Définition de la structure opaque du Gestionnaire d'Inputs
 * ========================================================================== */
struct DUAL_InputManager {
    GLFWwindow* window;             /* Handle de la fenêtre GLFW */
    int32_t     largeur_ecran;       /* Dimensions nominales d'un écran */
    int32_t     hauteur_ecran;
    
    /* Tableaux d'états pour les boutons (Frame courante vs Frame précédente) */
    bool current_buttons[DUAL_BUTTON_COUNT];
    bool previous_buttons[DUAL_BUTTON_COUNT];
    
    /* Tableaux de correspondance Clavier PC -> Boutons Console */
    int mapping_clavier[DUAL_BUTTON_COUNT];
    
    /* Données du Stick Analogique */
    DUAL_Vec2 joystick_axes;
    float     joystick_deadzone;
    
    /* Données de l'Écran Tactile */
    DUAL_TouchState touch_current;
    bool            was_touching_last_frame;
};

/* ============================================================================
 * Cycle de vie du gestionnaire
 * ========================================================================== */

DUAL_Result DUAL_InputManager_Create(DUAL_App* app, DUAL_InputManager** out_input) {
    if (!app || !out_input) return DUAL_ERROR_INVALID_ARG;

    DUAL_InputManager* input = (DUAL_InputManager*)malloc(sizeof(struct DUAL_InputManager));
    if (!input) return DUAL_ERROR_OUT_OF_MEMORY;

    /* Récupération des informations de la fenêtre */
    input->window = (GLFWwindow*)DUAL_Internal_GetGLFWWindow(app);
    DUAL_Internal_GetScreenDimensions(app, &input->largeur_ecran, &input->hauteur_ecran);

    /* Initialisation des états par défaut */
    for (int i = 0; i < DUAL_BUTTON_COUNT; i++) {
        input->current_buttons[i] = false;
        input->previous_buttons[i] = false;
    }

    /* MAPPING CLAVIER PC PAR DÉFAUT (Ajustable selon tes préférences)
     *
     * IMPORTANT : le stick analogique (simulé plus bas par ZQSD) réserve les
     * touches Z, Q, S et D. Ces 4 touches ne doivent JAMAIS être également
     * assignées à un bouton ci-dessous : une touche utilisée pour les deux
     * déclencherait le bouton en même temps que le déplacement du stick, ce
     * qui donne l'impression que le bouton reste enfoncé en boucle pendant
     * qu'on se déplace. C'était le cas de X (sur S), Y (sur D) et R (sur Z)
     * dans une version précédente : ils ont été déplacés ci-dessous. */
    input->mapping_clavier[DUAL_BUTTON_A]      = GLFW_KEY_X;           /* X pour valider */
    input->mapping_clavier[DUAL_BUTTON_B]      = GLFW_KEY_C;           /* C pour annuler */
    input->mapping_clavier[DUAL_BUTTON_X]      = GLFW_KEY_V;           /* Anciennement S : entrait en conflit avec le stick */
    input->mapping_clavier[DUAL_BUTTON_Y]      = GLFW_KEY_B;           /* Anciennement D : entrait en conflit avec le stick */
    input->mapping_clavier[DUAL_BUTTON_L]      = GLFW_KEY_A;           /* Gâchettes en haut à gauche/droite du stick */
    input->mapping_clavier[DUAL_BUTTON_R]      = GLFW_KEY_E;           /* Anciennement Z : entrait en conflit avec le stick */
    input->mapping_clavier[DUAL_BUTTON_START]  = GLFW_KEY_ENTER;
    input->mapping_clavier[DUAL_BUTTON_SELECT] = GLFW_KEY_RIGHT_SHIFT;
    input->mapping_clavier[DUAL_BUTTON_UP]     = GLFW_KEY_UP;          /* Croix directionnelle */
    input->mapping_clavier[DUAL_BUTTON_DOWN]   = GLFW_KEY_DOWN;
    input->mapping_clavier[DUAL_BUTTON_LEFT]   = GLFW_KEY_LEFT;
    input->mapping_clavier[DUAL_BUTTON_RIGHT]  = GLFW_KEY_RIGHT;

    input->joystick_axes.x = 0.0f;
    input->joystick_axes.y = 0.0f;
    input->joystick_deadzone = 0.15f; /* 15% de zone morte par défaut */

    input->touch_current.phase = DUAL_TOUCH_NONE;
    input->touch_current.position.x = 0.0f;
    input->touch_current.position.y = 0.0f;
    input->was_touching_last_frame = false;

    *out_input = input;
    return DUAL_OK;
}

void DUAL_InputManager_Destroy(DUAL_InputManager* input) {
    if (input) {
        free(input);
    }
}

void DUAL_InputManager_Update(DUAL_InputManager* input) {
    if (!input || !input->window) return;

    /* 0. RAFRAÎCHISSEMENT DE L'ÉTAT GLFW
     *
     * glfwGetKey()/glfwGetMouseButton() ne renvoient l'état réel du matériel
     * que si la file d'événements GLFW a été traitée récemment. Si l'appli
     * appelle déjà glfwPollEvents() ailleurs (typiquement dans
     * DUAL_BeginFrame()), cet appel est un simple no-op sans effet de bord
     * néfaste. En revanche, si ce n'est pas le cas, son absence est la cause
     * la plus fréquente d'un état de touche qui semble "figé" (donc lu comme
     * enfoncé en continu) : ce module ne doit pas dépendre d'un appel externe
     * pour fonctionner correctement. */
    glfwPollEvents();

    /* 1. MISE À JOUR DES BOUTONS PHYSIQUES */
    for (int i = 0; i < DUAL_BUTTON_COUNT; i++) {
        input->previous_buttons[i] = input->current_buttons[i];
        int code_touche = input->mapping_clavier[i];
        input->current_buttons[i] = (glfwGetKey(input->window, code_touche) == GLFW_PRESS);
    }

    /* 2. MISE À JOUR DU STICK ANALOGIQUE (Simulé ici par les touches ZQSD / WASD) */
    float stick_x = 0.0f;
    float stick_y = 0.0f;
    if (glfwGetKey(input->window, GLFW_KEY_D) == GLFW_PRESS) stick_x += 1.0f;
    if (glfwGetKey(input->window, GLFW_KEY_Q) == GLFW_PRESS) stick_x -= 1.0f;
    if (glfwGetKey(input->window, GLFW_KEY_Z) == GLFW_PRESS) stick_y += 1.0f;
    if (glfwGetKey(input->window, GLFW_KEY_S) == GLFW_PRESS) stick_y -= 1.0f;

    /* Application mathématique de la zone morte */
    float longueur_stick = sqrtf(stick_x * stick_x + stick_y * stick_y);
    if (longueur_stick < input->joystick_deadzone) {
        input->joystick_axes.x = 0.0f;
        input->joystick_axes.y = 0.0f;
    } else {
        /* Normalisation si le vecteur dépasse 1.0 */
        if (longueur_stick > 1.0f) {
            stick_x /= longueur_stick;
            stick_y /= longueur_stick;
        }
        input->joystick_axes.x = stick_x;
        input->joystick_axes.y = stick_y;
    }

    /* 3. MISE À JOUR DE L'ÉCRAN TACTILE (Conversion de la souris PC) */
    double souris_x, souris_y;
    glfwGetCursorPos(input->window, &souris_x, &souris_y);
    bool clic_souris = (glfwGetMouseButton(input->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

    /* Vérification stricte : le clic est-il confiné dans la moitié inférieure de la fenêtre globale ? */
    bool est_dans_ecran_bas = (souris_x >= 0 && souris_x < input->largeur_ecran &&
                               souris_y >= input->hauteur_ecran && souris_y < (input->hauteur_ecran * 2));

    if (clic_souris && est_dans_ecran_bas) {
        /* Traduction en coordonnées locales à l'écran du bas (Y repart de zéro) */
        float local_x = (float)souris_x;
        float local_y = (float)(souris_y - input->hauteur_ecran);

        if (!input->was_touching_last_frame) {
            input->touch_current.phase = DUAL_TOUCH_STARTED;
        } else {
            if (local_x != input->touch_current.position.x || local_y != input->touch_current.position.y) {
                input->touch_current.phase = DUAL_TOUCH_MOVED;
            } else {
                input->touch_current.phase = DUAL_TOUCH_HELD;
            }
        }
        input->touch_current.position.x = local_x;
        input->touch_current.position.y = local_y;
        input->was_touching_last_frame = true;
    } else {
        /* Pas de contact ou sortie de l'écran tactile */
        if (input->was_touching_last_frame) {
            input->touch_current.phase = DUAL_TOUCH_ENDED;
            input->was_touching_last_frame = false;
        } else {
            input->touch_current.phase = DUAL_TOUCH_NONE;
        }
    }
}

/* ============================================================================
 * Fonctions de requêtes d'état (API Publique)
 * ========================================================================== */

bool DUAL_IsButtonDown(const DUAL_InputManager* input, DUAL_Button bouton) {
    /* (int) force une comparaison signée : selon le compilateur, un enum dont
     * toutes les valeurs sont positives peut être représenté par un type non
     * signé, ce qui rendrait "bouton < 0" toujours faux et masquerait un appel
     * invalide (ex: cast direct d'un -1). */
    if (!input || (int)bouton < 0 || bouton >= DUAL_BUTTON_COUNT) return false;
    return input->current_buttons[bouton];
}

bool DUAL_IsButtonPressed(const DUAL_InputManager* input, DUAL_Button bouton) {
    if (!input || (int)bouton < 0 || bouton >= DUAL_BUTTON_COUNT) return false;
    return (input->current_buttons[bouton] && !input->previous_buttons[bouton]);
}

bool DUAL_IsButtonReleased(const DUAL_InputManager* input, DUAL_Button bouton) {
    if (!input || (int)bouton < 0 || bouton >= DUAL_BUTTON_COUNT) return false;
    return (!input->current_buttons[bouton] && input->previous_buttons[bouton]);
}

DUAL_Vec2 DUAL_GetJoystickAxis(const DUAL_InputManager* input, DUAL_JoystickID joystick) {
    DUAL_Vec2 v_zero = {0.0f, 0.0f};
    if (!input || joystick != DUAL_JOYSTICK_PRINCIPAL) return v_zero;
    return input->joystick_axes;
}

void DUAL_SetJoystickDeadzone(DUAL_InputManager* input, DUAL_JoystickID joystick, float taille_zone_morte) {
    if (input && joystick == DUAL_JOYSTICK_PRINCIPAL) {
        if (taille_zone_morte >= 0.0f && taille_zone_morte <= 1.0f) {
            input->joystick_deadzone = taille_zone_morte;
        }
    }
}

DUAL_TouchState DUAL_GetTouchState(const DUAL_InputManager* input) {
    DUAL_TouchState t_none = {DUAL_TOUCH_NONE, {0.0f, 0.0f}};
    if (!input) return t_none;
    return input->touch_current;
}

bool DUAL_IsTouching(const DUAL_InputManager* input) {
    if (!input) return false;
    return (input->touch_current.phase == DUAL_TOUCH_STARTED ||
            input->touch_current.phase == DUAL_TOUCH_MOVED ||
            input->touch_current.phase == DUAL_TOUCH_HELD);
}