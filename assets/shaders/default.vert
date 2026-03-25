#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aVel;
layout (location = 2) in vec4 aProperties;
layout (location = 3) in vec4 aDebugging;

uniform mat4 projection;
uniform float size;

out vec2 Velocity;
out vec2 TexCoord;
out vec4 Properties;
out vec4 Debugging;
flat out int ID;

const vec2 positions[4] = vec2[4](
    vec2(-1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2( 1.0, -1.0)
);

const vec2 uvs[4] = vec2[4](
    vec2(0.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0)
);

void main() {
    Velocity = aVel;
    TexCoord = uvs[gl_VertexID];
    Properties = aProperties;
    Debugging = aDebugging;
    ID = gl_InstanceID;

    gl_Position = projection * vec4(size * positions[gl_VertexID] + aPos, 0.0f, 1.0f);
}