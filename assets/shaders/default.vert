#version 330 core

layout (location = 0) in vec3 aSpherePos;
layout (location = 1) in vec3 aSphereNormal;

layout (location = 2) in vec3 aPos;
layout (location = 3) in vec3 aVel;
layout (location = 4) in vec4 aProperties;
layout (location = 5) in vec4 aFlags;
layout (location = 6) in vec4 aDebugging;

out vec3 Velocity;
out vec2 TexCoord;
out vec4 Properties;
out vec4 Flags;
out vec4 Debugging;
flat out int ID;

uniform float size;

// Camera
uniform mat4 view;
uniform mat4 projection;

void main() {
    Velocity = aVel;
    Properties = aProperties;
    Flags = aFlags;
    Debugging = aDebugging;
    ID = gl_InstanceID;

    vec3 worldPos = aPos + aSpherePos * size;
    gl_Position = projection * view * vec4(worldPos, 1.0f);
}