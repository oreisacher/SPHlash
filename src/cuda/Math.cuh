#include <vector_functions.h>
#include <vector_types.h>

__device__ inline float4 operator+(const float4& a, const float4& b) {
    return make_float4(
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
        a.w + b.w
    );
}

__device__ inline float4 operator-(const float4& a, const float4& b) {
    return make_float4(
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
        0.0f
    );
}

__device__ inline float4& operator+=(float4& a, const float4& b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
    return a;
}

__device__ inline float4 operator/(const float4& v, float s) {
    return make_float4(
        v.x / s,
        v.y / s,
        v.z / s,
        0.0f
    );
}

__device__ inline float4 operator*(float s, const float4& v) {
    return make_float4(
        v.x * s,
        v.y * s,
        v.z * s,
        0.0f
    );
}

__device__ inline float4 operator*(const float4& v, float s) {
    return make_float4(
        v.x * s,
        v.y * s,
        v.z * s,
        0.0f
    );
}

__device__ inline uint3 operator/(const uint3& v, const uint s) {
    return make_uint3(
        v.x / s,
        v.y / s,
        v.z / s
    );
}

__device__ inline int3 operator+(const int3& v, const int3& b) {
    return make_int3(
        v.x + b.x,
        v.y + b.y,
        v.z + b.z
    );
}

__device__ inline uint3 operator%(const uint3& v, const int s) {
    return make_uint3(
        v.x % s,
        v.y % s,
        v.z % s
    );
}