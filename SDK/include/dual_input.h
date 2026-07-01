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
#include "dual_core.h"
#include "dual_math.h"

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

/* ============================================================================
 *  Enumérations
 * ========================================================================== */

/**
 * @brief Identifie chacun des boutons physiques de la console.
 */
typedef enum DUAL_Button {
    DUAL_BUTTON_A      = 0,  /**< Bouton d'action principal A. */
    DUAL_BUTTON_B      = 1,  /**< Bouton d'action secondaire B. */
    DUAL_BUTTON_X      = 2,  /**< Bouton d'action X. */
    DUAL_BUTTON_Y      = 3,  /**< Bouton d'action Y. */
    DUAL_BUTTON_L      = 4,  /**< Gâchette/épaule gauche. */
    DUAL_BUTTON_R      = 5,  /**< Gâchette/épaule droite. */
    DUAL_BUTTON_START  = 6,  /**< Bouton de démarrage / menu pause. */
    DUAL_BUTTON_SELECT = 7,  /**< Bouton de sélection / menu secondaire. */
    DUAL_BUTTON_UP     = 8,  /**< Direction haut de la croix directionnelle. */
    DUAL_BUTTON_DOWN   = 9,  /**< Direction bas de la croix directionnelle. */
    DUAL_BUTTON_LEFT   = 10, /**< Direction gauche de la croix directionnelle. */
    DUAL_BUTTON_RIGHT  = 11, /**< Direction droite de la croix directionnelle. */
    DUAL_BUTTON_COUNT  = 12  /**< Nombre total de boutons physiques gérés. */
} DUAL_Button;

/**
 * @brief Identifie le joystick analogique interrogé.
 *
 * @note La majorité des modèles de console cible n'exposent qu'un seul stick
 *       analogique ; cette énumération anticipe les variantes matérielles à deux sticks.
 */
typedef enum DUAL_JoystickID {
    DUAL_JOYSTICK_PRINCIPAL = 0, /**< Joystick analogique principal. */
    DUAL_JOYSTICK_COUNT     = 1  /**< Nombre total de joysticks gérés par le matériel cible. */
} DUAL_JoystickID;

/**
 * @brief Phase d'un contact tactile sur l'écran inférieur.
 */
typedef enum DUAL_TouchPhase {
    DUAL_TOUCH_NONE     = 0, /**< Aucun contact tactile en cours. */
    DUAL_TOUCH_STARTED  = 1, /**< Le contact vient de débuter sur cette frame. */
    DUAL_TOUCH_MOVED    = 2, /**< Le contact est maintenu et sa position a changé. */
    DUAL_TOUCH_HELD     = 3, /**< Le contact est maintenu, position inchangée depuis la dernière frame. */
    DUAL_TOUCH_ENDED    = 4  /**< Le contact vient de se terminer sur cette frame. */
} DUAL_TouchPhase;

/* ============================================================================
 *  Structures
 * ========================================================================== */

/**
 * @brief État courant d'un contact tactile sur l'écran inférieur.
 */
typedef struct DUAL_TouchState {
    DUAL_TouchPhase phase;    /**< Phase actuelle du contact tactile. */
    DUAL_Vec2       position; /**< Position du contact, en coordonnées écran (pixels). */
} DUAL_TouchState;

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

#ifdef __cplusplus
}
#endif

#endif /* DUAL_INPUT_H */
