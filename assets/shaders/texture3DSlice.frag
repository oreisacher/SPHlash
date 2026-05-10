#version 330 core

uniform sampler3D tex;
uniform float sliceZ;

in vec2 uv;
out vec4 fragColor;

void main() {
    float sdf = texture(tex, vec3(uv, sliceZ)).r;

    float c = step(0.0, sdf); // 0 if inside, 1 if outside
    fragColor = vec4(vec3(c), 1.0);
}