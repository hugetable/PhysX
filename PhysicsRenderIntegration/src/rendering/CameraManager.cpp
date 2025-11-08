// CameraManager.cpp - 相机管理系统实现

#include "rendering/CameraManager.h"
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

static inline float dot(const float3& a, const float3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// ============================================================================
// CameraManager 实现
// ============================================================================

CameraManager::CameraManager()
    : activeCameraIndex_(0)
    , d_cameraBuffer_(0)
    , bufferSize_(0)
{
}

CameraManager::~CameraManager() {
    if (d_cameraBuffer_) {
        cudaFree(reinterpret_cast<void*>(d_cameraBuffer_));
        d_cameraBuffer_ = 0;
    }
}

int CameraManager::addCamera(const CameraParameters& params) {
    int index = static_cast<int>(cameras_.size());
    cameras_.push_back(params);

    // 如果这是第一个相机，设置为活动相机
    if (cameras_.size() == 1) {
        activeCameraIndex_ = 0;
    }

    std::cout << "Camera " << index << " added (type: "
              << static_cast<int>(params.type) << ")" << std::endl;

    return index;
}

void CameraManager::updateCamera(int index, const CameraParameters& params) {
    if (index >= 0 && index < static_cast<int>(cameras_.size())) {
        cameras_[index] = params;
    }
}

void CameraManager::removeCamera(int index) {
    if (index >= 0 && index < static_cast<int>(cameras_.size())) {
        cameras_.erase(cameras_.begin() + index);

        // 调整活动相机索引
        if (activeCameraIndex_ >= static_cast<int>(cameras_.size())) {
            activeCameraIndex_ = std::max(0, static_cast<int>(cameras_.size()) - 1);
        }
    }
}

void CameraManager::setActiveCamera(int index) {
    if (index >= 0 && index < static_cast<int>(cameras_.size())) {
        activeCameraIndex_ = index;
        std::cout << "Active camera set to: " << index << std::endl;
    }
}

void CameraManager::computeCameraFrame(
    const float3& position,
    const float3& lookAt,
    const float3& up,
    float3& u,
    float3& v,
    float3& w
) {
    // w = lookAt 方向的反向（右手坐标系）
    w = normalize(make_float3(
        position.x - lookAt.x,
        position.y - lookAt.y,
        position.z - lookAt.z
    ));

    // u = up × w (右向量)
    u = normalize(cross(up, w));

    // v = w × u (上向量)
    v = cross(w, u);
}

CameraDefinition CameraManager::convertToDeviceCamera(const CameraParameters& params) {
    CameraDefinition devCam;
    std::memset(&devCam, 0, sizeof(CameraDefinition));

    // 计算相机坐标系
    float3 u, v, w;
    computeCameraFrame(params.position, params.lookAt, params.up, u, v, w);

    // 设置相机位置
    devCam.cameraPosition = params.position;

    // 计算图像平面尺寸
    float vfov = radians(params.fov);
    float tanVfov = tanf(vfov * 0.5f);

    // 相机向量（OptiX 约定）
    // U: 指向右侧，长度 = 图像平面宽度的一半
    // V: 指向上方，长度 = 图像平面高度的一半
    // W: 指向相机，长度 = 焦距

    float halfHeight = tanVfov;
    float halfWidth = halfHeight * params.aspectRatio;

    devCam.cameraU = make_float3(u.x * halfWidth, u.y * halfWidth, u.z * halfWidth);
    devCam.cameraV = make_float3(v.x * halfHeight, v.y * halfHeight, v.z * halfHeight);
    devCam.cameraW = make_float3(-w.x, -w.y, -w.z);  // 负号：指向观察方向

    // 薄透镜参数
    devCam.lensRadius = params.aperture * 0.5f;
    devCam.focalDistance = params.focalDistance;

    return devCam;
}

CUdeviceptr CameraManager::uploadToGPU() {
    if (cameras_.empty()) {
        std::cerr << "Warning: No cameras to upload" << std::endl;
        return 0;
    }

    // 转换所有相机到设备格式
    deviceCameras_.clear();
    deviceCameras_.reserve(cameras_.size());

    for (const auto& cam : cameras_) {
        deviceCameras_.push_back(convertToDeviceCamera(cam));
    }

    // 计算所需缓冲区大小
    size_t requiredSize = deviceCameras_.size() * sizeof(CameraDefinition);

    // 如果缓冲区不够大，重新分配
    if (bufferSize_ < requiredSize) {
        if (d_cameraBuffer_) {
            cudaFree(reinterpret_cast<void*>(d_cameraBuffer_));
        }

        cudaError_t err = cudaMalloc(
            reinterpret_cast<void**>(&d_cameraBuffer_),
            requiredSize
        );

        if (err != cudaSuccess) {
            std::cerr << "Failed to allocate camera buffer: "
                      << cudaGetErrorString(err) << std::endl;
            d_cameraBuffer_ = 0;
            bufferSize_ = 0;
            return 0;
        }

        bufferSize_ = requiredSize;
    }

    // 上传数据
    cudaError_t err = cudaMemcpy(
        reinterpret_cast<void*>(d_cameraBuffer_),
        deviceCameras_.data(),
        requiredSize,
        cudaMemcpyHostToDevice
    );

    if (err != cudaSuccess) {
        std::cerr << "Failed to upload camera data: "
                  << cudaGetErrorString(err) << std::endl;
        return 0;
    }

    std::cout << "Uploaded " << deviceCameras_.size()
              << " cameras to GPU (" << requiredSize << " bytes)" << std::endl;

    return d_cameraBuffer_;
}

int CameraManager::createFromLookAt(
    const float3& position,
    const float3& target,
    float fov,
    float aperture
) {
    CameraParameters params;
    params.type = (aperture > 0.0f) ? CameraType::THINLENS : CameraType::PINHOLE;
    params.position = position;
    params.lookAt = target;
    params.fov = fov;
    params.aperture = aperture;

    // 计算焦距（从位置到目标的距离）
    float3 dir = make_float3(
        target.x - position.x,
        target.y - position.y,
        target.z - position.z
    );
    params.focalDistance = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

    return addCamera(params);
}

int CameraManager::createOrbitCamera(
    const float3& center,
    float radius,
    float elevation,
    float azimuth
) {
    // 球坐标到笛卡尔坐标
    float elevRad = radians(elevation);
    float azimRad = radians(azimuth);

    float3 position = make_float3(
        center.x + radius * cosf(elevRad) * sinf(azimRad),
        center.y + radius * sinf(elevRad),
        center.z + radius * cosf(elevRad) * cosf(azimRad)
    );

    CameraParameters params;
    params.type = CameraType::PINHOLE;
    params.position = position;
    params.lookAt = center;
    params.fov = 60.0f;

    return addCamera(params);
}

void CameraManager::updateOrbitCamera(int index, float elevation, float azimuth) {
    if (index < 0 || index >= static_cast<int>(cameras_.size())) {
        return;
    }

    auto& cam = cameras_[index];
    float3 center = cam.lookAt;

    // 计算当前半径
    float3 dir = make_float3(
        cam.position.x - center.x,
        cam.position.y - center.y,
        cam.position.z - center.z
    );
    float radius = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

    // 更新位置
    float elevRad = radians(elevation);
    float azimRad = radians(azimuth);

    cam.position = make_float3(
        center.x + radius * cosf(elevRad) * sinf(azimRad),
        center.y + radius * sinf(elevRad),
        center.z + radius * cosf(elevRad) * cosf(azimRad)
    );
}

} // namespace PhysicsRender
