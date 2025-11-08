// VolumeManager.h - 体积渲染系统
// 支持雾、烟、云等参与介质

#ifndef VOLUME_MANAGER_H
#define VOLUME_MANAGER_H

#include <cuda.h>
#include <optix.h>
#include <vector>

namespace PhysicsRender {

/**
 * @brief 体积类型
 */
enum class VolumeType {
    HOMOGENEOUS,        // 均匀介质
    HETEROGENEOUS,      // 非均匀介质
    PROCEDURAL_NOISE,   // 程序化噪声（Perlin/Simplex）
    GRID_BASED          // 基于网格（VDB）
};

/**
 * @brief 相位函数类型
 */
enum class PhaseFunctionType {
    ISOTROPIC,          // 各向同性
    RAYLEIGH,           // Rayleigh 散射（天空）
    MIE,                // Mie 散射（雾）
    HENYEY_GREENSTEIN   // Henyey-Greenstein（通用）
};

/**
 * @brief 体积参数
 */
struct VolumeParams {
    VolumeType type = VolumeType::HOMOGENEOUS;

    // 散射和吸收系数
    float3 scattering = make_float3(0.5f, 0.5f, 0.5f);  // σ_s
    float3 absorption = make_float3(0.1f, 0.1f, 0.1f);  // σ_a
    float3 emission = make_float3(0.0f, 0.0f, 0.0f);    // 发光

    // 相位函数
    PhaseFunctionType phaseFunction = PhaseFunctionType::HENYEY_GREENSTEIN;
    float g = 0.0f;  // 各向异性参数 [-1,1]，0=各向同性

    // 密度
    float density = 1.0f;           // 全局密度倍增器
    float densityScale = 1.0f;      // 密度缩放

    // 边界
    float3 boundsMin = make_float3(-10.0f, -10.0f, -10.0f);
    float3 boundsMax = make_float3(10.0f, 10.0f, 10.0f);

    // 程序化噪声参数
    int noiseOctaves = 4;
    float noiseLacunarity = 2.0f;
    float noiseGain = 0.5f;
    float noiseScale = 1.0f;
    float3 noiseOffset = make_float3(0.0f, 0.0f, 0.0f);

    // 网格数据（可选）
    CUdeviceptr gridData = 0;
    unsigned int gridResX = 0;
    unsigned int gridResY = 0;
    unsigned int gridResZ = 0;

    int volumeID = -1;
};

/**
 * @brief 体积散射事件
 */
struct VolumeScatterEvent {
    float3 position;        // 散射位置
    float3 direction;       // 新方向
    float3 throughput;      // 吞吐量
    float distance;         // 自由程距离
    bool absorbed;          // 是否被吸收
};

/**
 * @brief 体积管理器
 *
 * 管理场景中的所有体积（雾、烟、云等参与介质）
 */
class VolumeManager {
public:
    /**
     * @brief 构造函数
     */
    VolumeManager();

    /**
     * @brief 析构函数
     */
    ~VolumeManager();

    /**
     * @brief 添加均匀体积
     * @param scattering 散射系数
     * @param absorption 吸收系数
     * @param boundsMin 边界最小值
     * @param boundsMax 边界最大值
     * @return 体积索引
     */
    int addHomogeneousVolume(
        const float3& scattering,
        const float3& absorption,
        const float3& boundsMin,
        const float3& boundsMax
    );

    /**
     * @brief 添加指数雾（密度随高度指数衰减）
     * @param density 基础密度
     * @param falloff 衰减系数
     * @param baseHeight 基础高度
     * @param color 雾的颜色
     * @return 体积索引
     */
    int addExponentialFog(
        float density,
        float falloff,
        float baseHeight,
        const float3& color
    );

    /**
     * @brief 添加程序化云
     * @param boundsMin 边界最小值
     * @param boundsMax 边界最大值
     * @param density 云密度
     * @param noiseScale 噪声缩放
     * @return 体积索引
     */
    int addProceduralClouds(
        const float3& boundsMin,
        const float3& boundsMax,
        float density,
        float noiseScale
    );

    /**
     * @brief 添加自定义体积
     * @param params 体积参数
     * @return 体积索引
     */
    int addVolume(const VolumeParams& params);

    /**
     * @brief 更新体积
     * @param index 体积索引
     * @param params 新参数
     */
    void updateVolume(int index, const VolumeParams& params);

    /**
     * @brief 移除体积
     * @param index 体积索引
     */
    void removeVolume(int index);

    /**
     * @brief 获取体积数量
     */
    int getVolumeCount() const { return static_cast<int>(volumes_.size()); }

    /**
     * @brief 上传体积数据到 GPU
     * @return GPU 设备指针
     */
    CUdeviceptr uploadToGPU();

    /**
     * @brief 获取 GPU 缓冲区指针
     */
    CUdeviceptr getGPUBuffer() const { return d_volumeBuffer_; }

    /**
     * @brief 加载 OpenVDB 文件
     * @param filepath VDB 文件路径
     * @param gridName 网格名称
     * @return 体积索引，失败返回 -1
     */
    int loadVDB(const char* filepath, const char* gridName);

    /**
     * @brief 设置全局体积密度倍增器
     */
    void setGlobalDensityMultiplier(float multiplier) {
        globalDensityMultiplier_ = multiplier;
    }

private:
    std::vector<VolumeParams> volumes_;

    // GPU 缓冲区
    CUdeviceptr d_volumeBuffer_;
    size_t bufferSize_;

    // 全局参数
    float globalDensityMultiplier_;

    /**
     * @brief 生成 3D 噪声纹理
     */
    cudaTextureObject_t generateNoiseTexture(
        unsigned int resolution,
        int octaves,
        float lacunarity,
        float gain
    );
};

/**
 * @brief 体积采样和散射函数（设备端）
 */
namespace VolumeSampling {

/**
 * @brief 采样自由程距离（使用 Woodcock tracking）
 * @param rayOrigin 光线起点
 * @param rayDirection 光线方向
 * @param tMin 最小 t
 * @param tMax 最大 t
 * @param volume 体积参数
 * @param seed 随机数种子
 * @return 采样距离，-1 表示没有散射
 */
__device__ float sampleFreePathDistance(
    const float3& rayOrigin,
    const float3& rayDirection,
    float tMin,
    float tMax,
    const VolumeParams& volume,
    unsigned int& seed
);

/**
 * @brief 评估体积密度（在给定位置）
 * @param position 位置
 * @param volume 体积参数
 * @return 密度值
 */
__device__ float evaluateVolumeDensity(
    const float3& position,
    const VolumeParams& volume
);

/**
 * @brief Henyey-Greenstein 相位函数
 * @param cosTheta cos(散射角)
 * @param g 各向异性参数 [-1,1]
 * @return 相位函数值
 */
__device__ __forceinline__ float phaseHenyeyGreenstein(float cosTheta, float g) {
    float denom = 1.0f + g * g + 2.0f * g * cosTheta;
    return (1.0f - g * g) / (4.0f * M_PI * denom * sqrtf(denom));
}

/**
 * @brief 采样 Henyey-Greenstein 相位函数
 * @param wo 出射方向
 * @param g 各向异性参数
 * @param xi 随机数 [0,1)
 * @return 新方向
 */
__device__ float3 samplePhaseHenyeyGreenstein(
    const float3& wo,
    float g,
    float xi
);

/**
 * @brief 各向同性相位函数
 */
__device__ __forceinline__ float phaseIsotropic() {
    return 1.0f / (4.0f * M_PI);
}

/**
 * @brief 采样各向同性相位函数
 * @param xi 随机数 [0,1)^2
 * @return 方向
 */
__device__ float3 samplePhaseIsotropic(const float2& xi);

/**
 * @brief 计算透射率（Beer-Lambert 定律）
 * @param extinction 消光系数（scattering + absorption）
 * @param distance 距离
 * @return 透射率 [0,1]
 */
__device__ __forceinline__ float3 transmittance(
    const float3& extinction,
    float distance
) {
    return make_float3(
        expf(-extinction.x * distance),
        expf(-extinction.y * distance),
        expf(-extinction.z * distance)
    );
}

/**
 * @brief 3D Perlin 噪声
 */
__device__ float perlinNoise3D(const float3& p);

/**
 * @brief 分形布朗运动（FBM）
 */
__device__ float fbm3D(
    const float3& p,
    int octaves,
    float lacunarity,
    float gain
);

} // namespace VolumeSampling

} // namespace PhysicsRender

#endif // VOLUME_MANAGER_H
