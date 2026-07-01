//
// Created by killian on 7/1/26.
//

#ifndef DUAL_DUAL_INTERNAL_H
#define DUAL_DUAL_INTERNAL_H

// Fonctions internes pour permettre aux autres modules d'interroger la fenêtre principale
void* DUAL_Internal_GetGLFWWindow(const DUAL_App* app);
void  DUAL_Internal_GetScreenDimensions(const DUAL_App* app, int32_t* out_w, int32_t* out_h);

#endif //DUAL_DUAL_INTERNAL_H
