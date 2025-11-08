// DenoiserManager.h - OptiX Denoiser 集成
// AI 降噪器，用于减少路径追踪噪声

#ifndef DENOISER_MANAGER_H
#define DENOISER_MANAGER_H

#include <optix.h>
#include <optix_denoiser_tiling.h>
#include <cuda.h>

namespace PhysicsRender {

/**
 * @brief 降噪器模式
 */
enum class DenoiserMode {
    RGB,                // 仅 RGB
    RGB_ALBEDO,         // RGB + Albedo（反照率）
    RGB_ALBEDO_NORMAL   // RGB + Albedo + Normal（最高质量）
};

/**
 * @brief 降噪器参数
 */
struct DenoiserParams {
    DenoiserMode mode = DenoiserMode::RGB_ALBEDO_NORMAL;
    bool enableKernelPrediction = true;     // 启用内核预测（更好的质量）
    bool enableTemporalMode = false;        // 时序降噪（动画）
    float blendFactor = 0.0f;               // 时序混合因子 [0,1]
    unsigned int tileWidth = 0;             // 分块宽度（0=自动）
    unsigned int tileHeight = 0;            // 分块高度（0=自动）
};

/**
 * @brief OptiX Denoiser 管理器
 *
 * 集成 NVIDIA OptiX AI Denoiser，用于实时路径追踪降噪
 */
class DenoiserManager {
public:
    /**
     * @brief 构造函数
     * @param width 图像宽度
     * @param height 图像高度
     */
    DenoiserManager(unsigned int width, unsigned int height);

    /**
     * @brief 析构函数
     */
    ~DenoiserManager();

    /**
     * @brief 初始化降噪器
     * @param params 降噪器参数
     * @return 是否成功
     */
    bool initialize(const DenoiserParams& params);

    /**
     * @brief 执行降噪
     * @param inputRGB 输入 RGB 缓冲区
     * @param inputAlbedo 输入 Albedo 缓冲区（可选）
     * @param inputNormal 输入 Normal 缓冲区（可选）
     * @param outputRGB 输出 RGB 缓冲区
     * @param stream CUDA 流
     * @return 是否成功
     */
    bool denoise(
        CUdeviceptr inputRGB,
        CUdeviceptr inputAlbedo,
        CUdeviceptr inputNormal,
        CUdeviceptr outputRGB,
        CUstream stream = 0
    );

    /**
     * @brief 执行降噪（仅 RGB）
     * @param inputRGB 输入 RGB 缓冲区
     * @param outputRGB 输出 RGB 缓冲区
     * @param stream CUDA 流
     * @return 是否成功
     */
    bool denoiseRGB(
        CUdeviceptr inputRGB,
        CUdeviceptr outputRGB,
        CUstream stream = 0
    );

    /**
     * @brief 调整大小
     * @param width 新宽度
     * @param height 新高度
     * @return 是否成功
     */
    bool resize(unsigned int width, unsigned int height);

    /**
     * @brief 更新参数
     * @param params 新参数
     * @return 是否成功
     */
    bool updateParams(const DenoiserParams& params);

    /**
     * @brief 设置时序混合因子
     * @param alpha 混合因子 [0,1]，0=完全使用当前帧，1=完全使用历史
     */
    void setTemporalBlend(float alpha);

    /**
     * @brief 获取降噪器状态
     */
    bool isInitialized() const { return initialized_; }

    /**
     * @brief 获取图像尺寸
     */
    void getSize(unsigned int& width, unsigned int& height) const {
        width = width_;
        height = height_;
    }

    /**
     * @brief 获取降噪器内存使用
     */
    struct MemoryUsage {
        size_t stateSize;           // 状态缓冲区大小
        size_t scratchSize;         // 临时缓冲区大小
        size_t internalSize;        // 内部缓冲区大小
        size_t totalSize;           // 总大小
    };

    MemoryUsage getMemoryUsage() const { return memoryUsage_; }

    /**
     * @brief 启用/禁用降噪器
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

private:
    // OptiX Denoiser
    OptixDenoiser denoiser_;
    OptixDenoiserOptions denoiserOptions_;
    OptixDenoiserParams denoiserParams_;

    // 图像尺寸
    unsigned int width_;
    unsigned int height_;

    // 缓冲区
    CUdeviceptr d_state_;           // 降噪器状态
    CUdeviceptr d_scratch_;         // 临时缓冲区
    CUdeviceptr d_intensity_;       // 强度值（HDR）

    size_t stateSize_;
    size_t scratchSize_;

    // 时序降噪（可选）
    CUdeviceptr d_prevOutput_;      // 上一帧输出
    bool temporalModeEnabled_;

    // 分块降噪（大分辨率）
    bool tiled_;
    std::vector<OptixUtilDenoiserImageTile> tiles_;

    // 状态
    bool initialized_;
    bool enabled_;
    DenoiserParams params_;
    MemoryUsage memoryUsage_;

    /**
     * @brief 设置降噪器层
     */
    void setupLayers(
        OptixDenoiserLayer& layer,
        CUdeviceptr inputRGB,
        CUdeviceptr inputAlbedo,
        CUdeviceptr inputNormal,
        CUdeviceptr outputRGB
    );

    /**
     * @brief 计算强度（用于 HDR）
     */
    void computeIntensity(CUdeviceptr inputRGB, CUstream stream);

    /**
     * @brief 设置分块
     */
    void setupTiling();

    /**
     * @brief 清理资源
     */
    void cleanup();
};

} // namespace PhysicsRender

#endif // DENOISER_MANAGER_H
