#version 330 core

in vec2 TexCoord;
in vec4 VertexColor;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform bool uUseTexture;

void main() {
   if (uUseTexture) {
       FragColor = texture(uTexture, TexCoord) * VertexColor;
   } else {
       if (TexCoord.x < -1.5) {
           FragColor = VertexColor;
           return;
       }
       float distance = length(TexCoord);

       float epaisseur = 0.05;
       float lissage = 0.01;


       float alpha_ext = 1.0 - smoothstep(1.0 - lissage, 1.0, distance);
       float alpha_int = smoothstep(1.0 - epaisseur - lissage, 1.0 - epaisseur, distance);

       float final_alpha = alpha_ext * alpha_int;

       if (final_alpha <= 0.0) discard;
       FragColor = vec4(VertexColor.rgb, VertexColor.a * final_alpha);
   }
}