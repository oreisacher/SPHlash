#version 330 core

uniform sampler2D smoothDepth;
uniform vec2 texelSize;
uniform mat4 projection;

in vec2 uv;
out vec4 FragColor;

vec3 uvToViewPos(vec2 texCoord, float depth) {
    float x = (texCoord.x * 2.0 - 1.0);
    float y = (texCoord.y * 2.0 - 1.0);
    // unproject xy using projection matrix
    float fx = projection[0][0]; // focal length x
    float fy = projection[1][1]; // focal length y
    return vec3((x / fx) * (-depth), (y / fy) * (-depth), depth);
}

void main() {
    float depth  = texture(smoothDepth, uv).r;
    if (depth == 0.0) { FragColor = vec4(0.0); return; }

    float depthR = texture(smoothDepth, uv + vec2(texelSize.x, 0.0)).r;
    float depthU = texture(smoothDepth, uv + vec2(0.0, texelSize.y)).r;

    // skip edge pixels where neighbour is background
    if (depthR == 0.0) depthR = depth;
    if (depthU == 0.0) depthU = depth;

    vec3 p  = uvToViewPos(uv, depth);
    vec3 px = uvToViewPos(uv + vec2(texelSize.x, 0.0), depthR);
    vec3 py = uvToViewPos(uv + vec2(0.0, texelSize.y), depthU);

    vec3 normal = normalize(cross(px - p, py - p));

    FragColor = vec4(normal * 0.5 + 0.5, 1.0);
}