//
// Created by killian on 3/15/26.
//

#include "../../include/DUAL_Graphics/shader.h"
#include "../../include/DUAL_Core/dual_core.h"
#include "../../include/DUAL_Graphics/dual_graphics_3d.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <glad/glad.h>

#include "DUAL_Core/dual_core.h"

#define BUFFER_SIZE 4096

char* loadAndFillBuffer(char* path) {
    int i = 0;
    char* buffer = malloc(sizeof(char) * BUFFER_SIZE);
    char ch;
    FILE *fptr = fopen(path, "r");
    if (!fptr) {
        printf("Unable to open file %s\n",path);
        return NULL;
    }
    while ((ch = fgetc(fptr)) != EOF && i < BUFFER_SIZE) {
        buffer[i] = ch;
        i++;
    }
    buffer[i] = '\0';
    fclose(fptr);
    return buffer;
}

bool DUAL_Shader_load_VS_FS(DUAL_Shader* shader, char* vs_path, char* fs_path ,DUAL_ShaderRender renderMode) {
    char* vShaderCode = loadAndFillBuffer(vs_path);
    char* fShaderCode = loadAndFillBuffer(fs_path);

    if (vShaderCode == NULL || fShaderCode == NULL) {
        DUAL_Log(DUAL_LOG_ERROR, "Unable to load shaders\n");
        if (vShaderCode) free(vShaderCode);
        if (fShaderCode) free(fShaderCode);
        return false;
    }

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const char* const*)&vShaderCode, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        DUAL_Log(DUAL_LOG_ERROR, "ERROR::SHADER::VERTEX::COMPILATION_FAILED : %s\n", infoLog);
    }

    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const char* const*)&fShaderCode, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        DUAL_Log(DUAL_LOG_ERROR, "ERROR::SHADER::VERTEX::COMPILATION_FAILED : %s\n", infoLog);
    }

    // link shaders
    shader->shaderID = glCreateProgram();
    glAttachShader(shader->shaderID, vertexShader);
    glAttachShader(shader->shaderID, fragmentShader);
    glLinkProgram(shader->shaderID);

    glGetProgramiv(shader->shaderID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader->shaderID, 512, NULL, infoLog);
        DUAL_Log(DUAL_LOG_ERROR, "ERROR::SHADER::PROGRAM::LINKING_FAILED : %s\n", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    free(vShaderCode);
    free(fShaderCode);

    shader->renderMode = renderMode;

    return true;
}

bool DUAL_Shader_load_VS_GEO_FS(DUAL_Shader* shader, char* vs_path, char* geo_path, char* fs_path ,DUAL_ShaderRender renderMode) {
    char* vShaderCode = loadAndFillBuffer(vs_path);
    char* gShaderCode = loadAndFillBuffer(geo_path);
    char* fShaderCode = loadAndFillBuffer(fs_path);

    if (vShaderCode == NULL || gShaderCode == NULL || fShaderCode == NULL) {
        DUAL_Log(DUAL_LOG_ERROR, "Unable to load shaders\n");
        if (vShaderCode) free(vShaderCode);
        if (gShaderCode) free(gShaderCode);
        if (fShaderCode) free(fShaderCode);
        return false;
    }

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const char* const*)&vShaderCode, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        DUAL_Log(DUAL_LOG_ERROR, "ERROR::SHADER::VERTEX::COMPILATION_FAILED : %s\n", infoLog);
    }

    // geometry shader
    unsigned int geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometryShader, 1, (const char* const*)&gShaderCode, NULL);
    glCompileShader(geometryShader);

    glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(geometryShader, 512, NULL, infoLog);
        DUAL_Log(DUAL_LOG_ERROR, "ERROR::SHADER::GEOMETRY::COMPILATION_FAILED : %s\n", infoLog);
    }

    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const char* const*)&fShaderCode, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        DUAL_Log(DUAL_LOG_ERROR, "ERROR::SHADER::VERTEX::COMPILATION_FAILED : %s\n", infoLog);
    }

    // link shaders
    shader->shaderID = glCreateProgram();
    glAttachShader(shader->shaderID, vertexShader);
    glAttachShader(shader->shaderID, geometryShader);
    glAttachShader(shader->shaderID, fragmentShader);
    glLinkProgram(shader->shaderID);

    glGetProgramiv(shader->shaderID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader->shaderID, 512, NULL, infoLog);
        DUAL_Log(DUAL_LOG_ERROR, "ERROR::SHADER::PROGRAM::LINKING_FAILED : %s\n", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(geometryShader);
    glDeleteShader(fragmentShader);
    free(vShaderCode);
    free(gShaderCode);
    free(fShaderCode);

    shader->renderMode = renderMode;

    return true;
}

void DUAL_Shader_use(DUAL_Shader* shader){
    glUseProgram(shader->shaderID);
}

void DUAL_Shader_clean(DUAL_Shader* shader){
    glDeleteProgram(shader->shaderID);
}

void DUAL_Shader_setBool(DUAL_Shader* shader, char* name, bool value){
    glUniform1i(glGetUniformLocation(shader->shaderID, name), value);
}

void DUAL_Shader_setInt(DUAL_Shader* shader, char* name, int value) {
    glUniform1i(glGetUniformLocation(shader->shaderID, name), value);
}

void DUAL_Shader_setFloat(DUAL_Shader* shader, char* name, float value) {
    glUniform1f(glGetUniformLocation(shader->shaderID, name), value);
}

void DUAL_Shader_setVec2(DUAL_Shader* shader, char* name, DUAL_Vec2 value) {
    glUniform2fv(glGetUniformLocation(shader->shaderID, name), 1, (float[2]){value.x, value.y});
}

void DUAL_Shader_setVec3(DUAL_Shader* shader, char* name, DUAL_Vec3 value) {
    glUniform3fv(glGetUniformLocation(shader->shaderID, name), 1, (float[3]){value.x, value.y, value.z});
}

void DUAL_Shader_setVec4(DUAL_Shader* shader, char* name, DUAL_Vec4 value) {
    glUniform4fv(glGetUniformLocation(shader->shaderID, name), 1, (float[4]){value.x, value.y, value.z, value.w});
}

void DUAL_Shader_setMat4(DUAL_Shader* shader, char* name, DUAL_Mat4 value) {
    glUniformMatrix4fv(glGetUniformLocation(shader->shaderID, name), 1, GL_FALSE, value.m);
}
void DUAL_Shader_setMat4Array(DUAL_Shader* shader, char* name, int count, const DUAL_Mat4* values) {
    glUniformMatrix4fv(glGetUniformLocation(shader->shaderID, name), count, GL_FALSE, (const float*)values);
}