//
// Created by killian on 3/14/26.
//

#ifndef DUAL_CAMERA3D_H
#define DUAL_CAMERA3D_H

#include "../DUAL_Math/dual_math.h"

#define DUAL_VECTOR_UP (DUAL_Vec3){0.0,1.0,0.0}
#define DUAL_VECTOR_DOWN (DUAL_Vec3){0.0,-1.0,0.0}
#define DUAL_VECTOR_FRONT (DUAL_Vec3){0.0,0.0,-1.0}
#define DUAL_VECTOR_BACK (DUAL_Vec3){0.0,0.0,1.0}

#define DUAL_CAMERA_YAW (90.0)
#define DUAL_CAMERA_PITCH (0.0f)
#define DUAL_CAMERA_SPEED (10.0)
#define DUAL_CAMERA_SENSIVITY (0.1f)
#define DUAL_CAMERA_ZOOM (45.0)

typedef enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
}CameraMovement;

typedef struct DUAL_Camera3D {
    DUAL_Vec3 position;
    DUAL_Vec3 front;
    DUAL_Vec3 up;
    DUAL_Vec3 right;
    DUAL_Vec3 worldUp;
    float yaw;
    float pitch;
    float movementSpeed;
    float mouseSensitivity;
    float zoom;
}DUAL_Camera3D;

DUAL_Camera3D DUAL_Camera3D_Create(DUAL_Vec3 pos, DUAL_Vec3 up, DUAL_Vec3 front, float yaw, float pitch, float movementSpeed, float mouseSensitivity, float zoom);
DUAL_Mat4 DUAL_Camera3D_GetViewMatrix(DUAL_Camera3D* cam);
void DUAL_Camera3D_ProcessKeyboard(DUAL_Camera3D* cam ,CameraMovement direction ,float deltaTime);
void DUAL_Camera3D_ProcessMouseMovement(DUAL_Camera3D* cam, float xoffset, float yoffset);
void DUAL_Camera3D_ProcessMouseScroll(DUAL_Camera3D* cam, float yoffset);
void DUAL_Camera3D_UpdateCameraVectors(DUAL_Camera3D* cam);
void DUAL_Camera3D_LookAt(DUAL_Camera3D* cam, DUAL_Vec3 target);

#endif //DUAL_CAMERA3D_H