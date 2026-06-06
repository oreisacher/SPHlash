#version 330 core

layout (location = 0) in vec3 aSpherePos;
layout (location = 1) in vec3 aSphereNormal;

layout (location = 3) in vec4 aPos;
layout (location = 4) in vec3 aVel;
layout (location = 5) in vec4 aProperties;
layout (location = 6) in vec4 aFlags;
layout (location = 7) in vec4 aColor;

flat out vec3 Velocity;
flat out vec4 Properties;
flat out vec4 Flags;
flat out vec4 Color;

uniform float size;

// Camera
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    Velocity = aVel;
    Properties = aProperties;
    Flags = aFlags;
    Color = aColor;

    vec3 worldPos = aPos.xyz + aSpherePos * size;
    gl_Position = projection * view * vec4(worldPos, 1.0f);
}