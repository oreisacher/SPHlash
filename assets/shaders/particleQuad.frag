#version 430 core

in vec3 Normal;
in vec2 TexCoord;
in vec4 ViewPos;
in mat4 Projection;
in float Size;

out float FragDepth;

void main() {
    vec2 uv = TexCoord * 2.0 - 1.0f;
    float r2 = dot(uv, uv);

    // Depth defaults to zero
    if (r2 > 1.0)
        discard;

    // Construct normals of imaginary sphere
    vec3 sphereNormal = vec3(uv, sqrt(1.0 - r2));

    // Determine position of surface of imaginary sphere
    vec3 sphereSurface = vec3(ViewPos) + sphereNormal * Size;

    // Project surface position into clip space
    vec4 clipPos = Projection * vec4(sphereSurface, 1.0);

    // Map from NDC to [0, 1] range
    gl_FragDepth = (clipPos.z / clipPos.w) * 0.5 + 0.5;

    // Write linear depth into texture for future passes
    FragDepth = sphereSurface.z;
}
