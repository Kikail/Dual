#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 uVP;          // View * Projection matrix
uniform vec3 uCameraPos;   // Position monde de la caméra
uniform vec2 uSize;        // Taille du billboard (largeur, hauteur)

out vec2 TexCoord;

void main()
{
    vec3 Pos = gl_in[0].gl_Position.xyz;

    // Direct de la position vers la caméra
    vec3 toCamera = normalize(uCameraPos - Pos);
    vec3 up = vec3(0.0, 1.0, 0.0);

    // Orthogonalisation du vecteur Right
    vec3 right = normalize(cross(up, toCamera));

    // Recalcul du vrai Up orienté vers la caméra
    vec3 realUp = cross(toCamera, right);

    float halfWidth = uSize.x * 0.5;
    float halfHeight = uSize.y * 0.5;

    vec3 vertexPos = Pos - right * halfWidth - realUp * halfHeight;
    gl_Position = uVP * vec4(vertexPos, 1.0);
    TexCoord = vec2(0.0, 0.0);
    EmitVertex();

    vertexPos = Pos - right * halfWidth + realUp * halfHeight;
    gl_Position = uVP * vec4(vertexPos, 1.0);
    TexCoord = vec2(0.0, 1.0);
    EmitVertex();

    vertexPos = Pos + right * halfWidth - realUp * halfHeight;
    gl_Position = uVP * vec4(vertexPos, 1.0);
    TexCoord = vec2(1.0, 0.0);
    EmitVertex();

    vertexPos = Pos + right * halfWidth + realUp * halfHeight;
    gl_Position = uVP * vec4(vertexPos, 1.0);
    TexCoord = vec2(1.0, 1.0);
    EmitVertex();

    EndPrimitive();
}