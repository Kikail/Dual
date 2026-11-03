//
// Created by killian on 11/3/26.
//

#ifndef DUAL_DUAL_FONT_H
#define DUAL_DUAL_FONT_H
#include "dual_graphics_2d.h"

typedef struct DUAL_Font{
    DUAL_Texture* texture_atlas;
    float taille_pixels;
    stbtt_bakedchar données_caractères[96];

    char font_name[32];
    char font_path[128];
}DUAL_Font;

/**
 * @brief Charge une police depuis un fichier TTF pour le rendu de texte à l'écran.
 *
 * @param resources Gestionnaire de ressources utilisé pour suivre la consommation VRAM/RAM.
 * @param chemin_fichier Chemin du fichier de police (.ttf) à charger.
 * @param taille_pixels Taille de rendu de la police, en pixels.
 * @param out_font Pointeur recevant la police chargée en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_Font_LoadFromFile(DUAL_ResourceManager* resources,const char* chemin_fichier,int32_t taille_pixels,DUAL_Font** out_font);

/**
 * @brief Libère une police chargée et la mémoire associée.
 *
 * @param resources Gestionnaire de ressources ayant suivi l'allocation.
 * @param font Police à libérer.
 */
void DUAL_Font_Destroy(void* font);

/**
 * @brief Dessine une chaîne de texte à une position donnée, avec une police
 *        et une couleur spécifiées.
 *
 * @param renderer Contexte de rendu 2D, doit être entre Begin() et End().
 * @param font Police à utiliser pour le rendu.
 * @param texte Chaîne de caractères à afficher (encodage UTF-8).
 * @param position Position du coin supérieur gauche du texte, en coordonnées monde.
 * @param couleur Couleur du texte.
 */
void DUAL_DrawText(DUAL_Renderer2D* renderer, const DUAL_Font* font,const char* texte, DUAL_Vec2 position, DUAL_Color couleur);

/**
 * @brief Calcule la taille (largeur, hauteur) qu'occuperait une chaîne de texte
 *        si elle était dessinée avec une police donnée, sans la dessiner.
 *
 * Utile pour centrer ou aligner du texte avant de l'afficher.
 *
 * @param font Police à utiliser pour le calcul.
 * @param texte Chaîne de caractères à mesurer (encodage UTF-8).
 * @param out_largeur Pointeur recevant la largeur calculée, en pixels.
 * @param out_hauteur Pointeur recevant la hauteur calculée, en pixels.
 */
void DUAL_MeasureText(const DUAL_Font* font, const char* texte,float* out_largeur, float* out_hauteur);


#endif //DUAL_DUAL_FONT_H
