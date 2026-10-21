#version 330 core

layout (location = 0) in vec3 Position;
uniform vec3 uPosition;

void main()
{
    gl_Position = vec4(Position + uPosition, 1.0);
}