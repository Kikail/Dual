//
// Created by killian on 3/15/26.
//

#ifndef DUAL_SHADER_H
#define DUAL_SHADER_H

#include <stdbool.h>
#include "../DUAL_Math/dual_math.h"

typedef enum DUAL_ShaderRender {
    DUAL_SHADER_LIT       = 0,
    DUAL_SHADER_UNLIT     = 1,
} DUAL_ShaderRender;

typedef struct DUAL_Shader {
    unsigned int shaderID;
    DUAL_ShaderRender renderMode;
}DUAL_Shader;

char* loadAndFillBuffer(char* path);
bool DUAL_Shader_load_VS_FS(DUAL_Shader* shader, char* vs, char* fs ,DUAL_ShaderRender renderMode);
bool DUAL_Shader_load_VS_GEO_FS(DUAL_Shader* shader, char* vs, char* geo, char* fs ,DUAL_ShaderRender renderMode);
void DUAL_Shader_use(DUAL_Shader* shader);
void DUAL_Shader_clean(DUAL_Shader* shader);

void DUAL_Shader_setBool(DUAL_Shader* shader, char* name, bool value);
void DUAL_Shader_setInt(DUAL_Shader* shader, char* name, int value);
void DUAL_Shader_setFloat(DUAL_Shader* shader, char* name, float value);
void DUAL_Shader_setVec2(DUAL_Shader* shader, char* name, DUAL_Vec2 value);
void DUAL_Shader_setVec3(DUAL_Shader* shader, char* name, DUAL_Vec3 value);
void DUAL_Shader_setVec4(DUAL_Shader* shader, char* name, DUAL_Vec4 value);
void DUAL_Shader_setMat4(DUAL_Shader* shader, char* name, DUAL_Mat4 value);
void DUAL_Shader_setMat4Array(DUAL_Shader* shader, char* name, int count, const DUAL_Mat4* values);

#endif //DUAL_SHADER_H