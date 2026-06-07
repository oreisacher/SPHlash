#version 430 core

uniform sampler2D depthTex;
uniform vec2 direction;
uniform vec2 texelSize;

uniform int kernel = 7;
uniform float depthThreshold = 0.05f;

in vec2 uv;

out float FragDepth;

void main() {
    float centerDepth = texture(depthTex, uv).r;

    // Skip background pixels > Defaults to zero
    if (centerDepth == 0.0)
        discard;

    float weightSum = 0.0;
    float depthSum = 0.0;

    for (int i = -kernel; i <= kernel; i++) {
        vec2 offset = direction * texelSize * float(i);
        float sampleDepth = texture(depthTex, uv + offset).r;

        // Skip background
        if (sampleDepth == 0.0)
            continue;

        // Spatial weight (Gaussian)
        float spatialWeight = exp(-float(i * i) / (2.0 * kernel * kernel));

        // Depth weight (bilateral — reduces blur across surface edges)
        float depthDiff = abs(sampleDepth - centerDepth);
        float depthWeight = exp(-depthDiff / depthThreshold);

        float weight = spatialWeight * depthWeight;
        depthSum += sampleDepth * weight;
        weightSum += weight;
    }

    if (weightSum < 1e-4)
        FragDepth = centerDepth;
    else
        FragDepth = depthSum / weightSum;
}