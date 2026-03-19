#version 430

layout(local_size_x = 512, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 2) readonly buffer HistogramBuffer {
    uint counts[];
};

layout(std430, binding = 3) writeonly buffer OffsetBuffer {
    uint offsets[];
};

shared uint temp[1024];

uniform int numWorkgroups;

void main() {
    uint lID = gl_LocalInvocationID.x;
    uint bin = gl_WorkGroupID.y;
    uint n = 1024;

    uint idxA = (2 * lID) * 16 + bin;
    uint idxB = (2 * lID + 1) * 16 + bin;

    temp[2 * lID    ] = (2 * lID     < numWorkgroups) ? counts[idxA] : 0;
    temp[2 * lID + 1] = (2 * lID + 1 < numWorkgroups) ? counts[idxB] : 0;
}
