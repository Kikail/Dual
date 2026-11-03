/**
 * @file dual_input.h
 * @brief Module d'entrées de libdual : boutons physiques, joystick analogique
 *        et écran tactile inférieur.
 *
 * Ce module fournit un instantané de l'état des entrées à chaque frame.
 * Il doit être interrogé après DUAL_BeginFrame() (voir dual_core.h) afin de
 * garantir la cohérence de l'état lu sur toute la durée de la frame.
 */

#ifndef DUAL_INPUT_H
#define DUAL_INPUT_H

#include <stdint.h>
#include <stdbool.h>
#include "../DUAL_Core/dual_core.h"
#include "../DUAL_Core/dual_enums.h"
#include "../DUAL_Math/dual_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  Types opaques
 * ========================================================================== */

/**
 * @brief Gestionnaire d'entrées, responsable de la capture et de l'historique
 *        des états des boutons, joysticks et écran tactile.
 */
typedef struct DUAL_InputManager DUAL_InputManager;

/**
 * @brief État courant d'un contact tactile sur l'écran inférieur.
 */
typedef struct DUAL_TouchState {
    DUAL_TouchPhase phase;    /**< Phase actuelle du contact tactile. */
    DUAL_Vec2       position; /**< Position du contact, en coordonnées écran (pixels). */
} DUAL_TouchState;

#define MAX_INPUT_SIZE 256
typedef struct {
    char data[MAX_INPUT_SIZE];
    int cursor_pos;
    int length;
    bool active;
} DUAL_TextInputBuffer;

/* ============================================================================
 *  Cycle de vie du gestionnaire
 * ========================================================================== */

/**
 * @brief Crée un nouveau gestionnaire d'entrées associé à l'application.
 *
 * @param app Instance de l'application libdual.
 * @param out_input Pointeur recevant le gestionnaire créé en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_InputManager_Create(DUAL_App* app, DUAL_InputManager** out_input);

/**
 * @brief Détruit un gestionnaire d'entrées.
 *
 * @param input Gestionnaire à détruire.
 */
void DUAL_InputManager_Destroy(DUAL_InputManager* input);

/**
 * @brief Met à jour l'état interne des entrées pour la frame courante.
 *
 * Doit être appelée une fois par frame, typiquement juste après
 * DUAL_BeginFrame(), avant toute lecture d'état des boutons ou du tactile.
 *
 * @param input Gestionnaire d'entrées à mettre à jour.
 */
void DUAL_InputManager_Update(DUAL_InputManager* input);

/* ============================================================================
 *  Boutons physiques
 * ========================================================================== */

/**
 * @brief Indique si un bouton est actuellement maintenu enfoncé.
 *
 * @param input Gestionnaire d'entrées.
 * @param bouton Bouton à interroger.
 * @return true si le bouton est actuellement enfoncé, false sinon.
 */
bool DUAL_IsButtonDown(const DUAL_InputManager* input, DUAL_Button bouton);

/**
 * @brief Indique si un bouton vient d'être enfoncé sur cette frame précisément.
 *
 * @param input Gestionnaire d'entrées.
 * @param bouton Bouton à interroger.
 * @return true si le bouton vient de passer de relâché à enfoncé sur cette frame.
 */
bool DUAL_IsButtonPressed(const DUAL_InputManager* input, DUAL_Button bouton);

/**
 * @brief Indique si un bouton vient d'être relâché sur cette frame précisément.
 *
 * @param input Gestionnaire d'entrées.
 * @param bouton Bouton à interroger.
 * @return true si le bouton vient de passer d'enfoncé à relâché sur cette frame.
 */
bool DUAL_IsButtonReleased(const DUAL_InputManager* input, DUAL_Button bouton);

/* ============================================================================
 *  Joystick analogique
 * ========================================================================== */

/**
 * @brief Retourne la position normalisée d'un joystick analogique.
 *
 * Les composantes x et y sont comprises entre -1.0 et 1.0. Une zone morte
 * (dead zone) est appliquée en interne pour filtrer le bruit du capteur.
 *
 * @param input Gestionnaire d'entrées.
 * @param joystick Identifiant du joystick à interroger.
 * @return Position normalisée du joystick.
 */
DUAL_Vec2 DUAL_GetJoystickAxis(const DUAL_InputManager* input, DUAL_JoystickID joystick);

/**
 * @brief Configure la taille de la zone morte appliquée à un joystick analogique.
 *
 * @param input Gestionnaire d'entrées.
 * @param joystick Identifiant du joystick à configurer.
 * @param taille_zone_morte Rayon de la zone morte, entre 0.0 et 1.0.
 */
void DUAL_SetJoystickDeadzone(DUAL_InputManager* input, DUAL_JoystickID joystick,
                               float taille_zone_morte);

/* ============================================================================
 *  Écran tactile
 * ========================================================================== */

/**
 * @brief Récupère l'état courant du contact tactile sur l'écran inférieur.
 *
 * La console cible ne supportant qu'un point de contact à la fois, cette
 * fonction retourne un état unique plutôt qu'une liste de contacts multiples.
 *
 * @param input Gestionnaire d'entrées.
 * @return État courant du contact tactile.
 */
DUAL_TouchState DUAL_GetTouchState(const DUAL_InputManager* input);

/**
 * @brief Indique si l'écran tactile est actuellement touché (quelle que soit la phase).
 *
 * @param input Gestionnaire d'entrées.
 * @return true si un contact tactile est actif, false sinon.
 */
bool DUAL_IsTouching(const DUAL_InputManager* input);

void text_input_init(DUAL_TextInputBuffer* input);
void text_input_move_cursor(DUAL_TextInputBuffer* input, int direction);
void text_input_insert_char(DUAL_TextInputBuffer* input, char c);
void text_input_backspace(DUAL_TextInputBuffer* input) ;
void character_callback(GLFWwindow* window, unsigned int codepoint);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void setup_text_input_callbacks(GLFWwindow* window, DUAL_TextInputBuffer* input);
char* get_buffer_data(DUAL_InputManager* input);
bool is_buffer_active(DUAL_InputManager* input);
void set_buffer_data_active(DUAL_InputManager* input, bool active);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_INPUT_H */
