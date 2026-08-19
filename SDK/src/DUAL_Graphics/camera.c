//
// Created by killian on 3/14/26.
//
#include "../../include/DUAL_Graphics/camera.h"

#include <math.h>

DUAL_Camera DUAL_Camera_Create(DUAL_Vec3 pos, DUAL_Vec3 up, DUAL_Vec3 front, float yaw, float pitch, float movementSpeed, float mouseSensitivity, float zoom) {
    DUAL_Camera newCamera;
    newCamera.position = pos;
    newCamera.up = up;
    newCamera.front = front;
    newCamera.yaw = yaw;
    newCamera.pitch = pitch;
    newCamera.movementSpeed = movementSpeed;
    newCamera.mouseSensitivity = mouseSensitivity;
    newCamera.zoom = zoom;
    newCamera.worldUp = up;
    DUAL_Camera_UpdateCameraVectors(&newCamera);
    return newCamera;
}
DUAL_Mat4 DUAL_Camera_GetViewMatrix(DUAL_Camera* cam){
    return DUAL_Mat4_LookAt(cam->position, DUAL_Vec3_Add(cam->position, cam->front), cam->up);
}
void DUAL_Camera_ProcessKeyboard(DUAL_Camera* cam, CameraMovement direction, float deltaTime){
    float velocity = cam->movementSpeed * deltaTime;
    if (direction == FORWARD)
        cam->position = DUAL_Vec3_Add(cam->position, DUAL_Vec3_Scale(cam->front, velocity));
    if (direction == BACKWARD)
        cam->position = DUAL_Vec3_Sub(cam->position, DUAL_Vec3_Scale(cam->front, velocity));
    if (direction == LEFT)
        cam->position =  DUAL_Vec3_Sub(cam->position, DUAL_Vec3_Scale(cam->right, velocity));
    if (direction == RIGHT)
        cam->position = DUAL_Vec3_Add(cam->position, DUAL_Vec3_Scale(cam->right, velocity));
}
void DUAL_Camera_ProcessMouseMovement(DUAL_Camera* cam, float xoffset, float yoffset){
    xoffset *= cam->mouseSensitivity;
    yoffset *= cam->mouseSensitivity;

    cam->yaw   += xoffset;
    cam->pitch += yoffset;

    // update Front, Right and Up Vectors using the updated Euler angles
    DUAL_Camera_UpdateCameraVectors(cam);
}
void DUAL_Camera_ProcessMouseScroll(DUAL_Camera* cam, float yoffset){
    cam->zoom -= (float)yoffset;
    if (cam->zoom < 1.0f)
        cam->zoom = 1.0f;
    if (cam->zoom > 45.0f)
        cam->zoom = 45.0f;
}
void DUAL_Camera_LookAt(DUAL_Camera* cam, DUAL_Vec3 target) {
    DUAL_Vec3 direction = DUAL_Vec3_Normalize(DUAL_Vec3_Sub(target, cam->position));
    cam->pitch = asinf(direction.y) * (180.0f / 3.14159265358979323846f);
    cam->yaw = atan2f(direction.z, direction.x) * (180.0f / 3.14159265358979323846f);
    DUAL_Camera_UpdateCameraVectors(cam);
}
void DUAL_Camera_UpdateCameraVectors(DUAL_Camera* cam) {
    DUAL_Vec3 front;
    float yawRad = DUAL_RAD(cam->yaw);
    float pitchRad = DUAL_RAD(cam->pitch);

    front.x = cos(yawRad) * cosf(pitchRad);
    front.y = sinf(pitchRad);
    front.z = sinf(yawRad) * cosf(pitchRad);

    cam->front = DUAL_Vec3_Normalize(front);
    cam->right = DUAL_Vec3_Normalize(DUAL_Vec3_Cross(cam->front, cam->worldUp));
    cam->up    = DUAL_Vec3_Normalize(DUAL_Vec3_Cross(cam->right, cam->front));
}