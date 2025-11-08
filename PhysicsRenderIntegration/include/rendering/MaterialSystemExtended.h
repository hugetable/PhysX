// MaterialSystemExtended.h - 扩展材质系统
// 支持 GGX BRDF、透明材质、次表面散射

#ifndef MATERIAL_SYSTEM_EXTENDED_H
#define MATERIAL_SYSTEM_EXTENDED_H

#include <cuda.h>
#include <vector>
#include "physics_material_definition.h"

namespace PhysicsRender {

/**
 * @brief BRDF 类型
 */
enum class BRDFType {
    LAMBERT,        // Lambert 漫反射
    GGX,            // GGX 微表面模型（金属/粗糙度工作流）
    PHONG,          // Phong 镜面反射
    DISNEY,         // Disney Principled BRDF
    OREN_NAYAR      // Oren-Nayar 漫反射（粗糙表面）
};

/**
 * @brief 材质标志位
 */
enum MaterialFlags {
    MAT_FLAG_NONE               = 0,
    MAT_FLAG_DOUBLE_SIDED       = 1 << 0,   // 双面材质
    MAT_FLAG_THIN_WALLED        = 1 << 1,   // 薄壁（单层透明）
    MAT_FLAG_HAS_ALBEDO_TEX     = 1 << 2,   // 有反照率纹理
    MAT_FLAG_HAS_NORMAL_TEX     = 1 << 3,   // 有法线贴图
    MAT_FLAG_HAS_METALLIC_TEX   = 1 << 4,   // 有金属度纹理
    MAT_FLAG_HAS_ROUGHNESS_TEX  = 1 << 5,   // 有粗糙度纹理
    MAT_FLAG_HAS_EMISSION_TEX   = 1 << 6,   // 有发光纹理
    MAT_FLAG_ALPHA_TESTED       = 1 << 7,   // Alpha 测试
    MAT_FLAG_TRANSPARENT        = 1 << 8,   // 透明材质
    MAT_FLAG_SUBSURFACE         = 1 << 9,   // 次表面散射
    MAT_FLAG_EMISSIVE           = 1 << 10   // 发光材质
};

/**
 * @brief 扩展材质参数
 */
struct ExtendedMaterialParams {
    // 基础 PBR 参数
    float3 albedo = make_float3(0.8f, 0.8f, 0.8f);
    float3 emission = make_float3(0.0f, 0.0f, 0.0f);

    float metallic = 0.0f;      // 金属度 [0,1]
    float roughness = 0.5f;     // 粗糙度 [0,1]
    float ior = 1.5f;           // 折射率（玻璃~1.5, 水~1.33）
    float opacity = 1.0f;       // 不透明度 [0,1]

    // 次表面散射参数
    float3 subsurfaceColor = make_float3(0.8f, 0.2f, 0.2f);
    float subsurfaceRadius = 0.0f;      // 散射半径
    float subsurfaceScale = 1.0f;       // 散射强度

    // 各向异性（Anisotropy）
    float anisotropy = 0.0f;            // [-1,1], 0=各向同性
    float anisotropyRotation = 0.0f;    // [0,1], 旋转角度

    // 清漆层（Clearcoat）
    float clearcoat = 0.0f;             // [0,1]
    float clearcoatRoughness = 0.1f;    // [0,1]

    // 光泽（Sheen）- 用于织物
    float3 sheenColor = make_float3(0.0f, 0.0f, 0.0f);
    float sheenRoughness = 0.5f;

    // 透明参数
    float3 transmittance = make_float3(1.0f, 1.0f, 1.0f);  // 透射颜色
    float transmissionDepth = 1.0f;     // 透射深度

    // 物理属性（来自物理引擎）
    float density = 1.0f;
    float temperature = 0.0f;
    float damageLevel = 0.0f;
    float wetness = 0.0f;

    // BRDF 类型
    BRDFType brdfType = BRDFType::GGX;

    // 纹理
    cudaTextureObject_t textureAlbedo = 0;
    cudaTextureObject_t textureNormal = 0;
    cudaTextureObject_t textureMetallic = 0;
    cudaTextureObject_t textureRoughness = 0;
    cudaTextureObject_t textureEmission = 0;
    cudaTextureObject_t textureOpacity = 0;

    // 标志位
    unsigned int flags = MAT_FLAG_NONE;

    // 材质 ID
    int materialID = -1;
};

/**
 * @brief 材质预设
 */
namespace MaterialPresets {
    // 金属材质
    ExtendedMaterialParams Chrome();
    ExtendedMaterialParams Gold();
    ExtendedMaterialParams Copper();
    ExtendedMaterialParams Aluminum();
    ExtendedMaterialParams Iron();

    // 非金属材质
    ExtendedMaterialParams Plastic();
    ExtendedMaterialParams Rubber();
    ExtendedMaterialParams Wood();
    ExtendedMaterialParams Concrete();
    ExtendedMaterialParams Ceramic();

    // 透明材质
    ExtendedMaterialParams Glass();
    ExtendedMaterialParams Water();
    ExtendedMaterialParams ClearPlastic();
    ExtendedMaterialParams FrostedGlass();

    // 次表面散射材质
    ExtendedMaterialParams Skin();
    ExtendedMaterialParams Marble();
    ExtendedMaterialParams Wax();
    ExtendedMaterialParams Milk();

    // 发光材质
    ExtendedMaterialParams Emissive(const float3& color, float intensity);

    // 织物材质
    ExtendedMaterialParams Velvet();
    ExtendedMaterialParams Silk();
}

/**
 * @brief 材质系统管理器
 */
class MaterialSystemManager {
public:
    /**
     * @brief 构造函数
     */
    MaterialSystemManager();

    /**
     * @brief 析构函数
     */
    ~MaterialSystemManager();

    /**
     * @brief 添加材质
     * @param params 材质参数
     * @return 材质索引
     */
    int addMaterial(const ExtendedMaterialParams& params);

    /**
     * @brief 更新材质
     * @param index 材质索引
     * @param params 新的材质参数
     */
    void updateMaterial(int index, const ExtendedMaterialParams& params);

    /**
     * @brief 移除材质
     * @param index 材质索引
     */
    void removeMaterial(int index);

    /**
     * @brief 获取材质数量
     */
    int getMaterialCount() const { return static_cast<int>(materials_.size()); }

    /**
     * @brief 上传材质数据到 GPU
     * @return GPU 设备指针
     */
    CUdeviceptr uploadToGPU();

    /**
     * @brief 获取 GPU 缓冲区指针
     */
    CUdeviceptr getGPUBuffer() const { return d_materialBuffer_; }

    /**
     * @brief 更新物理驱动的材质属性
     * @param index 材质索引
     * @param temperature 温度
     * @param damageLevel 破损等级
     * @param wetness 湿润度
     */
    void updatePhysicsProperties(
        int index,
        float temperature,
        float damageLevel,
        float wetness
    );

    /**
     * @brief 加载材质库（从文件）
     * @param filename 文件名
     * @return 加载的材质数量
     */
    int loadMaterialLibrary(const char* filename);

    /**
     * @brief 保存材质库（到文件）
     * @param filename 文件名
     * @return 是否成功
     */
    bool saveMaterialLibrary(const char* filename) const;

private:
    std::vector<ExtendedMaterialParams> materials_;
    std::vector<PhysicsMaterialDefinition> deviceMaterials_;

    // GPU 缓冲区
    CUdeviceptr d_materialBuffer_;
    size_t bufferSize_;

    /**
     * @brief 将扩展材质参数转换为设备材质定义
     */
    PhysicsMaterialDefinition convertToDeviceMaterial(const ExtendedMaterialParams& params);
};

} // namespace PhysicsRender

#endif // MATERIAL_SYSTEM_EXTENDED_H
