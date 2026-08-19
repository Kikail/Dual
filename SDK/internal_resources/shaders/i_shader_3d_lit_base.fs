#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

#define MAX_LIGHTS 4

struct Light {
    int type;
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float range;
};

uniform Light uLights[MAX_LIGHTS];
uniform vec3 uAmbientLight;
uniform vec3 uViewPos;
uniform sampler2D texture_diffuse;

void main() {
    vec4 texColor = texture(texture_diffuse, TexCoord);
    vec3 ambient = uAmbientLight * texColor.rgb;
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(uViewPos - FragPos);

    vec3 diffuseAccum = vec3(0.0);

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (uLights[i].intensity <= 0.0) continue;

        vec3 lightDir;
        float attenuation = 1.0;

        if (uLights[i].type == 0) {
            lightDir = normalize(-uLights[i].direction);
        } else {
            vec3 lightVec = uLights[i].position - FragPos;
            float distance = length(lightVec);
            lightDir = normalize(lightVec);

            if (uLights[i].range > 0.0) {
                if (distance >= uLights[i].range) {
                    attenuation = 0.0;
                } else {
                    float ratio = distance / uLights[i].range;
                    float baseAttenuation = 1.0 / (1.0 + 2.0 * ratio + ratio * ratio);
                    float edgeFade = smoothstep(1.0, 0.7, ratio);
                    attenuation = baseAttenuation * edgeFade;
                }
            }
        }

        float diff = max(dot(norm, lightDir), 0.0);
        diffuseAccum += uLights[i].color * uLights[i].intensity * diff * texColor.rgb * attenuation;
    }

    vec3 result = ambient + diffuseAccum;
    FragColor = vec4(result, texColor.a);
}