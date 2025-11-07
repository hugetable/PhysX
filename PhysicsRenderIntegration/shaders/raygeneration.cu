// raygeneration.cu - 光线生成程序

#include <optix.h>
#include "physics_system_data.h"
#include "per_ray_data.h"
#include "vector_math_ext.h"
#include "random_number_generators.h"
#include "camera_definition.h"

extern "C" {
__constant__ PhysicsSystemData sysData;
}

/**
 * @brief 光线生成程序
 *
 * 为每个像素生成相机光线并追踪
 */
extern "C" __global__ void __raygen__pinhole_camera() {
    const uint3 launch_idx = optixGetLaunchIndex();
    const uint3 launch_dim = optixGetLaunchDimensions();

    // 像素坐标
    const unsigned int px = launch_idx.x;
    const unsigned int py = launch_idx.y;

    if (px >= sysData.resolution.x || py >= sysData.resolution.y) {
        return;
    }

    // 随机数生成器
    unsigned int seed = tea<4>(px + py * sysData.resolution.x,
                                sysData.iterationIndex);

    // 相机
    const CameraDefinition& camera = sysData.cameraDefinitions[0];

    // 抗锯齿抖动
    const float2 jitter = make_float2(rng(seed), rng(seed));
    const float2 screen = make_float2(
        (static_cast<float>(px) + jitter.x) / static_cast<float>(sysData.resolution.x),
        (static_cast<float>(py) + jitter.y) / static_cast<float>(sysData.resolution.y)
    );

    // NDC 坐标 [-1, 1]
    const float2 ndc = screen * 2.0f - 1.0f;

    // 生成光线
    float3 origin = camera.cameraPosition;
    float3 direction = normalize(
        camera.cameraU * ndc.x +
        camera.cameraV * ndc.y +
        camera.cameraW
    );

    // 光线追踪
    PerRayData prd;
    prd.radiance = make_float3(0.0f);
    prd.throughput = make_float3(1.0f);
    prd.depth = 0;
    prd.seed = seed;

    // 追踪主光线
    for (int depth = 0; depth < sysData.pathLengths.y; ++depth) {
        prd.depth = depth;

        optixTrace(
            sysData.topObject,
            origin,
            direction,
            sysData.sceneEpsilon,     // tmin
            1e16f,                     // tmax
            0.0f,                      // rayTime
            OptixVisibilityMask(255),
            OPTIX_RAY_FLAG_NONE,
            0,                         // SBT offset
            1,                         // SBT stride
            0,                         // missSBTIndex
            // Payload
            float3_as_args(prd.radiance),
            float3_as_args(prd.throughput),
            prd.depth,
            prd.seed
        );

        // 如果光线被吸收，结束
        if (prd.depth == -1) {
            break;
        }

        // 更新光线
        origin = prd.hitPoint;
        direction = prd.direction;

        // 俄罗斯轮盘
        if (depth >= sysData.pathLengths.x) {
            const float probability = fmaxf(prd.throughput);
            if (probability < rng(seed)) {
                break;
            }
            prd.throughput /= probability;
        }
    }

    // 累积结果
    const unsigned int pixelIndex = py * sysData.resolution.x + px;
    float3* output = reinterpret_cast<float3*>(sysData.outputBuffer);

    if (sysData.iterationIndex == 0) {
        output[pixelIndex] = prd.radiance;
    } else {
        // 累积平均
        const float t = 1.0f / static_cast<float>(sysData.iterationIndex + 1);
        output[pixelIndex] = lerp(output[pixelIndex], prd.radiance, t);
    }
}
