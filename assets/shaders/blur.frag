#version 430 core

uniform sampler2D depthTex;
uniform vec2 direction;
uniform vec2 texelSize;

in vec2 uv;

out float FragDepth;

const int KERNEL_RADIUS = 15;
const float DEPTH_THRESHOLD = 0.05f;

void main() {
    float centerDepth = texture(depthTex, uv).r;

    // skip background pixels
    if (centerDepth == 0.0) {
        return;
    }

    float weightSum = 0.0;
    float depthSum = 0.0;

    for (int i = -KERNEL_RADIUS; i <= KERNEL_RADIUS; i++) {
        vec2 offset = direction * texelSize * float(i);
        float sampleDepth = texture(depthTex, uv + offset).r;

        // skip background samples
        if (sampleDepth == 0.0) continue;

        // spatial weight (Gaussian)
        float spatialWeight = exp(-float(i * i) / (2.0 * float(KERNEL_RADIUS)));

        // depth weight (bilateral — reduces blur across surface edges)
        float depthDiff = abs(sampleDepth - centerDepth);
        float depthWeight = exp(-depthDiff / DEPTH_THRESHOLD);

        float weight = spatialWeight * depthWeight;
        depthSum += sampleDepth * weight;
        weightSum += weight;
    }

    FragDepth = depthSum / weightSum;
}