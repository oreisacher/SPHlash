#version 330 core

uniform sampler3D tex;
uniform float sliceZ;

in vec2 uv;
out vec4 fragColor;

void main() {
    float value = texture(tex, vec3(uv, sliceZ)).r;
    fragColor = vec4(value, 0.0, 0.0, 1.0);
}