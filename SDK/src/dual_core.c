#include "dual_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "dual_math.h"

/* ============================================================================
 * Définition de la structure opaque (Invisible pour le dev final)
 * ========================================================================== */
typedef struct DUAL_App {
    GLFWwindow* window;         /* Le handle de la fenêtre de test PC */
    int32_t     largeur_ecran;   /* Largeur d'UN écran logique */
    int32_t     hauteur_ecran;   /* Hauteur d'UN écran logique */
    bool        should_close;    /* Flag d'arrêt forcé */

    /* Variables de Timing */
    double      time_current;    /* Temps de la frame actuelle */
    double      time_previous;   /* Temps de la frame précédente */
    double      delta_time;      /* Temps écoulé entre les deux frames */

    /* Variables de calcul de FPS */
    double      fps_timer;
    int32_t     fps_counter;
    int32_t     fps_current;

    DUAL_Color screenTopClearColor;
    DUAL_Color screenBottomClearColor;
}DUAL_App;

/* ============================================================================
 * Cycle de vie de l'application
 * ========================================================================== */

DUAL_Result DUAL_Init(const DUAL_AppConfig* config, DUAL_App** out_app) {
    if (!config || !out_app) {
        return DUAL_ERROR_INVALID_ARG;
    }

    /* 1. Tenter d'allouer la structure opaque */
    DUAL_App* app = (DUAL_App*)malloc(sizeof(struct DUAL_App));
    if (!app) {
        DUAL_Log(DUAL_LOG_ERROR, "Échec d'allocation mémoire pour DUAL_App.");
        return DUAL_ERROR_OUT_OF_MEMORY;
    }

    /* 2. Initialisation de GLFW */
    if (!glfwInit()) {
        DUAL_Log(DUAL_LOG_ERROR, "Impossible d'initialiser GLFW.");
        free(app);
        return DUAL_ERROR_INIT_FAILED;
    }

    /* Configuration OpenGL (Cible OpenGL 3.3 Core Profile / ES jumeau) */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    /* 3. Calcul de la hauteur de la fenêtre virtuelle (Cumul vertical) */
    int32_t hauteur_virtuelle = config->hauteur_ecran * 2;

    /* 4. Création de la fenêtre */
    GLFWmonitor* moniteur = config->plein_ecran ? glfwGetPrimaryMonitor() : NULL;
    app->window = glfwCreateWindow(config->largeur_ecran, hauteur_virtuelle, config->titre_fenetre, moniteur, NULL);

    if (!app->window) {
        DUAL_Log(DUAL_LOG_ERROR, "Impossible de créer la fenêtre virtuelle GLFW.");
        glfwTerminate();
        free(app);
        return DUAL_ERROR_INIT_FAILED;
    }

    glfwMakeContextCurrent(app->window);
    glfwSetInputMode(app->window, GLFW_STICKY_KEYS, GLFW_TRUE);

    /* INITIALISATION DE GLAD */
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        DUAL_Log(DUAL_LOG_ERROR, "Échec d'initialisation de GLAD (chargement des fonctions OpenGL).");
        glfwDestroyWindow(app->window);
        glfwTerminate();
        free(app);
        return DUAL_ERROR_INIT_FAILED;
    }

    /* 5. Post-Configuration du matériel */
    if (config->vsync_actif) {
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }

    /* Remplissage des données de notre structure interne */
    app->largeur_ecran  = config->largeur_ecran;
    app->hauteur_ecran  = config->hauteur_ecran;
    app->should_close   = false;
    app->time_previous  = glfwGetTime();
    app->time_current   = app->time_previous;
    app->delta_time     = 0.0;
    app->fps_timer      = 0.0;
    app->fps_counter    = 0;
    app->fps_current    = 0;

    /* Export du pointeur vers l'application du dev */
    *out_app = app;

    DUAL_Log(DUAL_LOG_INFO, "Système Dual initialisé avec succès (%dx%d par écran).", app->largeur_ecran, app->hauteur_ecran);

    app->screenTopClearColor = (DUAL_Color){0.1f, 0.6f, 0.2f, 1.0f};
    app->screenBottomClearColor = (DUAL_Color){0.1f, 0.3f, 0.6f, 1.0f};

    return DUAL_OK;
}

void DUAL_Shutdown(DUAL_App* app) {
    if (app) {
        if (app->window) {
            glfwDestroyWindow(app->window);
        }
        glfwTerminate();
        DUAL_Log(DUAL_LOG_INFO, "Système Dual arrêté proprement.");
        free(app);
    }
}

bool DUAL_ShouldRun(const DUAL_App* app) {
    if (!app) return false;
    return (!glfwWindowShouldClose(app->window) && !app->should_close);
}

void DUAL_RequestExit(DUAL_App* app) {
    if (app) {
        app->should_close = true;
    }
}

/* ============================================================================
 * Boucle de rendu / frame
 * ========================================================================== */

void DUAL_BeginFrame(DUAL_App* app) {
    if (!app) return;

    /* Polling des inputs/évènements système Windows/Linux */
    glfwPollEvents();

    /* Calcul du Delta Time précis */
    app->time_current = glfwGetTime();
    app->delta_time   = app->time_current - app->time_previous;
    app->time_previous = app->time_current;

    /* Calcul lissé des FPS */
    app->fps_timer += app->delta_time;
    app->fps_counter += 1;
    if (app->fps_timer >= 1.0) {
        app->fps_current = app->fps_counter;
        app->fps_counter = 0;
        app->fps_timer   = 0.0;
    }

    // ─── RENDU ÉCRAN DU HAUT (VERT) ───
    DUAL_SetActiveScreen(app, DUAL_SCREEN_TOP);
    glClearColor(app->screenTopClearColor.r, app->screenTopClearColor.g, app->screenTopClearColor.b, app->screenTopClearColor.a); // Vert sapin
    glClear(GL_COLOR_BUFFER_BIT);

    // ─── RENDU ÉCRAN DU BAS (BLEU) ───
    DUAL_SetActiveScreen(app, DUAL_SCREEN_BOTTOM);
    glClearColor(app->screenBottomClearColor.r,app->screenBottomClearColor.g,app->screenBottomClearColor.b,app->screenBottomClearColor.a); // Bleu océan
    glClear(GL_COLOR_BUFFER_BIT);

    //DUAL_Log(DUAL_LOG_DEBUG, "FPS: %d", app->fps_current);
}

void DUAL_SetActiveScreen(DUAL_App* app, DUAL_ScreenID screen) {
    if (!app) return;

    // 1. On active le mode "Scissor Test" dans OpenGL.
    // Cela force OpenGL à ne colorer QUE la zone découpée, sinon glClear colorie toute la fenêtre.
    glEnable(GL_SCISSOR_TEST);

    if (screen == DUAL_SCREEN_TOP) {
        // Écran du HAUT : la zone commence à Y = hauteur_ecran
        glViewport(0, app->hauteur_ecran, app->largeur_ecran, app->hauteur_ecran);
        glScissor(0, app->hauteur_ecran, app->largeur_ecran, app->hauteur_ecran);
    }
    else if (screen == DUAL_SCREEN_BOTTOM) {
        // Écran du BAS : la zone commence à Y = 0
        glViewport(0, 0, app->largeur_ecran, app->hauteur_ecran);
        glScissor(0, 0, app->largeur_ecran, app->hauteur_ecran);
    }
}

void DUAL_EndFrame(DUAL_App* app) {
    if (!app) return;

    /* Affichage de la frame sur le moniteur */
    glfwSwapBuffers(app->window);
}

void DUAL_SetScreenClearColor(DUAL_App* app,DUAL_ScreenID screenId, DUAL_Color clearColor) {
    if (!app) return;

    switch (screenId) {
        case DUAL_SCREEN_TOP:
            app->screenTopClearColor = clearColor;
            break;
        case DUAL_SCREEN_BOTTOM:
            app->screenBottomClearColor = clearColor;
            break;
        default:
            break;
    }
}

/* ============================================================================
 * Timing
 * ========================================================================== */

double DUAL_GetDeltaTime(const DUAL_App* app) {
    return app ? app->delta_time : 0.0;
}

double DUAL_GetTime(const DUAL_App* app) {
    return glfwGetTime();
}

int32_t DUAL_GetFPS(const DUAL_App* app) {
    return app ? app->fps_current : 0;
}

/* ============================================================================
 * Logging
 * ========================================================================== */

void DUAL_Log(DUAL_LogLevel niveau, const char* format, ...) {
    const char* prefix = "[INFO]";
    switch (niveau) {
        case DUAL_LOG_DEBUG:   prefix = "[DEBUG]";   break;
        case DUAL_LOG_INFO:    prefix = "[INFO]";    break;
        case DUAL_LOG_WARNING: prefix = "[WARN]";    break;
        case DUAL_LOG_ERROR:   prefix = "[ERROR]";   break;
    }

    printf("%s ", prefix);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
}

GLFWwindow* DUAL_GetWindow(const DUAL_App* app) {
    return app ? app->window : NULL;
}

/* ============================================================================
 * Utility
 * ========================================================================== */

void* DUAL_Internal_GetGLFWWindow(const DUAL_App* app) {
    return app ? app->window : NULL;
}

void DUAL_Internal_GetScreenDimensions(const DUAL_App* app, int32_t* out_w, int32_t* out_h) {
    if (app) {
        if (out_w) *out_w = app->largeur_ecran;
        if (out_h) *out_h = app->hauteur_ecran;
    }
}