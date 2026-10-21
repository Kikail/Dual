#include "../../include/DUAL_Graphics/camera2d.h"

void DUAL_Camera2D_Init(DUAL_Camera2D* camera, DUAL_Vec2 offset) {
    if (!camera) return;
    camera->position = (DUAL_Vec2){0.0f, 0.0f};
    camera->zoom = 1.0f;
    camera->rotation = 0.0f;
    camera->offset = offset;
}

DUAL_Mat4 DUAL_Camera2D_GetViewMatrix(DUAL_Camera2D* camera) {
    if (!camera) return DUAL_Mat4_Identity();

    DUAL_Mat4 view = DUAL_Mat4_Identity();
    view = DUAL_Mat4_Multiply(view, DUAL_Mat4_Translate((DUAL_Vec3){camera->offset.x, camera->offset.y, 0.0f}));
    view = DUAL_Mat4_Multiply(view, DUAL_Mat4_Scale((DUAL_Vec3){camera->zoom, camera->zoom, 1.0f}));
    view = DUAL_Mat4_Multiply(view, DUAL_Mat4_Rotate((DUAL_Vec3){0.0, 0.0, 1.0}, camera->rotation));
    view = DUAL_Mat4_Multiply(view, DUAL_Mat4_Translate((DUAL_Vec3){-camera->position.x, -camera->position.y, 0.0f}));

    return view;
}