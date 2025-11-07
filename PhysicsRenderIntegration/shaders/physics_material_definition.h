// physics_material_definition.h - 物理材质定义（设备端）

#ifndef PHYSICS_MATERIAL_DEFINITION_H
#define PHYSICS_MATERIAL_DEFINITION_H

#include <cuda_runtime.h>

/**
 * @brief 物理材质定义（GPU）
 *
 * 扩展标准 PBR 材质，添加物理相关属性
 */
struct PhysicsMaterialDefinition {
    // PBR 基础参数
    float3 albedo;              // 反照率 / 基础颜色
    float3 emission;            // 发光颜色
    float metallic;             // 金属度 (0-1)
    float roughness;            // 粗糙度 (0-1)
    float ior;                  // 折射率

    // 纹理对象
    cudaTextureObject_t textureAlbedo;
    cudaTextureObject_t textureNormal;
    cudaTextureObject_t textureMetallic;
    cudaTextureObject_t textureRoughness;

    // 物理属性（影响视觉）
    float density;              // 密度
    float temperature;          // 温度（影响发光）
    float damageLevel;          // 破损程度 (0-1)
    float wetness;              // 湿润度 (0-1)

    // 标志位
    unsigned int flags;

    // 填充到 16 字节对齐
    int pad[3];
};

// 材质标志位
#define MATERIAL_FLAG_THIN_WALLED       (1 << 0)
#define MATERIAL_FLAG_HAS_ALBEDO_TEX    (1 << 1)
#define MATERIAL_FLAG_HAS_NORMAL_TEX    (1 << 2)
#define MATERIAL_FLAG_HAS_METALLIC_TEX  (1 << 3)
#define MATERIAL_FLAG_HAS_ROUGHNESS_TEX (1 << 4)
#define MATERIAL_FLAG_EMISSIVE          (1 << 5)

#endif // PHYSICS_MATERIAL_DEFINITION_H
