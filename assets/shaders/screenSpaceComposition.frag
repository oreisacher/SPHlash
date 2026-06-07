#version 430 core

uniform sampler2D sceneTex;
uniform sampler2D particlesDepthTex;
uniform sampler2D normalTex;

uniform vec3 lightDir = vec3(-0.8, -1.0, -0.6);

uniform vec3 fluidColor;
uniform float transparency;

uniform float ambientStrength;
uniform float specularStrength;
uniform float fresnelStrength;

uniform mat4 view;
uniform mat4 projection;

in vec2 uv;
out vec4 FragColor;

void main() {
    vec4 normal = texture(normalTex, uv);

    // Draw scene if no particle data exists
    if (normal.a == 0.0) {
        FragColor = texture(sceneTex, uv);
        return;
    }

    vec3 N = normalize(normal.rgb * 2.0 - 1.0);

    // View direction in viewspace is alwys towards the camera in Z direction
    vec3 V = vec3(0.0, 0.0, 1.0);

    vec3 L = normalize(vec3(view * vec4(-lightDir, 0.0)));
    vec3 H = normalize(L + V);

    // Diffuse
    float diffuse = max(dot(N, L), 0.0);

    // Specular
    float spec = pow(max(dot(N, H), 0.0), 128.0);

    // Fresnel
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);

    // Fake Refraction — Offset UV into scene texture using normal
    vec2 refractOffset = N.xy * 0.05;
    vec3 refractColor = texture(sceneTex, clamp(uv + refractOffset, 0.001, 0.999)).rgb;

    // Mix resulting color
    vec3 color = mix(fluidColor, fluidColor * refractColor, transparency);
    color = color * (ambientStrength + diffuse);
    color += vec3(1.0) * spec * specularStrength;
    color += fluidColor * fresnel * fresnelStrength;

    FragColor = vec4(color, 1.0);
}