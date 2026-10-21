#ifndef DUAL_CAMERA_2D_H
#define DUAL_CAMERA_2D_H

#include "../DUAL_Math/dual_math.h"

typedef struct DUAL_Camera2D{
    DUAL_Vec2 position;
    float     zoom;
    float     rotation;
    DUAL_Vec2 offset;
} DUAL_Camera2D;

#ifdef __cplusplus
extern "C" {
#endif

void DUAL_Camera2D_Init(DUAL_Camera2D* camera, DUAL_Vec2 offset);
DUAL_Mat4 DUAL_Camera2D_GetViewMatrix(DUAL_Camera2D* camera);

#ifdef __cplusplus
}
#endif

#endif // DUAL_CAMERA_2D_H