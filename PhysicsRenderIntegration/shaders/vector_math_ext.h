// vector_math_ext.h - 向量数学扩展

#ifndef VECTOR_MATH_EXT_H
#define VECTOR_MATH_EXT_H

#include <cuda_runtime.h>

#ifndef M_PIf
#define M_PIf 3.14159265358979323846f
#endif

/**
 * @brief 创建正交基
 */
__device__ __forceinline__ void createOrthonormalBasis(
    const float3& normal,
    float3& tangent,
    float3& bitangent
) {
    // 找一个不平行于 normal 的向量
    float3 up = (fabsf(normal.z) < 0.999f) ? make_float3(0, 0, 1) : make_float3(1, 0, 0);

    // Gram-Schmidt 正交化
    tangent = normalize(cross(up, normal));
    bitangent = cross(normal, tangent);
}

/**
 * @brief 向量点积
 */
__device__ __forceinline__ float dot(const float3& a, const float3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/**
 * @brief 向量叉积
 */
__device__ __forceinline__ float3 cross(const float3& a, const float3& b) {
    return make_float3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

/**
 * @brief 向量归一化
 */
__device__ __forceinline__ float3 normalize(const float3& v) {
    float invLen = rsqrtf(dot(v, v));
    return make_float3(v.x * invLen, v.y * invLen, v.z * invLen);
}

/**
 * @brief 向量长度
 */
__device__ __forceinline__ float length(const float3& v) {
    return sqrtf(dot(v, v));
}

/**
 * @brief 线性插值
 */
__device__ __forceinline__ float3 lerp(const float3& a, const float3& b, float t) {
    return make_float3(
        a.x + t * (b.x - a.x),
        a.y + t * (b.y - a.y),
        a.z + t * (b.z - a.z)
    );
}

/**
 * @brief 向量加法
 */
__device__ __forceinline__ float3 operator+(const float3& a, const float3& b) {
    return make_float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

/**
 * @brief 向量减法
 */
__device__ __forceinline__ float3 operator-(const float3& a, const float3& b) {
    return make_float3(a.x - b.x, a.y - b.y, a.z - b.z);
}

/**
 * @brief 向量乘法（分量）
 */
__device__ __forceinline__ float3 operator*(const float3& a, const float3& b) {
    return make_float3(a.x * b.x, a.y * b.y, a.z * b.z);
}

/**
 * @brief 向量标量乘法
 */
__device__ __forceinline__ float3 operator*(const float3& v, float s) {
    return make_float3(v.x * s, v.y * s, v.z * s);
}

__device__ __forceinline__ float3 operator*(float s, const float3& v) {
    return make_float3(v.x * s, v.y * s, v.z * s);
}

/**
 * @brief 向量除法
 */
__device__ __forceinline__ float3 operator/(const float3& v, float s) {
    float invS = 1.0f / s;
    return make_float3(v.x * invS, v.y * invS, v.z * invS);
}

/**
 * @brief 向量取反
 */
__device__ __forceinline__ float3 operator-(const float3& v) {
    return make_float3(-v.x, -v.y, -v.z);
}

/**
 * @brief 向量复合赋值
 */
__device__ __forceinline__ void operator+=(float3& a, const float3& b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
}

__device__ __forceinline__ void operator*=(float3& v, float s) {
    v.x *= s;
    v.y *= s;
    v.z *= s;
}

__device__ __forceinline__ void operator/=(float3& v, float s) {
    float invS = 1.0f / s;
    v.x *= invS;
    v.y *= invS;
    v.z *= invS;
}

#endif // VECTOR_MATH_EXT_H
