#version 330 core

layout (location = 0) in vec3 aPos;   // Offset 0 (offsetof(LineVertex, position))
layout (location = 1) in vec3 aColor; // Offset 12 (offsetof(LineVertex, color))

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vColor;

void main() {
    vColor = aColor;
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}