// PhysicsRenderer.h - 物理渲染器
// 扩展自 OptiX_Apps Raytracer

#ifndef PHYSICS_RENDERER_H
#define PHYSICS_RENDERER_H

#include "config.h"
#include "Raytracer.h"  // OptiX_Apps
#include "physics/PhysicsSimulator.h"

#include <cuda.h>
#include <optix.h>
#include <vector>
#include <memory>

namespace PhysicsRender {

/**
 * @brief 物理材质定义（扩展 OptiX MaterialDefinition）
 */
struct PhysicsMaterialDefinition {
    // 基础 PBR 参数
    float3 albedo;
    float3 emission;
    float metallic;
    float roughness;
    float ior;

    // 纹理
    cudaTextureObject_t textureAlbedo;
    cudaTextureObject_t textureNormal;
    cudaTextureObject_t textureMetallic;
    cudaTextureObject_t textureRoughness;

    // 物理属性（影响视觉效果）
    float density;
    float temperature;     // 影响发光颜色
    float damageLevel;     // Blast 破损程度 (0-1)
    float wetness;         // Flow 湿润度 (0-1)

    // 标志位
    unsigned int flags;
};

/**
 * @brief 动态几何数据
 */
struct DynamicGeometry {
    int geometryID;
    OptixTraversableHandle handle;  // BLAS handle
    std::vector<float> transforms;  // 实例变换矩阵数组
    bool needsRebuild;              // 是否需要重建
};

/**
 * @brief 物理渲染器配置
 */
struct PhysicsRendererConfig {
    int2 resolution = make_int2(1920, 1080);
    int samplesPerPixel = 4;
    int maxPathLength = 8;
    float sceneEpsilon = 0.001f;

    // 性能选项
    bool useAsyncUpdate = true;
    bool useDynamicBLAS = true;
    bool enableDenoiser = true;
};

/**
 * @brief 物理渲染器
 *
 * 扩展 OptiX_Apps Raytracer，添加动态几何支持
 */
class PhysicsRenderer : public Raytracer {
public:
    /**
     * @brief 构造函数
     * @param config 渲染器配置
     */
    explicit PhysicsRenderer(const PhysicsRendererConfig& config);

    /**
     * @brief 析构函数
     */
    ~PhysicsRenderer() override;

    /**
     * @brief 初始化渲染器
     * @return 成功返回 true
     */
    bool initialize();

    /**
     * @brief 设置动态几何
     * @param renderables 可渲染对象列表
     */
    void setDynamicGeometry(const std::vector<RenderableObject>& renderables);

    /**
     * @brief 更新变换矩阵
     * @param transforms 变换矩阵数组
     */
    void updateTransforms(const std::vector<float*>& transforms);

    /**
     * @brief 重建顶层加速结构 (TLAS)
     */
    void rebuildTLAS();

    /**
     * @brief 重建粒子几何
     * @param positions 粒子位置（GPU buffer）
     * @param radii 粒子半径（GPU buffer）
     * @param count 粒子数量
     */
    void rebuildParticleGeometry(
        CUdeviceptr positions,
        CUdeviceptr radii,
        int count
    );

    /**
     * @brief 更新粒子位置（快速路径）
     * @param positions 粒子位置（GPU buffer）
     * @param count 粒子数量
     */
    void updateParticlePositions(CUdeviceptr positions, int count);

    /**
     * @brief 添加物理材质
     * @param material 材质定义
     * @return 材质 ID
     */
    int addPhysicsMaterial(const PhysicsMaterialDefinition& material);

    /**
     * @brief 更新物理材质
     * @param materialID 材质 ID
     * @param material 材质定义
     */
    void updatePhysicsMaterial(int materialID, const PhysicsMaterialDefinition& material);

    /**
     * @brief 渲染一帧
     * @return 渲染时间（毫秒）
     */
    unsigned int render() override;

private:
    PhysicsRendererConfig config_;

    // OptiX 资源
    OptixDeviceContext optixContext_;
    OptixModule optixModule_;
    OptixPipeline pipeline_;
    OptixShaderBindingTable sbt_;

    // 加速结构
    OptixTraversableHandle topLevelAS_;
    std::vector<OptixInstance> instances_;
    CUdeviceptr instanceBuffer_;

    std::vector<DynamicGeometry> dynamicGeometries_;

    // 粒子几何
    struct ParticleGeometry {
        OptixTraversableHandle handle;
        CUdeviceptr positionsBuffer;
        CUdeviceptr radiiBuffer;
        CUdeviceptr tempBuffer;
        size_t tempBufferSize;
        int count;
    };
    ParticleGeometry particleGeom_;

    // 材质系统
    std::vector<PhysicsMaterialDefinition> physicsMaterials_;
    CUdeviceptr materialBuffer_;

    // 输出缓冲区
    CUdeviceptr outputBuffer_;
    CUdeviceptr accumBuffer_;

    // 辅助函数
    bool createOptixContext();
    bool createModule();
    bool createPipeline();
    bool createSBT();

    void buildBLAS(const RenderableObject& obj, DynamicGeometry& geom);
    void updateBLAS(DynamicGeometry& geom);
    void buildTLAS();

    void cleanupOptixResources();
};

} // namespace PhysicsRender

#endif // PHYSICS_RENDERER_H
