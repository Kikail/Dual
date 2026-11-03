//
// Created by killian on 11/3/26.
//

#ifndef DUAL_DUAL_COLOR_H
#define DUAL_DUAL_COLOR_H

/**
 * @brief Couleur RGBA avec composantes flottantes normalisées entre 0.0 et 1.0.
 */
typedef struct DUAL_Color {
    float r; /**< Composante rouge, entre 0.0 et 1.0. */
    float g; /**< Composante verte, entre 0.0 et 1.0. */
    float b; /**< Composante bleue, entre 0.0 et 1.0. */
    float a; /**< Composante alpha (opacité), entre 0.0 et 1.0. */
} DUAL_Color;

#define DUAL_COLOR_RED      (DUAL_Color){1.0, 0.0, 0.0, 1.0}
#define DUAL_COLOR_GREEN    (DUAL_Color){0.0, 1.0, 0.0, 1.0}
#define DUAL_COLOR_BLUE     (DUAL_Color){0.0, 0.0, 1.0, 1.0}
#define DUAL_COLOR_YELLOW   (DUAL_Color){1.0, 1.0, 0.0, 1.0}
#define DUAL_COLOR_MAGENTA  (DUAL_Color){1.0, 0.0, 1.0, 1.0}
#define DUAL_COLOR_CYAN     (DUAL_Color){0.0, 1.0, 1.0, 1.0}
#define DUAL_COLOR_WHITE    (DUAL_Color){1.0, 1.0, 1.0, 1.0}
#define DUAL_COLOR_BLACK    (DUAL_Color){0.0, 0.0, 0.0, 1.0}

#define DUAL_VEC3_COLOR_RED     (DUAL_Vec3){1.0, 0.0, 0.0}
#define DUAL_VEC3_COLOR_GREEN   (DUAL_Vec3){0.0, 1.0, 0.0}
#define DUAL_VEC3_COLOR_BLUE    (DUAL_Vec3){0.0, 0.0, 1.0}
#define DUAL_VEC3_COLOR_YELLOW  (DUAL_Vec3){1.0, 1.0, 0.0}
#define DUAL_VEC3_COLOR_MAGENTA (DUAL_Vec3){1.0, 0.0, 1.0}
#define DUAL_VEC3_COLOR_CYAN    (DUAL_Vec3){0.0, 1.0, 1.0}
#define DUAL_VEC3_COLOR_WHITE   (DUAL_Vec3){1.0, 1.0, 1.0}
#define DUAL_VEC3_COLOR_BLACK   (DUAL_Vec3){0.0, 0.0, 0.0}

#endif //DUAL_DUAL_COLOR_H
