// closesthit_physics.cu - 物理材质最近命中程序

#include <optix.h>
#include "physics_system_data.h"
#include "per_ray_data.h"
#include "vector_math_ext.h"
#include "random_number_generators.h"

extern "C" {
__constant__ PhysicsSystemData sysData;
}

/**
 * @brief Lambert 漫反射 BSDF 评估
 */
__device__ float3 evalLambertBSDF(
    const PhysicsMaterialDefinition& material,
    const float3& normal,
    const float3& wo,
    const float3& wi,
    float& pdf
) {
    // Lambert BRDF: albedo / pi
    pdf = fmaxf(0.0f, dot(normal, wi)) / M_PIf;
    return material.albedo / M_PIf;
}

/**
 * @brief Lambert 漫反射 BSDF 采样
 */
__device__ float3 sampleLambertBSDF(
    const PhysicsMaterialDefinition& material,
    const float3& normal,
    const float3& wo,
    unsigned int& seed,
    float3& wi,
    float& pdf
) {
    // 余弦加权半球采样
    const float r1 = rng(seed);
    const float r2 = rng(seed);

    const float phi = 2.0f * M_PIf * r1;
    const float cosTheta = sqrtf(r2);
    const float sinTheta = sqrtf(1.0f - r2);

    // 局部坐标系
    float3 tangent, bitangent;
    createOrthonormalBasis(normal, tangent, bitangent);

    // 世界坐标系方向
    wi = sinTheta * cosf(phi) * tangent +
         sinTheta * sinf(phi) * bitangent +
         cosTheta * normal;

    pdf = cosTheta / M_PIf;

    return material.albedo / M_PIf;
}

/**
 * @brief 最近命中程序 - 物理材质
 */
extern "C" __global__ void __closesthit__physics_radiance() {
    // 获取 SBT 数据
    const PhysicsGeometryInstanceData* instanceData =
        reinterpret_cast<const PhysicsGeometryInstanceData*>(
            optixGetSbtDataPointer()
        );

    // 获取材质
    const PhysicsMaterialDefinition& material =
        sysData.materialDefinitions[instanceData->materialID];

    // 获取交点信息
    const float3 rayOrigin = optixGetWorldRayOrigin();
    const float3 rayDirection = optixGetWorldRayDirection();
    const float hitT = optixGetRayTmax();

    const float3 hitPoint = rayOrigin + hitT * rayDirection;

    // 获取法线（简化：使用几何法线）
    const float3 geometricNormal = make_float3(
        __uint_as_float(optixGetAttribute_0()),
        __uint_as_float(optixGetAttribute_1()),
        __uint_as_float(optixGetAttribute_2())
    );

    float3 normal = normalize(geometricNormal);

    // 确保法线面向射线源
    if (dot(normal, rayDirection) > 0.0f) {
        normal = -normal;
    }

    // 获取 payload
    PerRayData prd;
    prd.radiance = make_float3_from_args();
    prd.throughput = make_float3_from_args();
    prd.depth = optixGetPayload_6();
    prd.seed = optixGetPayload_7();

    // 应用物理属性到材质
    float3 albedo = material.albedo;

    // 破损效果：降低亮度
    if (material.damageLevel > 0.0f) {
        albedo *= (1.0f - material.damageLevel * 0.5f);
    }

    // 湿润效果：增加光泽（这里简化为稍微变暗）
    if (material.wetness > 0.0f) {
        albedo *= (1.0f - material.wetness * 0.2f);
    }

    // 温度效果：发光
    float3 emission = material.emission;
    if (material.temperature > 0.0f) {
        // 温度越高，发光越强
        emission += make_float3(1.0f, 0.5f, 0.1f) * material.temperature;
    }

    // 添加自发光
    prd.radiance += prd.throughput * emission;

    // 采样 BSDF
    float3 wi;
    float pdf;
    float3 bsdfValue = sampleLambertBSDF(
        material,
        normal,
        -rayDirection,
        prd.seed,
        wi,
        pdf
    );

    if (pdf > 0.0f) {
        // 更新 throughput
        prd.throughput *= bsdfValue * fmaxf(0.0f, dot(normal, wi)) / pdf;

        // 设置下一次光线追踪的参数
        prd.hitPoint = hitPoint + sysData.sceneEpsilon * normal;
        prd.direction = wi;
    } else {
        // 终止光线
        prd.depth = -1;
    }

    // 写回 payload
    set_float3_payload(prd.radiance, 0);
    set_float3_payload(prd.throughput, 3);
    optixSetPayload_6(prd.depth);
    optixSetPayload_7(prd.seed);
}

/**
 * @brief 阴影光线最近命中程序（遮挡测试）
 */
extern "C" __global__ void __closesthit__shadow() {
    // 简单地设置 payload 表示被遮挡
    optixSetPayload_0(0);  // 0 = 被遮挡
}
