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

#include "../DUAL_Math/dual_math.h"
#include "dual_color.h"
#include "dual_enums.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct DUAL_App {
    GLFWwindow* window;
    int32_t     largeur_ecran;
    int32_t     hauteur_ecran;
    bool        should_close;

    double      time_current;
    double      time_previous;
    double      delta_time;

    double      fps_timer;
    int32_t     fps_counter;
    int32_t     fps_current;

    DUAL_ScreenLayout layout;
    DUAL_Color clear_colors[2];
}DUAL_App;

/**
 * @brief Paramètres d'initialisation de l'application libdual.
 *
 * Cette structure est transmise à DUAL_Init() pour configurer la fenêtre,
 * le titre, et les options de rendu de base. Contrairement aux types opaques,
 * cette structure est publique car le développeur doit pouvoir la construire
 * directement.
 */
typedef struct DUAL_AppConfig {
    const char* titre_fenetre;          /**< Titre affiché dans la barre de fenêtre (mode développement desktop). */
    int32_t     largeur_ecran;          /**< Largeur en pixels de chaque écran logique. */
    int32_t     hauteur_ecran;          /**< Hauteur en pixels de chaque écran logique. */
    DUAL_ScreenLayout screenLayout;     /**< Disposition de l'ecran */
    bool        plein_ecran;            /**< Si vrai, lance la fenêtre en mode plein écran (simulateur desktop). */
    bool        vsync_actif;            /**< Si vrai, active la synchronisation verticale. */
    int32_t     fps_cible;              /**< Fréquence d'images par seconde cible (0 = illimité). */
} DUAL_AppConfig;

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

/**
 * @brief Permet de changer la couleur de fond d'un ecran
 *
 * Certaines couleurs predefinies sont disponibles comme :
 *      DUAL_COLOR_RED
 *      DUAL_COLOR_GREEN
 *      DUAL_COLOR_BLUE
 *
 * @param app Instance de l'application.
 * @param screenId Id correspondant a l'ecran cible [DUAL_SCREEN_TOP, DUAL_SCREEN_BOTTOM]
 * @param clearColor Couleur que nous voulons appliquer pour le fond
 */
void DUAL_SetScreenClearColor(DUAL_App* app,DUAL_ScreenID screenId, DUAL_Color clearColor);

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

/**
 * @brief Retourne un pointeur visant la fenetre GLFW
 *
 * @param app Instance de l'application.
 * @return Pointeur vers la fenetre GLFW.
 */
GLFWwindow* DUAL_GetWindow(const DUAL_App* app);

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


DUAL_ScreenLayout DUAL_GetScreenLayout(const DUAL_App* app);

void* DUAL_GetGLFWWindow(const DUAL_App* app);

void  DUAL_GetScreenDimensions(const DUAL_App* app, int32_t* out_w, int32_t* out_h);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_CORE_H */
