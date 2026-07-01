/**
 * @file dual_math.h
 * @brief Module mathématique de libdual : vecteurs 2D/3D, matrices, primitives
 *        de collision et fonctions d'interpolation.
 *
 * Ce module ne dépend d'aucun autre module de libdual et peut être inclus
 * indépendamment. Contrairement aux autres modules, les types principaux
 * (vecteurs, matrices, rectangles) sont des structures transparentes (non opaques)
 * car leur manipulation directe par le développeur (accès aux champs x/y/z) est
 * nécessaire pour des raisons de performance.
 */

#ifndef DUAL_MATH_H
#define DUAL_MATH_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  Types vectoriels et matriciels
 * ========================================================================== */

/**
 * @brief Vecteur à deux composantes flottantes, utilisé pour les positions
 *        et directions en 2D.
 */
typedef struct DUAL_Vec2 {
    float x; /**< Composante horizontale. */
    float y; /**< Composante verticale. */
} DUAL_Vec2;

/**
 * @brief Vecteur à trois composantes flottantes, utilisé pour les positions
 *        et directions en 3D.
 */
typedef struct DUAL_Vec3 {
    float x; /**< Composante sur l'axe X. */
    float y; /**< Composante sur l'axe Y. */
    float z; /**< Composante sur l'axe Z. */
} DUAL_Vec3;

/**
 * @brief Vecteur à quatre composantes flottantes, utilisé notamment pour
 *        les couleurs RGBA et les quaternions.
 */
typedef struct DUAL_Vec4 {
    float x; /**< Première composante (ou rouge, si utilisé comme couleur). */
    float y; /**< Deuxième composante (ou vert, si utilisé comme couleur). */
    float z; /**< Troisième composante (ou bleu, si utilisé comme couleur). */
    float w; /**< Quatrième composante (ou alpha, si utilisé comme couleur). */
} DUAL_Vec4;

/**
 * @brief Matrice 4x4 en notation column-major, compatible avec le pipeline OpenGL.
 *
 * Le tableau interne `m` stocke les 16 éléments colonne par colonne,
 * conformément aux conventions OpenGL.
 */
typedef struct DUAL_Mat4 {
    float m[16]; /**< Éléments de la matrice, rangés en column-major. */
} DUAL_Mat4;

/* ============================================================================
 *  Primitives géométriques
 * ========================================================================== */

/**
 * @brief Rectangle axis-aligned défini par sa position (coin supérieur gauche)
 *        et ses dimensions, utilisé pour les collisions et le rendu 2D.
 */
typedef struct DUAL_Rect {
    float x;      /**< Position horizontale du coin supérieur gauche. */
    float y;      /**< Position verticale du coin supérieur gauche. */
    float largeur; /**< Largeur du rectangle. */
    float hauteur; /**< Hauteur du rectangle. */
} DUAL_Rect;

/**
 * @brief Cercle défini par son centre et son rayon, utilisé pour les collisions 2D.
 */
typedef struct DUAL_Circle {
    DUAL_Vec2 centre; /**< Position du centre du cercle. */
    float     rayon;  /**< Rayon du cercle. */
} DUAL_Circle;

/**
 * @brief Boîte englobante alignée sur les axes (AABB) en 3D, utilisée pour
 *        les collisions et le frustum culling.
 */
typedef struct DUAL_AABB {
    DUAL_Vec3 min; /**< Coin minimal de la boîte (x, y, z les plus petits). */
    DUAL_Vec3 max; /**< Coin maximal de la boîte (x, y, z les plus grands). */
} DUAL_AABB;

/**
 * @brief Sphère définie par son centre et son rayon, utilisée pour les collisions 3D.
 */
typedef struct DUAL_Sphere {
    DUAL_Vec3 centre; /**< Position du centre de la sphère. */
    float     rayon;  /**< Rayon de la sphère. */
} DUAL_Sphere;

/* ============================================================================
 *  Opérations sur DUAL_Vec2
 * ========================================================================== */

/**
 * @brief Additionne deux vecteurs 2D.
 * @param a Premier vecteur.
 * @param b Second vecteur.
 * @return Résultat de a + b.
 */
DUAL_Vec2 DUAL_Vec2_Add(DUAL_Vec2 a, DUAL_Vec2 b);

/**
 * @brief Soustrait deux vecteurs 2D.
 * @param a Premier vecteur.
 * @param b Second vecteur.
 * @return Résultat de a - b.
 */
DUAL_Vec2 DUAL_Vec2_Sub(DUAL_Vec2 a, DUAL_Vec2 b);

/**
 * @brief Multiplie un vecteur 2D par un scalaire.
 * @param v Vecteur source.
 * @param scalaire Facteur multiplicatif.
 * @return Vecteur résultant.
 */
DUAL_Vec2 DUAL_Vec2_Scale(DUAL_Vec2 v, float scalaire);

/**
 * @brief Calcule la longueur (norme euclidienne) d'un vecteur 2D.
 * @param v Vecteur à mesurer.
 * @return Longueur du vecteur.
 */
float DUAL_Vec2_Length(DUAL_Vec2 v);

/**
 * @brief Normalise un vecteur 2D (longueur ramenée à 1).
 * @param v Vecteur à normaliser.
 * @return Vecteur normalisé. Retourne {0,0} si le vecteur source est nul.
 */
DUAL_Vec2 DUAL_Vec2_Normalize(DUAL_Vec2 v);

/**
 * @brief Calcule le produit scalaire de deux vecteurs 2D.
 * @param a Premier vecteur.
 * @param b Second vecteur.
 * @return Produit scalaire a . b.
 */
float DUAL_Vec2_Dot(DUAL_Vec2 a, DUAL_Vec2 b);

/**
 * @brief Calcule la distance euclidienne entre deux points 2D.
 * @param a Premier point.
 * @param b Second point.
 * @return Distance entre a et b.
 */
float DUAL_Vec2_Distance(DUAL_Vec2 a, DUAL_Vec2 b);

/* ============================================================================
 *  Opérations sur DUAL_Vec3
 * ========================================================================== */

/**
 * @brief Additionne deux vecteurs 3D.
 * @param a Premier vecteur.
 * @param b Second vecteur.
 * @return Résultat de a + b.
 */
DUAL_Vec3 DUAL_Vec3_Add(DUAL_Vec3 a, DUAL_Vec3 b);

/**
 * @brief Soustrait deux vecteurs 3D.
 * @param a Premier vecteur.
 * @param b Second vecteur.
 * @return Résultat de a - b.
 */
DUAL_Vec3 DUAL_Vec3_Sub(DUAL_Vec3 a, DUAL_Vec3 b);

/**
 * @brief Multiplie un vecteur 3D par un scalaire.
 * @param v Vecteur source.
 * @param scalaire Facteur multiplicatif.
 * @return Vecteur résultant.
 */
DUAL_Vec3 DUAL_Vec3_Scale(DUAL_Vec3 v, float scalaire);

/**
 * @brief Calcule le produit vectoriel de deux vecteurs 3D.
 * @param a Premier vecteur.
 * @param b Second vecteur.
 * @return Vecteur résultant, perpendiculaire à a et b.
 */
DUAL_Vec3 DUAL_Vec3_Cross(DUAL_Vec3 a, DUAL_Vec3 b);

/**
 * @brief Calcule le produit scalaire de deux vecteurs 3D.
 * @param a Premier vecteur.
 * @param b Second vecteur.
 * @return Produit scalaire a . b.
 */
float DUAL_Vec3_Dot(DUAL_Vec3 a, DUAL_Vec3 b);

/**
 * @brief Calcule la longueur (norme euclidienne) d'un vecteur 3D.
 * @param v Vecteur à mesurer.
 * @return Longueur du vecteur.
 */
float DUAL_Vec3_Length(DUAL_Vec3 v);

/**
 * @brief Normalise un vecteur 3D (longueur ramenée à 1).
 * @param v Vecteur à normaliser.
 * @return Vecteur normalisé. Retourne {0,0,0} si le vecteur source est nul.
 */
DUAL_Vec3 DUAL_Vec3_Normalize(DUAL_Vec3 v);

/* ============================================================================
 *  Opérations matricielles (DUAL_Mat4)
 * ========================================================================== */

/**
 * @brief Construit une matrice identité 4x4.
 * @return Matrice identité.
 */
DUAL_Mat4 DUAL_Mat4_Identity(void);

/**
 * @brief Multiplie deux matrices 4x4.
 * @param a Première matrice (appliquée en second).
 * @param b Seconde matrice (appliquée en premier).
 * @return Résultat de a * b.
 */
DUAL_Mat4 DUAL_Mat4_Multiply(DUAL_Mat4 a, DUAL_Mat4 b);

/**
 * @brief Construit une matrice de translation.
 * @param deplacement Vecteur de translation à appliquer.
 * @return Matrice de translation correspondante.
 */
DUAL_Mat4 DUAL_Mat4_Translate(DUAL_Vec3 deplacement);

/**
 * @brief Construit une matrice de mise à l'échelle.
 * @param echelle Facteurs d'échelle sur chaque axe.
 * @return Matrice d'échelle correspondante.
 */
DUAL_Mat4 DUAL_Mat4_Scale(DUAL_Vec3 echelle);

/**
 * @brief Construit une matrice de rotation autour d'un axe arbitraire.
 * @param axe Axe de rotation (doit être normalisé).
 * @param angle_radians Angle de rotation, en radians.
 * @return Matrice de rotation correspondante.
 */
DUAL_Mat4 DUAL_Mat4_Rotate(DUAL_Vec3 axe, float angle_radians);

/**
 * @brief Construit une matrice de projection orthographique, typiquement
 *        utilisée pour le rendu 2D des sprites sur les écrans de la console.
 *
 * @param gauche Coordonnée du plan gauche.
 * @param droite Coordonnée du plan droit.
 * @param bas Coordonnée du plan bas.
 * @param haut Coordonnée du plan haut.
 * @param proche Distance du plan proche.
 * @param lointain Distance du plan lointain.
 * @return Matrice de projection orthographique.
 */
DUAL_Mat4 DUAL_Mat4_Ortho(float gauche, float droite, float bas, float haut,
                          float proche, float lointain);

/**
 * @brief Construit une matrice de projection en perspective, utilisée pour
 *        le rendu 3D.
 *
 * @param fov_radians Champ de vision vertical, en radians.
 * @param ratio_aspect Ratio largeur/hauteur de l'écran cible.
 * @param proche Distance du plan de clipping proche.
 * @param lointain Distance du plan de clipping lointain.
 * @return Matrice de projection en perspective.
 */
DUAL_Mat4 DUAL_Mat4_Perspective(float fov_radians, float ratio_aspect,
                                 float proche, float lointain);

/**
 * @brief Construit une matrice de vue de type "look-at", orientant la caméra
 *        d'une position vers une cible.
 *
 * @param position Position de la caméra dans l'espace monde.
 * @param cible Point regardé par la caméra.
 * @param haut Vecteur "haut" du monde (généralement {0,1,0}).
 * @return Matrice de vue correspondante.
 */
DUAL_Mat4 DUAL_Mat4_LookAt(DUAL_Vec3 position, DUAL_Vec3 cible, DUAL_Vec3 haut);

/* ============================================================================
 *  Tests de collision
 * ========================================================================== */

/**
 * @brief Teste l'intersection entre deux rectangles 2D.
 * @param a Premier rectangle.
 * @param b Second rectangle.
 * @return true si les rectangles se chevauchent, false sinon.
 */
bool DUAL_CollideRectRect(DUAL_Rect a, DUAL_Rect b);

/**
 * @brief Teste l'intersection entre deux cercles.
 * @param a Premier cercle.
 * @param b Second cercle.
 * @return true si les cercles se chevauchent, false sinon.
 */
bool DUAL_CollideCircleCircle(DUAL_Circle a, DUAL_Circle b);

/**
 * @brief Teste si un point 2D est contenu dans un rectangle.
 * @param point Point à tester.
 * @param rect Rectangle de référence.
 * @return true si le point est à l'intérieur du rectangle, false sinon.
 */
bool DUAL_CollidePointRect(DUAL_Vec2 point, DUAL_Rect rect);

/**
 * @brief Teste l'intersection entre deux boîtes englobantes 3D (AABB).
 * @param a Première boîte englobante.
 * @param b Seconde boîte englobante.
 * @return true si les boîtes se chevauchent, false sinon.
 */
bool DUAL_CollideAABBAABB(DUAL_AABB a, DUAL_AABB b);

/**
 * @brief Teste l'intersection entre deux sphères.
 * @param a Première sphère.
 * @param b Seconde sphère.
 * @return true si les sphères se chevauchent, false sinon.
 */
bool DUAL_CollideSphereSphere(DUAL_Sphere a, DUAL_Sphere b);

/* ============================================================================
 *  Interpolation
 * ========================================================================== */

/**
 * @brief Effectue une interpolation linéaire entre deux valeurs scalaires.
 * @param depart Valeur de départ (t = 0).
 * @param arrivee Valeur d'arrivée (t = 1).
 * @param t Facteur d'interpolation, généralement compris entre 0 et 1.
 * @return Valeur interpolée.
 */
float DUAL_Lerp(float depart, float arrivee, float t);

/**
 * @brief Effectue une interpolation linéaire entre deux vecteurs 2D.
 * @param depart Vecteur de départ (t = 0).
 * @param arrivee Vecteur d'arrivée (t = 1).
 * @param t Facteur d'interpolation, généralement compris entre 0 et 1.
 * @return Vecteur interpolé.
 */
DUAL_Vec2 DUAL_Vec2_Lerp(DUAL_Vec2 depart, DUAL_Vec2 arrivee, float t);

/**
 * @brief Effectue une interpolation linéaire entre deux vecteurs 3D.
 * @param depart Vecteur de départ (t = 0).
 * @param arrivee Vecteur d'arrivée (t = 1).
 * @param t Facteur d'interpolation, généralement compris entre 0 et 1.
 * @return Vecteur interpolé.
 */
DUAL_Vec3 DUAL_Vec3_Lerp(DUAL_Vec3 depart, DUAL_Vec3 arrivee, float t);

/**
 * @brief Applique une fonction d'atténuation "ease-in-out" (lissage cubique)
 *        à un facteur d'interpolation.
 *
 * Utile pour adoucir les animations de mouvement ou de transition d'écran.
 *
 * @param t Facteur d'interpolation linéaire, compris entre 0 et 1.
 * @return Facteur d'interpolation lissé, compris entre 0 et 1.
 */
float DUAL_EaseInOut(float t);

/**
 * @brief Limite (clamp) une valeur flottante entre un minimum et un maximum.
 * @param valeur Valeur à limiter.
 * @param min Limite inférieure.
 * @param max Limite supérieure.
 * @return Valeur limitée à l'intervalle [min, max].
 */
float DUAL_Clamp(float valeur, float min, float max);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_MATH_H */
