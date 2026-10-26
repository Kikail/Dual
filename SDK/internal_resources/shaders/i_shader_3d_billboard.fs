#version 330 core

uniform sampler2D texture_diffuse;
uniform vec4 uColor;

in vec2 TexCoord;
out vec4 FragColor;

void main()
{
    vec4 texColor = texture(texture_diffuse, TexCoord);

    float brightness = dot(texColor.rgb, vec3(0.299, 0.587, 0.114));
    if (brightness < 0.1) {
         discard;
    }

    if (texColor.a < 0.1) {
        discard;
    }

    vec3 finalColor = texColor.rgb * uColor.rgb;

    float finalAlpha = texColor.a * uColor.w;

    FragColor = vec4(finalColor, finalAlpha);
}