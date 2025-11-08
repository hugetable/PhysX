// LightManager.cpp - 光源管理系统实现

#include "rendering/LightManager.h"
#include "config.h"
#include <cmath>
#include <iostream>
#include <cstring>

namespace PhysicsRender {

// ============================================================================
// 辅助函数
// ============================================================================

static inline float radians(float degrees) {
    return degrees * M_PI / 180.0f;
}

static inline float3 normalize(const float3& v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 0.0f) {
        return make_float3(v.x / len, v.y / len, v.z / len);
    }
    return make_float3(0.0f, 0.0f, 1.0f);
}

static inline float3 cross(const float3& a, const float3& b) {
    return make_float3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

// ============================================================================
// LightManager 实现
// ============================================================================

LightManager::LightManager()
    : d_lightBuffer_(0)
    , bufferSize_(0)
{
}

LightManager::~LightManager() {
    if (d_lightBuffer_) {
        cudaFree(reinterpret_cast<void*>(d_lightBuffer_));
        d_lightBuffer_ = 0;
    }
}

int LightManager::addPointLight(const PointLightParams& params) {
    LightParameters light;
    light.type = LightType::POINT;
    light.point = params;
    return addLight(light);
}

int LightManager::addDirectionalLight(const DirectionalLightParams& params) {
    LightParameters light;
    light.type = LightType::DIRECTIONAL;
    light.directional = params;
    return addLight(light);
}

int LightManager::addSpotLight(const SpotLightParams& params) {
    LightParameters light;
    light.type = LightType::SPOT;
    light.spot = params;
    return addLight(light);
}

int LightManager::addAreaRectLight(const AreaRectLightParams& params) {
    LightParameters light;
    light.type = LightType::AREA_RECT;
    light.areaRect = params;
    return addLight(light);
}

int LightManager::addAreaSphereLight(const AreaSphereLightParams& params) {
    LightParameters light;
    light.type = LightType::AREA_SPHERE;
    light.areaSphere = params;
    return addLight(light);
}

int LightManager::addLight(const LightParameters& params) {
    int index = static_cast<int>(lights_.size());
    LightParameters light = params;
    light.lightID = index;
    lights_.push_back(light);

    std::cout << "Light " << index << " added (type: "
              << static_cast<int>(light.type) << ")" << std::endl;

    return index;
}

void LightManager::updateLight(int index, const LightParameters& params) {
    if (index >= 0 && index < static_cast<int>(lights_.size())) {
        lights_[index] = params;
        lights_[index].lightID = index;
    }
}

void LightManager::removeLight(int index) {
    if (index >= 0 && index < static_cast<int>(lights_.size())) {
        lights_.erase(lights_.begin() + index);

        // 重新分配 light IDs
        for (int i = 0; i < static_cast<int>(lights_.size()); ++i) {
            lights_[i].lightID = i;
        }
    }
}

void LightManager::setLightEnabled(int index, bool enabled) {
    if (index >= 0 && index < static_cast<int>(lights_.size())) {
        lights_[index].enabled = enabled;
    }
}

LightDefinition LightManager::convertToDeviceLight(const LightParameters& params) {
    LightDefinition devLight;
    std::memset(&devLight, 0, sizeof(LightDefinition));

    devLight.type = static_cast<int>(params.type);

    switch (params.type) {
    case LightType::POINT: {
        devLight.position = params.point.position;
        devLight.emission = params.point.emission;
        devLight.radius = params.point.radius;

        // 计算衰减参数（物理正确的平方反比）
        devLight.falloff = params.point.falloff;
        break;
    }

    case LightType::DIRECTIONAL: {
        devLight.direction = normalize(params.directional.direction);
        devLight.emission = params.directional.emission;
        devLight.angularSize = radians(params.directional.angularSize);
        break;
    }

    case LightType::SPOT: {
        devLight.position = params.spot.position;
        devLight.direction = normalize(params.spot.direction);
        devLight.emission = params.spot.emission;
        devLight.radius = params.spot.radius;
        devLight.falloff = params.spot.falloff;

        // 聚光灯锥角
        devLight.innerAngle = cosf(radians(params.spot.innerAngle));
        devLight.outerAngle = cosf(radians(params.spot.outerAngle));
        break;
    }

    case LightType::AREA_RECT: {
        devLight.position = params.areaRect.position;
        devLight.normal = normalize(params.areaRect.normal);
        devLight.emission = params.areaRect.emission;

        // 计算切线和副切线
        float3 tangent = normalize(params.areaRect.tangent);
        float3 bitangent = cross(devLight.normal, tangent);

        // 矩形边向量
        devLight.u = make_float3(
            tangent.x * params.areaRect.width * 0.5f,
            tangent.y * params.areaRect.width * 0.5f,
            tangent.z * params.areaRect.width * 0.5f
        );
        devLight.v = make_float3(
            bitangent.x * params.areaRect.height * 0.5f,
            bitangent.y * params.areaRect.height * 0.5f,
            bitangent.z * params.areaRect.height * 0.5f
        );

        // 面积（用于重要性采样）
        devLight.area = params.areaRect.width * params.areaRect.height;
        devLight.doubleSided = params.areaRect.doubleSided ? 1 : 0;
        break;
    }

    case LightType::AREA_SPHERE: {
        devLight.position = params.areaSphere.position;
        devLight.emission = params.areaSphere.emission;
        devLight.radius = params.areaSphere.radius;
        devLight.area = 4.0f * M_PI * params.areaSphere.radius * params.areaSphere.radius;
        break;
    }

    default:
        break;
    }

    return devLight;
}

CUdeviceptr LightManager::uploadToGPU() {
    if (lights_.empty()) {
        std::cout << "Warning: No lights to upload" << std::endl;
        return 0;
    }

    // 转换所有光源到设备格式
    deviceLights_.clear();
    deviceLights_.reserve(lights_.size());

    for (const auto& light : lights_) {
        if (light.enabled) {
            deviceLights_.push_back(convertToDeviceLight(light));
        }
    }

    if (deviceLights_.empty()) {
        std::cout << "Warning: No enabled lights to upload" << std::endl;
        return 0;
    }

    // 计算所需缓冲区大小
    size_t requiredSize = deviceLights_.size() * sizeof(LightDefinition);

    // 如果缓冲区不够大，重新分配
    if (bufferSize_ < requiredSize) {
        if (d_lightBuffer_) {
            cudaFree(reinterpret_cast<void*>(d_lightBuffer_));
        }

        cudaError_t err = cudaMalloc(
            reinterpret_cast<void**>(&d_lightBuffer_),
            requiredSize
        );

        if (err != cudaSuccess) {
            std::cerr << "Failed to allocate light buffer: "
                      << cudaGetErrorString(err) << std::endl;
            d_lightBuffer_ = 0;
            bufferSize_ = 0;
            return 0;
        }

        bufferSize_ = requiredSize;
    }

    // 上传数据
    cudaError_t err = cudaMemcpy(
        reinterpret_cast<void*>(d_lightBuffer_),
        deviceLights_.data(),
        requiredSize,
        cudaMemcpyHostToDevice
    );

    if (err != cudaSuccess) {
        std::cerr << "Failed to upload light data: "
                  << cudaGetErrorString(err) << std::endl;
        return 0;
    }

    std::cout << "Uploaded " << deviceLights_.size()
              << " lights to GPU (" << requiredSize << " bytes)" << std::endl;

    return d_lightBuffer_;
}

void LightManager::createThreePointLighting(const float3& targetPos, float distance) {
    // 主光源（Key Light）- 明亮，45度角
    PointLightParams keyLight;
    keyLight.position = make_float3(
        targetPos.x + distance * 0.707f,
        targetPos.y + distance * 0.5f,
        targetPos.z + distance * 0.707f
    );
    keyLight.emission = make_float3(80.0f, 80.0f, 80.0f);
    keyLight.radius = 0.5f;
    addPointLight(keyLight);

    // 补光（Fill Light）- 柔和，侧面
    PointLightParams fillLight;
    fillLight.position = make_float3(
        targetPos.x - distance * 0.5f,
        targetPos.y + distance * 0.3f,
        targetPos.z + distance * 0.5f
    );
    fillLight.emission = make_float3(30.0f, 30.0f, 35.0f);  // 稍微偏蓝
    fillLight.radius = 1.0f;
    addPointLight(fillLight);

    // 轮廓光（Rim Light）- 背后，勾勒轮廓
    PointLightParams rimLight;
    rimLight.position = make_float3(
        targetPos.x,
        targetPos.y + distance * 0.8f,
        targetPos.z - distance * 0.8f
    );
    rimLight.emission = make_float3(50.0f, 50.0f, 45.0f);  // 稍微偏暖
    rimLight.radius = 0.3f;
    addPointLight(rimLight);

    std::cout << "Created three-point lighting setup" << std::endl;
}

void LightManager::createEnvironmentLighting(float intensity) {
    // 顶部环境光（模拟天空）
    DirectionalLightParams skyLight;
    skyLight.direction = make_float3(0.0f, -1.0f, 0.0f);
    skyLight.emission = make_float3(intensity, intensity, intensity * 1.1f);  // 稍微偏蓝
    skyLight.angularSize = 90.0f;  // 大面积光源
    addDirectionalLight(skyLight);

    // 底部反射光（模拟地面反射）
    DirectionalLightParams groundLight;
    groundLight.direction = make_float3(0.0f, 1.0f, 0.0f);
    groundLight.emission = make_float3(
        intensity * 0.3f,
        intensity * 0.3f,
        intensity * 0.25f
    );  // 较暗，稍微偏暖
    groundLight.angularSize = 120.0f;
    addDirectionalLight(groundLight);

    std::cout << "Created environment lighting setup" << std::endl;
}

} // namespace PhysicsRender
