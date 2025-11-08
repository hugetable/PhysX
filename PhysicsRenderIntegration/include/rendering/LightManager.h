// LightManager.h - 光源管理系统
// 负责光源参数管理和 GPU 数据上传

#ifndef LIGHT_MANAGER_H
#define LIGHT_MANAGER_H

#include <cuda.h>
#include <vector>
#include <memory>
#include "light_definition.h"

namespace PhysicsRender {

/**
 * @brief 光源类型
 */
enum class LightType {
    POINT,          // 点光源
    DIRECTIONAL,    // 方向光源（太阳光）
    SPOT,           // 聚光灯
    AREA_RECT,      // 矩形区域光
    AREA_SPHERE,    // 球形区域光
    AREA_DISK,      // 圆盘区域光
    ENV             // 环境光/IBL
};

/**
 * @brief 点光源参数
 */
struct PointLightParams {
    float3 position = make_float3(0.0f, 5.0f, 0.0f);
    float3 emission = make_float3(100.0f, 100.0f, 100.0f);  // 发光强度
    float radius = 0.1f;        // 光源半径（软阴影）
    float falloff = 1.0f;       // 衰减系数（1 = 物理正确的平方反比）
};

/**
 * @brief 方向光源参数
 */
struct DirectionalLightParams {
    float3 direction = make_float3(0.0f, -1.0f, 0.0f);
    float3 emission = make_float3(1.0f, 1.0f, 1.0f);
    float angularSize = 0.53f;  // 角直径（度）- 太阳约为 0.53°
};

/**
 * @brief 聚光灯参数
 */
struct SpotLightParams {
    float3 position = make_float3(0.0f, 5.0f, 0.0f);
    float3 direction = make_float3(0.0f, -1.0f, 0.0f);
    float3 emission = make_float3(100.0f, 100.0f, 100.0f);
    float innerAngle = 30.0f;   // 内锥角（度）
    float outerAngle = 45.0f;   // 外锥角（度）
    float radius = 0.1f;
    float falloff = 1.0f;
};

/**
 * @brief 矩形区域光参数
 */
struct AreaRectLightParams {
    float3 position = make_float3(0.0f, 5.0f, 0.0f);
    float3 normal = make_float3(0.0f, -1.0f, 0.0f);
    float3 tangent = make_float3(1.0f, 0.0f, 0.0f);
    float3 emission = make_float3(10.0f, 10.0f, 10.0f);
    float width = 2.0f;
    float height = 2.0f;
    bool doubleSided = false;
};

/**
 * @brief 球形区域光参数
 */
struct AreaSphereLightParams {
    float3 position = make_float3(0.0f, 5.0f, 0.0f);
    float3 emission = make_float3(10.0f, 10.0f, 10.0f);
    float radius = 0.5f;
};

/**
 * @brief 通用光源参数
 */
struct LightParameters {
    LightType type = LightType::POINT;
    int lightID = -1;
    bool enabled = true;

    // 通用参数
    union {
        PointLightParams point;
        DirectionalLightParams directional;
        SpotLightParams spot;
        AreaRectLightParams areaRect;
        AreaSphereLightParams areaSphere;
    };

    // 默认构造函数
    LightParameters() : point{} {}
};

/**
 * @brief 光源管理器
 *
 * 管理场景中的所有光源，并负责将光源数据上传到 GPU
 */
class LightManager {
public:
    /**
     * @brief 构造函数
     */
    LightManager();

    /**
     * @brief 析构函数
     */
    ~LightManager();

    /**
     * @brief 添加点光源
     * @param params 光源参数
     * @return 光源索引
     */
    int addPointLight(const PointLightParams& params);

    /**
     * @brief 添加方向光源
     * @param params 光源参数
     * @return 光源索引
     */
    int addDirectionalLight(const DirectionalLightParams& params);

    /**
     * @brief 添加聚光灯
     * @param params 光源参数
     * @return 光源索引
     */
    int addSpotLight(const SpotLightParams& params);

    /**
     * @brief 添加矩形区域光
     * @param params 光源参数
     * @return 光源索引
     */
    int addAreaRectLight(const AreaRectLightParams& params);

    /**
     * @brief 添加球形区域光
     * @param params 光源参数
     * @return 光源索引
     */
    int addAreaSphereLight(const AreaSphereLightParams& params);

    /**
     * @brief 添加通用光源
     * @param params 光源参数
     * @return 光源索引
     */
    int addLight(const LightParameters& params);

    /**
     * @brief 更新光源参数
     * @param index 光源索引
     * @param params 新的光源参数
     */
    void updateLight(int index, const LightParameters& params);

    /**
     * @brief 移除光源
     * @param index 光源索引
     */
    void removeLight(int index);

    /**
     * @brief 启用/禁用光源
     * @param index 光源索引
     * @param enabled 是否启用
     */
    void setLightEnabled(int index, bool enabled);

    /**
     * @brief 获取光源数量
     */
    int getLightCount() const { return static_cast<int>(lights_.size()); }

    /**
     * @brief 上传光源数据到 GPU
     * @return GPU 设备指针
     */
    CUdeviceptr uploadToGPU();

    /**
     * @brief 获取 GPU 缓冲区指针
     */
    CUdeviceptr getGPUBuffer() const { return d_lightBuffer_; }

    /**
     * @brief 创建简单的三点照明设置
     * @param targetPos 照明目标位置
     * @param distance 光源距离
     */
    void createThreePointLighting(const float3& targetPos, float distance = 5.0f);

    /**
     * @brief 创建环境光照明（来自上方）
     * @param intensity 光照强度
     */
    void createEnvironmentLighting(float intensity = 1.0f);

private:
    std::vector<LightParameters> lights_;
    std::vector<LightDefinition> deviceLights_;

    // GPU 缓冲区
    CUdeviceptr d_lightBuffer_;
    size_t bufferSize_;

    /**
     * @brief 将主机光源参数转换为设备定义
     */
    LightDefinition convertToDeviceLight(const LightParameters& params);
};

} // namespace PhysicsRender

#endif // LIGHT_MANAGER_H
