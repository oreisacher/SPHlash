#version 430 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

layout (location = 3) in vec4 aPos;
layout (location = 4) in vec4 aVel;
layout (location = 5) in vec4 aProperties;
layout (location = 6) in vec4 aFlags;
layout (location = 7) in vec4 aColor;

// Camera
uniform mat4 view;
uniform mat4 projection;

uniform float size = 0.015f;

out vec3 Normal;
out vec2 TexCoord;
out vec4 ViewPos;
out mat4 Projection;
out float Size;

void main() {
    vec4 centerView = view * vec4(aPos.xyz, 1.0);

    // Pass down data to frag
    TexCoord = aTexCoord;
    Size = size;
    Projection = projection;
    ViewPos = centerView;

    // Span camera facing quad with given size
    centerView.xy += aPosition.xy * size;
    gl_Position = projection * centerView;
}
