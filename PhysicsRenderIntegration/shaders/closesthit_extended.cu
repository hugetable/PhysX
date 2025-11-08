// closesthit_extended.cu - 扩展的 Closest Hit 着色器
// 支持 GGX、透明材质、次表面散射、多光源

#include <optix.h>
#include "physics_system_data.h"
#include "physics_material_definition.h"
#include "light_definition.h"
#include "vector_math_ext.h"
#include "random.cuh"
#include "bsdf_extended.cuh"

// ============================================================================
// 外部变量
// ============================================================================

extern "C" {
    __constant__ PhysicsSystemData sysData;
}

// ============================================================================
// Payload 定义
// ============================================================================

struct RadiancePayload {
    float3 radiance;      // 累积辐射度
    float3 throughput;    // 路径吞吐量
    float3 origin;        // 下一条光线的起点
    float3 direction;     // 下一条光线的方向
    int depth;            // 路径深度
    unsigned int seed;    // 随机数种子
    bool done;            // 是否终止
};

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 评估直接光照（单个光源）
 */
__device__ float3 EvaluateDirectLighting(
    const float3& surfacePos,
    const float3& normal,
    const float3& viewDir,
    const PhysicsMaterialDefinition& material,
    const LightDefinition& light,
    unsigned int& seed
) {
    float3 lightDir;
    float3 lightRadiance;
    float distance;

    // 根据光源类型计算光线方向和辐射度
    switch (light.type) {
    case 0: {  // POINT
        float3 toLight = light.position - surfacePos;
        distance = length(toLight);
        lightDir = toLight / distance;

        // 平方反比衰减
        float attenuation = 1.0f / (distance * distance);
        if (light.falloff > 0.0f) {
            attenuation = powf(fmaxf(1.0f - distance / 100.0f, 0.0f), light.falloff);
        }

        lightRadiance = light.emission * attenuation;
        break;
    }

    case 1: {  // DIRECTIONAL
        lightDir = -light.direction;
        distance = 1e10f;  // 无限远
        lightRadiance = light.emission;
        break;
    }

    case 2: {  // SPOT
        float3 toLight = light.position - surfacePos;
        distance = length(toLight);
        lightDir = toLight / distance;

        // 聚光灯衰减
        float spotEffect = dot(-lightDir, light.direction);
        float spotAttenuation = smoothstep(light.outerAngle, light.innerAngle, spotEffect);

        float attenuation = spotAttenuation / (distance * distance);
        lightRadiance = light.emission * attenuation;
        break;
    }

    case 3: {  // AREA_RECT
        // 在矩形上随机采样一个点
        float2 xi = make_float2(rnd(seed), rnd(seed));
        float3 lightPoint = light.position + light.u * (xi.x - 0.5f) * 2.0f + light.v * (xi.y - 0.5f) * 2.0f;

        float3 toLight = lightPoint - surfacePos;
        distance = length(toLight);
        lightDir = toLight / distance;

        // 面积光源衰减
        float NoL = fmaxf(dot(light.normal, -lightDir), 0.0f);
        if (!light.doubleSided && NoL <= 0.0f) {
            return make_float3(0.0f, 0.0f, 0.0f);
        }

        float attenuation = NoL * light.area / (distance * distance);
        lightRadiance = light.emission * attenuation;
        break;
    }

    case 4: {  // AREA_SPHERE
        // 在球面上随机采样一个点
        float2 xi = make_float2(rnd(seed), rnd(seed));
        float phi = 2.0f * M_PI * xi.x;
        float theta = acosf(1.0f - 2.0f * xi.y);

        float3 localDir = make_float3(
            sinf(theta) * cosf(phi),
            sinf(theta) * sinf(phi),
            cosf(theta)
        );

        float3 lightPoint = light.position + localDir * light.radius;

        float3 toLight = lightPoint - surfacePos;
        distance = length(toLight);
        lightDir = toLight / distance;

        float attenuation = light.area / (distance * distance);
        lightRadiance = light.emission * attenuation;
        break;
    }

    default:
        return make_float3(0.0f, 0.0f, 0.0f);
    }

    // 可见性测试（阴影光线）
    float3 shadowOrigin = surfacePos + normal * sysData.sceneEpsilon;

    unsigned int occluded = 0;
    optixTrace(
        sysData.topObject,
        shadowOrigin,
        lightDir,
        sysData.sceneEpsilon,
        distance - sysData.sceneEpsilon,
        0.0f,  // time
        OptixVisibilityMask(255),
        OPTIX_RAY_FLAG_TERMINATE_ON_FIRST_HIT | OPTIX_RAY_FLAG_DISABLE_CLOSESTHIT,
        0,  // SBT offset
        1,  // SBT stride
        0,  // miss SBT index
        occluded
    );

    if (occluded) {
        return make_float3(0.0f, 0.0f, 0.0f);
    }

    // 评估 BRDF
    float3 brdf;

    // 根据材质类型选择 BRDF
    if (material.flags & 0x100) {  // MAT_FLAG_TRANSPARENT
        // 透明材质：不计算直接光照（在折射/反射中处理）
        return make_float3(0.0f, 0.0f, 0.0f);
    }

    // 计算 F0（0 度菲涅尔反射率）
    float3 F0 = lerp(make_float3(0.04f, 0.04f, 0.04f), material.albedo, material.metallic);

    // 应用物理效果
    float3 albedo = material.albedo;
    float roughness = material.roughness;

    if (material.damageLevel > 0.0f) {
        albedo *= (1.0f - material.damageLevel * 0.5f);
        roughness = fminf(roughness + material.damageLevel * 0.3f, 1.0f);
    }

    if (material.wetness > 0.0f) {
        roughness *= (1.0f - material.wetness * 0.5f);
    }

    // 评估 GGX BRDF
    brdf = EvaluateGGX(normal, viewDir, lightDir, albedo, material.metallic, roughness, F0);

    // 次表面散射
    if (material.flags & 0x200) {  // MAT_FLAG_SUBSURFACE
        float3 sssColor = make_float3(0.9f, 0.3f, 0.2f);  // TODO: 从材质读取
        float sssRadius = 0.01f;

        float3 sssContrib = ApproximateSSS(normal, lightDir, albedo, sssColor, sssRadius);
        brdf += sssContrib * 0.5f;  // 混合权重
    }

    float NoL = fmaxf(dot(normal, lightDir), 0.0f);

    return lightRadiance * brdf * NoL;
}

/**
 * @brief 采样 BRDF 并生成下一条光线
 */
__device__ void SampleMaterial(
    const float3& wo,
    const float3& normal,
    const PhysicsMaterialDefinition& material,
    unsigned int& seed,
    float3& wi,
    float3& weight
) {
    // 透明材质
    if (material.flags & 0x100) {  // MAT_FLAG_TRANSPARENT
        bool thinWalled = (material.flags & 0x02) != 0;
        float3 transmittance = make_float3(0.95f, 0.95f, 0.95f);

        float pdf;
        SampleTransparent(wo, normal, material.ior, transmittance, thinWalled, seed, wi, weight, pdf);
        return;
    }

    // 计算 F0
    float3 F0 = lerp(make_float3(0.04f, 0.04f, 0.04f), material.albedo, material.metallic);

    // 应用物理效果
    float3 albedo = material.albedo;
    float roughness = material.roughness;

    if (material.damageLevel > 0.0f) {
        albedo *= (1.0f - material.damageLevel * 0.5f);
        roughness = fminf(roughness + material.damageLevel * 0.3f, 1.0f);
    }

    if (material.wetness > 0.0f) {
        roughness *= (1.0f - material.wetness * 0.5f);
    }

    // 采样 GGX BRDF
    float pdf;
    SampleGGX(normal, wo, albedo, material.metallic, roughness, F0, seed, wi, weight, pdf);
}

// ============================================================================
// Closest Hit 程序
// ============================================================================

extern "C" __global__ void __closesthit__extended_radiance() {
    // 获取几何数据
    const PhysicsGeometryInstanceData* instanceData =
        reinterpret_cast<PhysicsGeometryInstanceData*>(optixGetSbtDataPointer());

    // 获取材质
    int materialID = instanceData->materialID;
    if (materialID < 0 || materialID >= sysData.numMaterials) {
        return;
    }

    const PhysicsMaterialDefinition& material = sysData.materialDefinitions[materialID];

    // 获取 payload
    RadiancePayload* payload = getRadiancePayload();

    // 如果是发光材质，直接返回发光
    if (material.flags & 0x400) {  // MAT_FLAG_EMISSIVE
        float3 emission = material.emission;

        // 温度影响发光
        if (material.temperature > 0.0f) {
            emission += make_float3(1.0f, 0.5f, 0.1f) * material.temperature;
        }

        payload->radiance += payload->throughput * emission;
        payload->done = true;
        return;
    }

    // 获取命中点信息
    float3 rayOrigin = optixGetWorldRayOrigin();
    float3 rayDir = optixGetWorldRayDirection();
    float hitT = optixGetRayTmax();

    float3 hitPos = rayOrigin + rayDir * hitT;

    // 获取几何法线（从属性）
    float3 geometricNormal = make_float3(
        __uint_as_float(optixGetAttribute_0()),
        __uint_as_float(optixGetAttribute_1()),
        __uint_as_float(optixGetAttribute_2())
    );

    geometricNormal = normalize(geometricNormal);

    // TODO: 法线贴图
    float3 shadingNormal = geometricNormal;

    // 视角方向
    float3 viewDir = -rayDir;

    // ========================================================================
    // 直接光照
    // ========================================================================

    float3 directLighting = make_float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < sysData.numLights; ++i) {
        directLighting += EvaluateDirectLighting(
            hitPos,
            shadingNormal,
            viewDir,
            material,
            sysData.lightDefinitions[i],
            payload->seed
        );
    }

    payload->radiance += payload->throughput * directLighting;

    // ========================================================================
    // 间接光照（路径追踪）
    // ========================================================================

    // 检查路径深度
    if (payload->depth >= sysData.pathLengths.y) {
        payload->done = true;
        return;
    }

    // 俄罗斯轮盘赌终止
    float rrProbability = 0.95f;
    if (payload->depth > sysData.pathLengths.x) {
        float luminance = payload->throughput.x * 0.2126f +
                          payload->throughput.y * 0.7152f +
                          payload->throughput.z * 0.0722f;

        rrProbability = fminf(luminance, 0.95f);

        if (rnd(payload->seed) > rrProbability) {
            payload->done = true;
            return;
        }
    }

    // 采样 BRDF
    float3 nextDir;
    float3 brdfWeight;

    SampleMaterial(viewDir, shadingNormal, material, payload->seed, nextDir, brdfWeight);

    // 更新 payload
    payload->throughput *= brdfWeight / rrProbability;
    payload->origin = hitPos + shadingNormal * sysData.sceneEpsilon;
    payload->direction = nextDir;
    payload->depth++;

    // 检查吞吐量
    float maxThroughput = fmaxf(payload->throughput.x, fmaxf(payload->throughput.y, payload->throughput.z));
    if (maxThroughput < 0.001f) {
        payload->done = true;
    }
}

// ============================================================================
// Any Hit 程序（Alpha 测试）
// ============================================================================

extern "C" __global__ void __anyhit__alpha_test() {
    const PhysicsGeometryInstanceData* instanceData =
        reinterpret_cast<PhysicsGeometryInstanceData*>(optixGetSbtDataPointer());

    int materialID = instanceData->materialID;
    if (materialID < 0 || materialID >= sysData.numMaterials) {
        return;
    }

    const PhysicsMaterialDefinition& material = sysData.materialDefinitions[materialID];

    // 如果材质有 alpha 测试标志
    if (material.flags & 0x80) {  // MAT_FLAG_ALPHA_TESTED
        // TODO: 实现纹理采样和 alpha 测试
        // float alpha = tex2D(material.textureOpacity, uv);
        // if (alpha < 0.5f) {
        //     optixIgnoreIntersection();
        // }
    }
}
