#version 430 core

uniform sampler2D sceneDepthTex;
uniform sampler2D normalTex;
uniform sampler2D depthTex;
uniform sampler2D sceneTex;

uniform vec3 lightDir = vec3(-0.8, -1.0, -0.6);
uniform vec3 fluidColor;
uniform mat4 projection;

in vec2 uv;
out vec4 FragColor;

void main() {
    vec4 normal4 = texture(normalTex, uv);

    // reconstruct fluid NDC depth from view-space depth
    float fluidViewDepth = texture(depthTex, uv).r;
    vec4 clipPos = projection * vec4(0.0, 0.0, fluidViewDepth, 1.0);
    float fluidNDCDepth = (clipPos.z / clipPos.w) * 0.5 + 0.5;

    // sample scene depth
    float sceneDepth = texture(sceneDepthTex, uv).r;

    // fluid is behind scene geometry — show scene instead
    if (normal4.a == 0.0 || fluidNDCDepth > sceneDepth) {
        FragColor = texture(sceneTex, uv);
        return;
    }

    vec3 N = normalize(normal4.rgb * 2.0 - 1.0);
    vec3 V = vec3(0.0, 0.0, 1.0); // view direction in view space
    vec3 L = normalize(lightDir);
    vec3 H = normalize(L + V);

    // diffuse
    float diff = max(dot(N, L), 0.5);

    // specular
    float spec = pow(max(dot(N, H), 0.0), 128.0);

    // fresnel
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);

    // refraction — offset UV into scene texture using normal
    float depth = texture(depthTex, uv).r;
    vec2 refractOffset = N.xy * 0.05;
    vec3 refractColor = texture(sceneTex, uv + refractOffset).rgb;

    // combine
    vec3 color = mix(
        fluidColor * refractColor * diff,  // refracted fluid color
        vec3(1.0),                          // specular highlight
        spec
    );
    color += fluidColor * fresnel * 0.3;

    FragColor = vec4(color, 1.0);
}