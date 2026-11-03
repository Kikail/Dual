//
// Created by killian on 11/3/26.
//

#ifndef DUAL_DUAL_ENUMS_H
#define DUAL_DUAL_ENUMS_H

/**
 * @brief Niveaux de sévérité utilisés par le système de log interne de libdual.
 */
typedef enum DUAL_LogLevel {
    DUAL_LOG_DEBUG   = 0, /**< Informations de débogage détaillées. */
    DUAL_LOG_INFO    = 1, /**< Informations générales sur le déroulement de l'application. */
    DUAL_LOG_WARNING = 2, /**< Avertissement n'empêchant pas l'exécution. */
    DUAL_LOG_ERROR   = 3  /**< Erreur critique nécessitant l'attention du développeur. */
} DUAL_LogLevel;

/**
 * @brief Disposition de l'ecran dans dual.
 */
typedef enum DUAL_ScreenLayout {
    DUAL_LAYOUT_HORIZONTAL_SPLIT,
    DUAL_LAYOUT_VERTICAL_SPLIT,
    DUAL_LAYOUT_NO_SPLIT
}DUAL_ScreenLayout;

/**
 * @brief Identifie l'un des deux écrans physiques de la console.
 */
typedef enum DUAL_ScreenID {
    DUAL_SCREEN_TOP    = 0, /**< Écran supérieur (généralement utilisé pour le rendu principal). */
    DUAL_SCREEN_BOTTOM = 1, /**< Écran inférieur (généralement tactile, utilisé pour l'UI/HUD). */
    DUAL_SCREEN_RIGHT   = 2, /**< Écran Droit (généralement utilisé pour le rendu principal). */
    DUAL_SCREEN_LEFT  = 3, /**< Écran Gauche (généralement tactile, utilisé pour l'UI/HUD). */
    DUAL_SCREEN_MAIN  = 4,
    DUAL_SCREEN_COUNT  = 5  /**< Nombre total d'écrans gérés par la console. */
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
 * @brief Channels audio
 */
typedef enum DUAL_AudioChannel {
    DUAL_CHANNEL_MASTER = 0,    /**< Audio channel general. */
    DUAL_CHANNEL_SFX    = 1,    /**< Audio pour les sound fx. */
    DUAL_CHANNEL_MUSIC  = 2,    /**< Audio pour les musiques. */
    DUAL_CHANNEL_VOICE  = 3     /**< Audio pour les voix. */
} DUAL_AudioChannel;

// Ajouter du contenu dans cette enum pour ajouter des nouveaux composants pour les entity
// Une entity peut etre que de un seul type pour faciliter la manipulation
/**
 * @brief Types d'entites dans une scene
 */
typedef enum DUAL_Editor_EntityType {
    DUAL_ENTITY_TYPE_NONE,          /**< Entite sans type */
    DUAL_ENTITY_TYPE_MODEL,         /**< Entite de type model */
    DUAL_ENTITY_TYPE_COLLISION,     /**< Entite de type collision */
    DUAL_ENTITY_TYPE_STRUCT         /**< Entite de type struct */
}DUAL_Editor_EntityType;

/**
 * @brief Mode d'ouverture d'un fichier de sauvegarde.
 */
typedef enum DUAL_SaveMode {
    DUAL_SAVE_MODE_LECTURE           = 0, /**< Ouverture en lecture seule. */
    DUAL_SAVE_MODE_ECRITURE          = 1, /**< Ouverture en écriture, écrase le contenu existant (de façon atomique, voir note du fichier). */
    DUAL_SAVE_MODE_LECTURE_ECRITURE  = 2  /**< Ouverture en lecture et écriture, en préservant le contenu existant. */
} DUAL_SaveMode;

/**
 * @brief État de présence et de validité de la cartouche physique.
 */
typedef enum DUAL_CartridgeStatus {
    DUAL_CARTRIDGE_ABSENTE     = 0, /**< Aucune cartouche détectée dans le lecteur. */
    DUAL_CARTRIDGE_VALIDE      = 1, /**< Cartouche détectée et lisible. */
    DUAL_CARTRIDGE_CORROMPUE   = 2  /**< Cartouche détectée mais données illisibles ou invalides. */
} DUAL_CartridgeStatus;

/**
 * @brief Mouvements de la camera
 */
typedef enum CameraMovement {
    CAMERA_MOVEMENT_FORWARD,    /**< Deplacement vers l'avant */
    CAMERA_MOVEMENT_BACKWARD,   /**< Deplacement vers l'arriere */
    CAMERA_MOVEMENT_LEFT,       /**< Deplacement vers la gauche */
    CAMERA_MOVEMENT_RIGHT       /**< Deplacement vers la droite */
}CameraMovement;

/**
 * @brief Mode de filtrage appliqué à une texture lors de son échantillonnage.
 */
typedef enum DUAL_TextureFilter {
    DUAL_FILTER_NEAREST = 0, /**< Filtrage au plus proche voisin (style pixel art). */
    DUAL_FILTER_LINEAR  = 1  /**< Filtrage bilinéaire (lissage des bords). */
} DUAL_TextureFilter;

/**
 * @brief Mode de mélange (blending) utilisé lors du dessin d'un sprite.
 */
typedef enum DUAL_BlendMode {
    DUAL_BLEND_ALPHA    = 0, /**< Mélange alpha standard (transparence classique). */
    DUAL_BLEND_ADDITIVE = 1, /**< Mélange additif (effets lumineux, particules). */
    DUAL_BLEND_NONE     = 2  /**< Aucun mélange, écrasement opaque des pixels. */
} DUAL_BlendMode;

typedef enum DUAL_Renderer2d_Shaders {
    DUAL_SHADER2D_DEFAULT
}DUAL_Renderer2d_Shaders;

typedef enum DUAL_Renderer3d_Shaders {
    DUAL_SHADER3D_LIT,
    DUAL_SHADER3D_UNLIT,
    DUAL_SHADER3D_SKELETAL_LIT,
    DUAL_SHADER3D_SKELETAL_UNLIT,
    DUAL_SHADER3D_SHADER_DEBUG
}DUAL_Renderer3d_Shaders;

typedef enum DUAL_ProjectionMode3D {
    DUAL_PROJECTION_PERSPECTIVE  = 0,
    DUAL_PROJECTION_ORTHOGRAPHIC = 1
} DUAL_ProjectionMode3D;

typedef enum DUAL_CullMode {
    DUAL_CULL_BACK  = 0,
    DUAL_CULL_FRONT = 1,
    DUAL_CULL_NONE  = 2
} DUAL_CullMode;

typedef enum DUAL_RenderMode3D {
    DUAL_RENDER_LIT       = 0,
    DUAL_RENDER_UNLIT     = 1,
    DUAL_RENDER_WIREFRAME = 2
} DUAL_RenderMode3D;

typedef enum DUAL_ShaderRender {
    DUAL_SHADER_LIT       = 0,
    DUAL_SHADER_UNLIT     = 1,
} DUAL_ShaderRender;

/**
 * @brief Identifie chacun des boutons physiques de la console.
 */
typedef enum DUAL_Button {
    DUAL_BUTTON_A          = 0,  /**< Bouton d'action principal A. */
    DUAL_BUTTON_B          = 1,  /**< Bouton d'action secondaire B. */
    DUAL_BUTTON_X          = 2,  /**< Bouton d'action X. */
    DUAL_BUTTON_Y          = 3,  /**< Bouton d'action Y. */
    DUAL_BUTTON_L          = 4,  /**< Gâchette/épaule gauche. */
    DUAL_BUTTON_R          = 5,  /**< Gâchette/épaule droite. */
    DUAL_BUTTON_START      = 6,  /**< Bouton de démarrage / menu pause. */
    DUAL_BUTTON_SELECT     = 7,  /**< Bouton de sélection / menu secondaire. */
    DUAL_BUTTON_UP         = 8,  /**< Direction haut de la croix directionnelle. */
    DUAL_BUTTON_DOWN       = 9,  /**< Direction bas de la croix directionnelle. */
    DUAL_BUTTON_LEFT       = 10, /**< Direction gauche de la croix directionnelle. */
    DUAL_BUTTON_RIGHT      = 11, /**< Direction droite de la croix directionnelle. */
    DUAL_BUTTON_KEYBOARD_W = 12,
    DUAL_BUTTON_KEYBOARD_A = 13,
    DUAL_BUTTON_KEYBOARD_S = 14,
    DUAL_BUTTON_KEYBOARD_D = 15,
    DUAL_BUTTON_KEYBOARD_SPACE = 16,
    DUAL_BUTTON_KEYBOARD_ESCAPE = 17,
    DUAL_BUTTON_MOUSE_LEFT = 18,
    DUAL_BUTTON_MOUSE_RIGHT = 19,
    DUAL_BUTTON_COUNT      = 20,  /**< Nombre total de boutons physiques gérés. */
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

// Types de colliders gérés par le moteur
typedef enum {
    DUAL_COLLIDER_NONE = 0,
    DUAL_COLLIDER_AABB,
    DUAL_COLLIDER_SPHERE,
    DUAL_COLLIDER_CAPSULE
} DUAL_ColliderType;

/**
 * @brief Catégorie de mémoire ciblée par une allocation.
 */
typedef enum DUAL_MemoryType {
    DUAL_MEMORY_VRAM = 0, /**< Mémoire vidéo : textures, buffers de modèles 3D. */
    DUAL_MEMORY_RAM  = 1  /**< Mémoire système : sons, données de jeu génériques. */
} DUAL_MemoryType;

/**
 * @brief Catégorie fonctionnelle d'une ressource suivie, utilisée pour le
 *        reporting et le déchargement sélectif.
 */
typedef enum DUAL_ResourceCategory {
    DUAL_RESOURCE_TEXTURE           = 0, /**< Texture 2D ou spritesheet. */
    DUAL_RESOURCE_MODEL_3D          = 1, /**< Modèle 3D (géométrie, OBJ importé). */
    DUAL_RESOURCE_SOUND             = 2, /**< Son chargé intégralement en mémoire. */
    DUAL_RESOURCE_MUSIC             = 3, /**< Flux musical en streaming. */
    DUAL_RESOURCE_STRUCT            = 4, /**< Structure personnalisee comme par exemple un personnage. */
    DUAL_RESOURCE_ANIMATED_MODEL    = 5,
    DUAL_RESOURCE_FONT              = 6,
    DUAL_RESOURCE_AUTRE             = 7  /**< Ressource générique non catégorisée. */
} DUAL_ResourceCategory;

#endif //DUAL_DUAL_ENUMS_H
