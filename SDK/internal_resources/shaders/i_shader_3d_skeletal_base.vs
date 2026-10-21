#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aWeights;
layout (location = 4) in ivec4 aBoneIds;

#define MAX_BONES 100

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;
uniform mat4 uFinalBonesMatrices[MAX_BONES];

void main()
{
    vec4 totalPosition = vec4(0.0);
    vec3 totalNormal = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 4; i++)
    {
        int boneId = aBoneIds[i];
        float weight = aWeights[i];

        if (boneId < 0 || boneId >= MAX_BONES || weight <= 0.0)
            continue;

        vec4 localPosition = uFinalBonesMatrices[boneId] * vec4(aPos, 1.0);
        totalPosition += localPosition * weight;

        vec3 localNormal = mat3(uFinalBonesMatrices[boneId]) * aNormal;
        totalNormal += localNormal * weight;

        totalWeight += weight;
    }

    if (totalWeight > 0.0001) {
        totalPosition /= totalWeight;
    } else {
        totalPosition = vec4(aPos, 1.0);
        totalNormal = aNormal;
    }

    FragPos = vec3(uModel * totalPosition);
    Normal = mat3(transpose(inverse(uModel))) * totalNormal;
    TexCoord = aTexCoord;

    gl_Position = uProjection * uView * vec4(FragPos, 1.0);
}