/**
 * @file dual_core.h
 * @brief Module central de libdual : cycle de vie de l'application, gestion de la fenêtre
 *        double écran (basée sur GLFW) et primitives de timing.
 *
 * Ce module est le point d'entrée obligatoire de tout programme utilisant libdual.
 * Il initialise le contexte GLFW/OpenGL sous-jacent, gère la boucle de rendu pour
 * les deux écrans physiques de la console, et fournit les outils de mesure du temps
 * nécessaires à une boucle de jeu stable (delta time, FPS, limitation de fréquence).
 *
 * @note Aucune fonction d'un autre module de libdual ne doit être appelée avant
 *       DUAL_Init() et après DUAL_Shutdown().
 */

#ifndef DUAL_CORE_H
#define DUAL_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  Types opaques
 * ========================================================================== */

/**
 * @brief Représente le contexte global de l'application (fenêtre, écrans, GLFW).
 *
 * Ce type est opaque : sa structure interne (handle GLFWwindow*, framebuffers
 * OpenGL des deux écrans, etc.) n'est jamais exposée au développeur final.
 */
typedef struct DUAL_App DUAL_App;

/* ============================================================================
 *  Enumérations
 * ========================================================================== */

/**
 * @brief Identifie l'un des deux écrans physiques de la console.
 */
typedef enum DUAL_ScreenID {
    DUAL_SCREEN_TOP    = 0, /**< Écran supérieur (généralement utilisé pour le rendu principal). */
    DUAL_SCREEN_BOTTOM = 1, /**< Écran inférieur (généralement tactile, utilisé pour l'UI/HUD). */
    DUAL_SCREEN_COUNT  = 2  /**< Nombre total d'écrans gérés par la console. */
} DUAL_ScreenID;

/**
 * @brief Codes de retour standards utilisés à travers toute l'API libdual.
 */
typedef enum DUAL_Result {
    DUAL_OK                  =  0, /**< Opération réussie. */
    DUAL_ERROR_UNKNOWN       = -1, /**< Erreur non spécifiée. */
    DUAL_ERROR_INIT_FAILED   = -2, /**< Échec d'initialisation de GLFW, OpenGL ou du matériel. */
    DUAL_ERROR_INVALID_ARG   = -3, /**< Argument invalide passé à une fonction de l'API. */
    DUAL_ERROR_OUT_OF_MEMORY = -4, /**< Allocation mémoire échouée. */
    DUAL_ERROR_NOT_FOUND     = -5  /**< Ressource ou fichier introuvable. */
} DUAL_Result;

/**
 * @brief Niveaux de sévérité utilisés par le système de log interne de libdual.
 */
typedef enum DUAL_LogLevel {
    DUAL_LOG_DEBUG   = 0, /**< Informations de débogage détaillées. */
    DUAL_LOG_INFO    = 1, /**< Informations générales sur le déroulement de l'application. */
    DUAL_LOG_WARNING = 2, /**< Avertissement n'empêchant pas l'exécution. */
    DUAL_LOG_ERROR   = 3  /**< Erreur critique nécessitant l'attention du développeur. */
} DUAL_LogLevel;

/* ============================================================================
 *  Structures de configuration
 * ========================================================================== */

/**
 * @brief Paramètres d'initialisation de l'application libdual.
 *
 * Cette structure est transmise à DUAL_Init() pour configurer la fenêtre,
 * le titre, et les options de rendu de base. Contrairement aux types opaques,
 * cette structure est publique car le développeur doit pouvoir la construire
 * directement.
 */
typedef struct DUAL_AppConfig {
    const char* titre_fenetre;     /**< Titre affiché dans la barre de fenêtre (mode développement desktop). */
    int32_t     largeur_ecran;     /**< Largeur en pixels de chaque écran logique. */
    int32_t     hauteur_ecran;     /**< Hauteur en pixels de chaque écran logique. */
    bool        plein_ecran;       /**< Si vrai, lance la fenêtre en mode plein écran (simulateur desktop). */
    bool        vsync_actif;       /**< Si vrai, active la synchronisation verticale. */
    int32_t     fps_cible;         /**< Fréquence d'images par seconde cible (0 = illimité). */
} DUAL_AppConfig;

/* ============================================================================
 *  Cycle de vie de l'application
 * ========================================================================== */

/**
 * @brief Initialise libdual, ouvre la fenêtre/le contexte graphique et prépare les deux écrans.
 *
 * Doit être la toute première fonction de libdual appelée dans le programme.
 * Initialise en interne GLFW et le contexte OpenGL associé.
 *
 * @param config Pointeur vers une structure DUAL_AppConfig décrivant les paramètres souhaités.
 * @param out_app Pointeur vers un DUAL_App* qui recevra l'instance allouée en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_Init(const DUAL_AppConfig* config, DUAL_App** out_app);

/**
 * @brief Libère toutes les ressources allouées par DUAL_Init() et ferme la fenêtre.
 *
 * Doit être appelée une seule fois, après la fin de la boucle principale,
 * avant la fin du programme.
 *
 * @param app Instance de l'application à détruire.
 */
void DUAL_Shutdown(DUAL_App* app);

/**
 * @brief Indique si l'application doit continuer à s'exécuter.
 *
 * Cette fonction doit être utilisée comme condition de la boucle principale du jeu.
 * Elle devient fausse lorsque l'utilisateur ferme la fenêtre ou que
 * DUAL_RequestExit() a été appelée.
 *
 * @param app Instance de l'application.
 * @return true si la boucle principale doit continuer, false sinon.
 */
bool DUAL_ShouldRun(const DUAL_App* app);

/**
 * @brief Demande l'arrêt propre de la boucle principale au prochain cycle.
 *
 * @param app Instance de l'application.
 */
void DUAL_RequestExit(DUAL_App* app);

/* ============================================================================
 *  Boucle de rendu / frame
 * ========================================================================== */

/**
 * @brief Démarre une nouvelle frame logique et de rendu.
 *
 * Doit être appelée en tout début de boucle, avant tout appel aux fonctions
 * de dessin des modules dual_graphics_2d.h / dual_graphics_3d.h.
 * Traite également les événements système (clavier desktop, fermeture fenêtre).
 *
 * @param app Instance de l'application.
 */
void DUAL_BeginFrame(DUAL_App* app);

/**
 * @brief Sélectionne l'écran cible pour les prochains appels de rendu.
 *
 * Tous les appels de dessin des modules graphiques effectués après cette fonction
 * seront dirigés vers le framebuffer de l'écran spécifié, jusqu'au prochain appel
 * à DUAL_SetActiveScreen() ou à DUAL_EndFrame().
 *
 * @param app Instance de l'application.
 * @param screen Identifiant de l'écran à activer (DUAL_SCREEN_TOP ou DUAL_SCREEN_BOTTOM).
 */
void DUAL_SetActiveScreen(DUAL_App* app, DUAL_ScreenID screen);

/**
 * @brief Termine la frame courante, présente les deux framebuffers à l'écran et échange les buffers.
 *
 * Doit être le dernier appel de la boucle principale pour la frame en cours.
 *
 * @param app Instance de l'application.
 */
void DUAL_EndFrame(DUAL_App* app);

/* ============================================================================
 *  Timing
 * ========================================================================== */

/**
 * @brief Retourne le temps écoulé depuis la frame précédente, en secondes.
 *
 * Valeur typiquement utilisée pour multiplier les vitesses de déplacement
 * afin de rendre la simulation indépendante du framerate.
 *
 * @param app Instance de l'application.
 * @return Delta-temps en secondes (ex: 0.0166 pour 60 FPS).
 */
double DUAL_GetDeltaTime(const DUAL_App* app);

/**
 * @brief Retourne le temps total écoulé depuis l'appel à DUAL_Init(), en secondes.
 *
 * @param app Instance de l'application.
 * @return Temps absolu en secondes depuis le démarrage de l'application.
 */
double DUAL_GetTime(const DUAL_App* app);

/**
 * @brief Retourne le nombre d'images par seconde actuel, lissé sur quelques frames.
 *
 * @param app Instance de l'application.
 * @return Valeur de FPS courante.
 */
int32_t DUAL_GetFPS(const DUAL_App* app);

GLFWwindow* DUAL_GetWindow(const DUAL_App* app);

/* ============================================================================
 *  Logging
 * ========================================================================== */

/**
 * @brief Émet un message dans le journal interne de libdual.
 *
 * Utile pour le débogage pendant le développement ; les logs de niveau
 * DUAL_LOG_DEBUG peuvent être désactivés en build de production par le SDK.
 *
 * @param niveau Sévérité du message (voir DUAL_LogLevel).
 * @param format Chaîne de format façon printf.
 * @param ... Arguments variadiques correspondant au format.
 */
void DUAL_Log(DUAL_LogLevel niveau, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_CORE_H */
