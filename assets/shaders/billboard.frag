#version 430 core

in vec3 Normal;
in vec2 TexCoord;
in vec4 ViewPos;
in mat4 Projection;
in float Radius;

out float FragColor;

void main() {
    vec2 uv = TexCoord * 2.0 - 1.0f;
    float r2 = dot(uv, uv);

    if (r2 > 1.0)
        discard;

    vec3 sphereNormal = vec3(uv, sqrt(1.0 - r2));
    vec3 sphereSurface = vec3(ViewPos) + sphereNormal * Radius;

    // Write correct sphere depth to depth buffer
    vec4 clipPos = Projection * vec4(sphereSurface, 1.0);
    gl_FragDepth = (clipPos.z / clipPos.w) * 0.5 + 0.5;

    FragColor = sphereSurface.z;
}
