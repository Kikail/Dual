//
// Created by killian on 3/14/26.
//

#ifndef DUAL_CAMERA_H
#define DUAL_CAMERA_H

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

typedef struct DUAL_Camera {
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
}DUAL_Camera;

DUAL_Camera DUAL_Camera_Create(DUAL_Vec3 pos, DUAL_Vec3 up, DUAL_Vec3 front, float yaw, float pitch, float movementSpeed, float mouseSensitivity, float zoom);
DUAL_Mat4 DUAL_Camera_GetViewMatrix(DUAL_Camera* cam);
void DUAL_Camera_ProcessKeyboard(DUAL_Camera* cam ,CameraMovement direction ,float deltaTime);
void DUAL_Camera_ProcessMouseMovement(DUAL_Camera* cam, float xoffset, float yoffset);
void DUAL_Camera_ProcessMouseScroll(DUAL_Camera* cam, float yoffset);
void DUAL_Camera_UpdateCameraVectors(DUAL_Camera* cam);
void DUAL_Camera_LookAt(DUAL_Camera* cam, DUAL_Vec3 target);

#endif //DUAL_CAMERA_H