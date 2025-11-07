// anyhit.cu - 任意命中程序

#include <optix.h>
#include "physics_system_data.h"

extern "C" {
__constant__ PhysicsSystemData sysData;
}

/**
 * @brief 透明度测试任意命中程序
 *
 * 用于处理带有 cutout texture 的材质
 */
extern "C" __global__ void __anyhit__cutout() {
    // 获取 SBT 数据
    const PhysicsGeometryInstanceData* instanceData =
        reinterpret_cast<const PhysicsGeometryInstanceData*>(
            optixGetSbtDataPointer()
        );

    // 获取材质
    const PhysicsMaterialDefinition& material =
        sysData.materialDefinitions[instanceData->materialID];

    // TODO: 实现纹理采样和透明度测试
    // 如果材质有 cutout 纹理：
    // 1. 获取 UV 坐标
    // 2. 采样纹理
    // 3. 如果 alpha < threshold，忽略交点

    // 暂时：总是接受交点
    // optixIgnoreIntersection();  // 拒绝交点
}

/**
 * @brief 阴影光线任意命中程序
 */
extern "C" __global__ void __anyhit__shadow() {
    // 对于不透明几何，可以立即终止
    optixTerminateRay();
}
