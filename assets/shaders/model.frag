#version 330 core

in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 albedo = vec3(1.0, 1.0, 1.0);
uniform vec3 lightDir = vec3(-0.8, -1.0, -0.6);

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(-lightDir);

    vec3 ambient = 0.1 * vec3(1.0);
    vec3 diffuse = max(dot(norm, lightDirection), 0.0) * vec3(1.0);

    vec3 color = (ambient + diffuse) * albedo;
    FragColor = vec4(color, 1.0f);
}
