#version 330 core

uniform sampler3D tex;
uniform float sliceZ;

in vec2 uv;
out vec4 fragColor;

void main() {
    float sdf = texture(tex, vec3(uv, sliceZ)).r;

    float t = sdf / 1000.0f;

    float c = smoothstep(950.0f, 1050.0f, sdf);

//    if (t < 0) {
//        fragColor = vec4(1.0, 0.0, 0.0, 1.0f);
//    } else if (t == 0) {
//        fragColor = vec4(1.0, 1.0, 1.0, 1.0f);
//    } else  {
//        fragColor = vec4(0.0, 1.0, 1.0, 1.0f);
//    }

    fragColor = vec4(t, c, 0.0f, 1.0f);
}