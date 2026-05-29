#version 330 core

layout (location = 0) in vec3 aSpherePos;
layout (location = 1) in vec3 aSphereNormal;

layout (location = 2) in vec4 aPos;
layout (location = 3) in vec3 aVel;
layout (location = 4) in vec4 aProperties;
layout (location = 5) in vec4 aFlags;

flat out vec3 Velocity;
flat out vec4 Properties;
flat out vec4 Flags;

uniform float size;

// Camera
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    Velocity = aVel;
    Properties = aProperties;
    Flags = aFlags;

    vec3 worldPos = aPos.xyz + aSpherePos * size;
    gl_Position = projection * view * vec4(worldPos, 1.0f);
}