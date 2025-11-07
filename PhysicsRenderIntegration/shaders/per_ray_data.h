// per_ray_data.h - 光线数据结构

#ifndef PER_RAY_DATA_H
#define PER_RAY_DATA_H

#include <cuda_runtime.h>

/**
 * @brief 每条光线的数据
 *
 * 通过 OptiX payload 传递
 */
struct PerRayData {
    float3 radiance;      // 累积的辐射度
    float3 throughput;    // 路径吞吐量
    int depth;            // 路径深度（-1 表示终止）
    unsigned int seed;    // 随机数种子

    // 下一次光线追踪的参数
    float3 hitPoint;      // 命中点
    float3 direction;     // 反射/折射方向
};

// Payload 辅助宏
#define float3_as_args(v) \
    __float_as_uint((v).x), __float_as_uint((v).y), __float_as_uint((v).z)

__device__ __forceinline__ float3 make_float3_from_args() {
    return make_float3(
        __uint_as_float(optixGetPayload_0()),
        __uint_as_float(optixGetPayload_1()),
        __uint_as_float(optixGetPayload_2())
    );
}

__device__ __forceinline__ void set_float3_payload(const float3& v, int offset) {
    optixSetPayload_0(offset + 0, __float_as_uint(v.x));
    optixSetPayload_1(offset + 1, __float_as_uint(v.y));
    optixSetPayload_2(offset + 2, __float_as_uint(v.z));
}

#endif // PER_RAY_DATA_H
