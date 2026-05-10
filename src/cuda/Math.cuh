#include <vector_functions.h>
#include <vector_types.h>

__device__ inline float3 operator+(const float3& a, const float3& b) {
    return make_float3(
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    );
}

__device__ inline float3 operator*(const float s, const float3& b) {
    return make_float3(
        s * b.x,
        s * b.y,
        s * b.z
    );
}

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

__device__ inline float3 operator-(const float3& a, const float3& b) {
    return make_float3(
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    );
}



__device__ inline float3 operator*(const float3& a, const float3& b) {
    return make_float3(
        a.x * b.x,
        a.y * b.y,
        a.z * b.z
    );
}

__device__ inline float3 operator*(const float3& a, const float& b) {
    return make_float3(
        a.x * b,
        a.y * b,
        a.z * b
    );
}

__host__ __device__
inline float3 cross(const float3 a, const float3 b)
{
    float3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

__host__ __device__
inline float dot(const float3 a, const float3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

__host__ __device__
inline float signf(float x)
{
    return (x > 0.0f) - (x < 0.0f);
}

__host__ __device__
inline float dot2(const float3 v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

__host__ __device__
inline float clampf(float x, float a, float b)
{
    return fminf(fmaxf(x, a), b);
}