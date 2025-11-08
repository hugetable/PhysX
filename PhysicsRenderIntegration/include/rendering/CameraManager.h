// CameraManager.h - 相机管理系统
// 负责相机参数管理和 GPU 数据上传

#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <cuda.h>
#include <vector>
#include <memory>
#include "camera_definition.h"

namespace PhysicsRender {

/**
 * @brief 相机类型
 */
enum class CameraType {
    PINHOLE,        // 针孔相机
    THINLENS,       // 薄透镜相机（景深效果）
    FISHEYE,        // 鱼眼相机
    SPHERICAL       // 球形全景相机
};

/**
 * @brief 相机参数（主机端）
 */
struct CameraParameters {
    CameraType type = CameraType::PINHOLE;

    // 位置和方向
    float3 position = make_float3(0.0f, 0.0f, 5.0f);
    float3 lookAt = make_float3(0.0f, 0.0f, 0.0f);
    float3 up = make_float3(0.0f, 1.0f, 0.0f);

    // 视角参数
    float fov = 60.0f;              // 垂直视场角（度）
    float aspectRatio = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 10000.0f;

    // 薄透镜参数（景深）
    float aperture = 0.0f;          // 光圈大小（0 = 针孔）
    float focalDistance = 1.0f;     // 对焦距离

    // 运动模糊
    float shutterOpen = 0.0f;
    float shutterClose = 1.0f;

    // 相机 ID
    int cameraID = 0;
};

/**
 * @brief 相机管理器
 *
 * 管理场景中的所有相机，并负责将相机数据上传到 GPU
 */
class CameraManager {
public:
    /**
     * @brief 构造函数
     */
    CameraManager();

    /**
     * @brief 析构函数
     */
    ~CameraManager();

    /**
     * @brief 添加相机
     * @param params 相机参数
     * @return 相机索引
     */
    int addCamera(const CameraParameters& params);

    /**
     * @brief 更新相机参数
     * @param index 相机索引
     * @param params 新的相机参数
     */
    void updateCamera(int index, const CameraParameters& params);

    /**
     * @brief 移除相机
     * @param index 相机索引
     */
    void removeCamera(int index);

    /**
     * @brief 设置活动相机
     * @param index 相机索引
     */
    void setActiveCamera(int index);

    /**
     * @brief 获取活动相机索引
     */
    int getActiveCamera() const { return activeCameraIndex_; }

    /**
     * @brief 获取相机数量
     */
    int getCameraCount() const { return static_cast<int>(cameras_.size()); }

    /**
     * @brief 上传相机数据到 GPU
     * @return GPU 设备指针
     */
    CUdeviceptr uploadToGPU();

    /**
     * @brief 获取 GPU 缓冲区指针
     */
    CUdeviceptr getGPUBuffer() const { return d_cameraBuffer_; }

    /**
     * @brief 从位置和目标点创建相机
     * @param position 相机位置
     * @param target 观察目标
     * @param fov 视场角
     * @return 相机索引
     */
    int createFromLookAt(
        const float3& position,
        const float3& target,
        float fov = 60.0f,
        float aperture = 0.0f
    );

    /**
     * @brief 创建轨道相机
     * @param center 轨道中心
     * @param radius 轨道半径
     * @param elevation 仰角（度）
     * @param azimuth 方位角（度）
     * @return 相机索引
     */
    int createOrbitCamera(
        const float3& center,
        float radius,
        float elevation,
        float azimuth
    );

    /**
     * @brief 更新轨道相机
     * @param index 相机索引
     * @param elevation 仰角（度）
     * @param azimuth 方位角（度）
     */
    void updateOrbitCamera(int index, float elevation, float azimuth);

private:
    std::vector<CameraParameters> cameras_;
    std::vector<CameraDefinition> deviceCameras_;
    int activeCameraIndex_;

    // GPU 缓冲区
    CUdeviceptr d_cameraBuffer_;
    size_t bufferSize_;

    /**
     * @brief 将主机相机参数转换为设备定义
     */
    CameraDefinition convertToDeviceCamera(const CameraParameters& params);

    /**
     * @brief 计算相机坐标系
     */
    void computeCameraFrame(
        const float3& position,
        const float3& lookAt,
        const float3& up,
        float3& u,
        float3& v,
        float3& w
    );
};

} // namespace PhysicsRender

#endif // CAMERA_MANAGER_H
