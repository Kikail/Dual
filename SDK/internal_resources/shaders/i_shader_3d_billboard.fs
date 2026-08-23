#version 330 core

uniform sampler2D texture_diffuse;

in vec2 TexCoord;
out vec4 FragColor;

void main()
{
    vec4 texColor = texture(texture_diffuse, TexCoord);

    if (texColor.a < 0.1) {
        discard;
    }

    FragColor = texColor;
}