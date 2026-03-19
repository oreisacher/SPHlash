#version 430

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 1) readonly buffer HashDataBuffer {
    uvec2 hashData[];
};

layout(std430, binding = 2) writeonly buffer HistogramBuffer {
    uint counts[];
};

shared uint localHistogram[16];

uniform int numParticles;
uniform int shift;

void main() {
    uint gID = gl_GlobalInvocationID.x;
    uint lID = gl_LocalInvocationID.x;

    // Init histograms
    // [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    if (lID < 16) {
        localHistogram[lID] = 0;
    }
    barrier();

    // Local in-block counting
    //  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    // [0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0]
    // ...
    // [0,  1,  2,  3,  1,  1,  2,  0,  0,  1,  0,  2,  0,  0,  3,  0]
    if (gID < numParticles) {
        uint val = (hashData[gID].x >> shift) & 0xFu; // Extract 4 bits
        atomicAdd(localHistogram[val], 1u);
    }
    barrier();

    // Write to global buffer
    // [0,  1,  2,  3,  1,  1,  2,  0,  0,  1,  0,  2,  0,  0,  3,  0] (Group 1)
    // [0,  1,  2,  3,  1,  1,  2,  0,  0,  1,  0,  2,  0,  0,  3,  0] (Group 2)
    // [0,  1,  2,  3,  1,  1,  2,  0,  0,  1,  0,  2,  0,  0,  3,  0] (Group 3)
    // ... Continous array
    if (lID < 16) {
        uint groupIdx = gl_WorkGroupID.x;
        counts[groupIdx * 16 + lID] = localHistogram[lID];
    }
}