#version 330 core

layout (location = 0) in vec3 aSpherePos;
layout (location = 1) in vec3 aSphereNormal;

layout (location = 2) in vec2 aPos;
layout (location = 3) in vec2 aVel;
layout (location = 4) in vec4 aProperties;
layout (location = 5) in vec4 aDebugging;

out vec2 Velocity;
out vec2 TexCoord;
out vec4 Properties;
out vec4 Debugging;
flat out int ID;

uniform float size;

// Camera
uniform mat4 view;
uniform mat4 projection;

void main() {
    Velocity = aVel;
    Properties = aProperties;
    Debugging = aDebugging;
    ID = gl_InstanceID;

    vec3 worldPos = vec3(aPos, 0.0) + aSpherePos * size;
    gl_Position = projection * view * vec4(worldPos, 1.0f);
}