// miss.cu - 未命中程序

#include <optix.h>
#include "physics_system_data.h"
#include "per_ray_data.h"

extern "C" {
__constant__ PhysicsSystemData sysData;
}

/**
 * @brief 环境贴图未命中程序
 */
extern "C" __global__ void __miss__env_constant() {
    // 获取 payload
    PerRayData prd;
    prd.radiance = make_float3_from_args();
    prd.throughput = make_float3_from_args();
    prd.depth = optixGetPayload_6();
    prd.seed = optixGetPayload_7();

    // 环境光（简单常量）
    const float3 envColor = make_float3(0.2f, 0.3f, 0.4f);  // 天空蓝

    // 累积辐射度
    prd.radiance += prd.throughput * envColor;

    // 标记光线结束
    prd.depth = -1;

    // 写回 payload
    set_float3_payload(prd.radiance, 0);
    set_float3_payload(prd.throughput, 3);
    optixSetPayload_6(prd.depth);
    optixSetPayload_7(prd.seed);
}

/**
 * @brief 黑色未命中程序
 */
extern "C" __global__ void __miss__black() {
    // 获取 payload
    PerRayData prd;
    prd.radiance = make_float3_from_args();
    prd.throughput = make_float3_from_args();
    prd.depth = optixGetPayload_6();
    prd.seed = optixGetPayload_7();

    // 黑色背景
    // radiance 保持不变

    // 标记光线结束
    prd.depth = -1;

    // 写回 payload
    set_float3_payload(prd.radiance, 0);
    set_float3_payload(prd.throughput, 3);
    optixSetPayload_6(prd.depth);
    optixSetPayload_7(prd.seed);
}
