# Dual Engine

Framework de rendu 2D/3D fait en C avec OpenGL.
> **Projet en développement** : fuites mémoire à corriger, code à réorganiser, documentation incomplète.

Examinez les modules principaux ainsi que les exemples (classés du plus simple au plus avancé) :
1. [Tampon de Texte (Text Buffer)](#1-tampon-de-texte-text-buffer)
2. [Jeu Flappy Bird](#2-jeu-flappy-bird)
3. [Système de Particules](#3-système-de-particules)
4. [Moteur Physique](#4-moteur-physique)
5. [Chargement de Scène 3D](#5-chargement-de-scène-3d)

Dans ce projet, je cherche à montrer comment fonctionnent les moteurs graphiques personnalisés et les systèmes de jeu, en concevant le tout à partir de zéro à l’aide de C, GLib et GLFW.

---

## Architecture et Modules (Du plus simple au plus complexe)

### 1. Tampon de Texte (`Text Buffer`) — *Le plus simple*
![Text Buffer](https://github.com/user-attachments/assets/d5022dab-c3e0-489d-8887-0f1c2e788c1e)

* **Ce que c'est :** Une interface textuelle interactive de base permettant de taper du texte au clavier et de l'afficher dynamiquement à l'écran.
* **Comment ça fonctionne :**
    * Le module gère un tableau de caractères alloué de manière fixe (`DUAL_TextInputBuffer`) ainsi qu'un curseur de position.
    * Des rappels (callbacks) GLFW (`glfwSetCharCallback` et `glfwSetKeyCallback`) écoutent les pressions de touches en temps réel.
    * Lorsqu'une touche alphabétique ou numérique est pressée, la fonction `text_input_insert_char` décale les caractères dans le tableau et met à jour l'affichage, tandis que la touche `BACKSPACE` gère la suppression

---

### 2. Jeu Flappy Bird — *Complexité Légère (Logique 2D)*
![Flappy Bird](https://github.com/user-attachments/assets/d966fe86-3068-4041-a12a-a4a5378c6c94)

* **Ce que c'est :** Un mini-jeu complet en 2D intégré pour tester la boucle de jeu, la gestion du temps (`Delta Time`) et les interactions de base.
* **Comment ça fonctionne :**
    * Repose sur une boucle de mise à jour par frame (`DUAL_BeginFrame` / `DUAL_EndFrame`).
    * Gère une physique simplifiée de gravité (le personnage subit une accélération verticale constante vers le bas, inversée par une impulsion lors d'un clic ou d'une touche).
    * Utilise des tests de collision 2D basiques (Bounding Boxes rectangulaires) entre l'oiseau et les obstacles défilants pour détecter la fin de partie (Game Over).

---

### 3. Système de Particules — *Complexité Moyenne (Effets visuels)*
![Particle System](https://github.com/user-attachments/assets/c08b4ee5-04a4-4494-bf3a-cb5a2853ccc3)

* **Ce que c'est :** Un générateur d'effets visuels dynamiques en temps réel (fumée, étincelles, explosions).
* **Comment ça fonctionne :**
    * Gère un tableau alloué de structures de particules, contenant chacune une position, une vitesse, une couleur, une durée de vie (`lifetime`) et une opacité.
    * À chaque frame, le moteur parcourt les particules actives, met à jour leurs coordonnées en fonction du vecteur de vélocité et du `Delta Time`, et réduit leur durée de vie.
    * Utilise le rendu par **billboards** (via des shaders de géométrie OpenGL) pour que les textures des particules pivotent toujours face à la caméra 3D.

---

### 4. Moteur Physique — *Complexité Avancée*
![Physics](https://github.com/user-attachments/assets/b8b3a56c-9a3b-4f55-b421-e93df9601305)

* **Ce que c'est :** Un monde physique minimaliste gérant la gravité, les corps rigides (`DUAL_RigidBody`) et la résolution de collisions par boîtes englobantes (`AABB`).
* **Comment ça fonctionne :**
    * **Intégration numérique :** À chaque pas de simulation (`DUAL_PhysicsWorld_Step`), les forces (comme la gravité) modifient l'accélération, qui met à jour la vitesse, qui elle-même déplace la position des objets.
    * **Collisions AABB :** Le moteur teste les intersections géométriques sur les trois axes ($X, Y, Z$) entre des objets statiques (décors) et dynamiques (cubes en chute).
    * **Rebonds et amortissement :** Lorsqu'une collision est détectée, la vitesse sur l'axe concerné est inversée et multipliée par un coefficient d'amortissement (`amortissement = 0.98f`) pour simuler la perte d'énergie cinétique.

---

### 5. Chargement de Scène 3D — *Le plus complexe*
![Scene 3D](https://github.com/user-attachments/assets/a6d6de71-2b78-4856-875d-3146a2df1a6e)

* **Ce que c'est :** Un système complet de sérialisation et de rendu 3D capable d'analyser un fichier de scène personnalisé (`.dual`) pour instancier des décors, des lumières, des collisions et des modèles 3D complexes texturés ou animés.
* **Comment ça fonctionne :**
    * **Lecture binaire/flux :** Le fichier `.dual` est lu octet par octet pour extraire un bloc d'entités (`DUAL_Editor_SceneEntity`).
    * **Gestion des ressources (GLib) :** Les chemins de fichiers sont résolus et les modèles 3D (via Assimp) sont chargés en mémoire sans doublon grâce à une table de hachage qui isole les noms de fichiers sans extension (`DUAL_GetFilenameWithoutExtension`).
    * **Pipeline de Rendu 3D :** Le moteur applique les matrices de projection, de vue (caméra subjective FPS) et de transformation, puis envoie les informations d'éclairage aux shaders OpenGL pour afficher le monde en relief avec gestion des matériaux et des lumières directionnelles/ponctuelles.

---

## Prise en main rapide

Le code final s'articule autour d'une architecture modulaire. Pour ouvrir une application avec le moins de lignes possible, voici l'exemple minimaliste d'initialisation :

```c
#include "../../SDK/include/DUAL_Core/dual_core.h"
#include "../../SDK/include/DUAL_Graphics/dual_graphics_3d.h"

int main() {
    DUAL_App* app = NULL;
    DUAL_AppConfig config = {
        .largeur_ecran = 1920,
        .hauteur_ecran = 1080,
        .plein_ecran = false,
        .fps_cible = 60,
        .titre_fenetre = "DUAL_Window",
        .vsync_actif = false,
        .screenLayout = DUAL_LAYOUT_NO_SPLIT
    };
    
    DUAL_Init(&config, &app);

    DUAL_Renderer3D* renderer3D = NULL;
    DUAL_Renderer3D_Create(app, &renderer3D);

    while (DUAL_ShouldRun(app)) {
        DUAL_BeginFrame(app);

        DUAL_Renderer3D_Begin(renderer3D);
        // Dessin et logique de l'application ici...
        DUAL_Renderer3D_End(renderer3D);

        DUAL_EndFrame(app);
    }

    return 0;
}
```
Ce code ouvrira la fenetre suivante :
![Fenetre](https://github.com/user-attachments/assets/b2934e60-7d6c-46a7-a718-994dd7237475)
