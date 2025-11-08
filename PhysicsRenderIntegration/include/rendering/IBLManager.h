// IBLManager.h - Image-Based Lighting 管理系统
// 环境贴图采样和重要性采样

#ifndef IBL_MANAGER_H
#define IBL_MANAGER_H

#include <cuda.h>
#include <optix.h>
#include <vector>
#include <string>

namespace PhysicsRender {

/**
 * @brief 环境贴图类型
 */
enum class EnvironmentMapType {
    EQUIRECTANGULAR,    // 等距柱状投影（全景图）
    CUBE_MAP,           // 立方体贴图
    LATITUDE_LONGITUDE  // 经纬度贴图
};

/**
 * @brief 环境贴图参数
 */
struct EnvironmentMapParams {
    EnvironmentMapType type = EnvironmentMapType::EQUIRECTANGULAR;
    std::string filepath;
    float rotation = 0.0f;          // 旋转角度（度）
    float intensity = 1.0f;         // 强度倍增器
    float3 tint = make_float3(1.0f, 1.0f, 1.0f);  // 色调
    bool enableImportanceSampling = true;
};

/**
 * @brief IBL 采样数据（设备端）
 */
struct IBLSamplingData {
    cudaTextureObject_t envTexture;     // 环境贴图纹理
    float* cdf_u;                       // 累积分布函数（U 方向）
    float* cdf_v;                       // 累积分布函数（V 方向）
    float* marginalCDF;                 // 边缘 CDF
    unsigned int width;                 // 贴图宽度
    unsigned int height;                // 贴图高度
    float integral;                     // 总积分（归一化常数）
    float rotation;                     // 旋转（弧度）
    float intensity;                    // 强度
    float3 tint;                        // 色调
};

/**
 * @brief IBL 管理器
 *
 * 管理环境光照，包括环境贴图加载、重要性采样 CDF 生成
 */
class IBLManager {
public:
    /**
     * @brief 构造函数
     */
    IBLManager();

    /**
     * @brief 析构函数
     */
    ~IBLManager();

    /**
     * @brief 加载环境贴图
     * @param params 环境贴图参数
     * @return 是否成功
     */
    bool loadEnvironmentMap(const EnvironmentMapParams& params);

    /**
     * @brief 从 HDR 图像创建环境贴图
     * @param filepath HDR 图像路径（.hdr, .exr）
     * @param rotation 旋转角度（度）
     * @param intensity 强度倍增器
     * @return 是否成功
     */
    bool loadHDREnvironment(
        const char* filepath,
        float rotation = 0.0f,
        float intensity = 1.0f
    );

    /**
     * @brief 创建程序化天空
     * @param sunDirection 太阳方向
     * @param turbidity 混浊度（1-10，1=清澈，10=雾霾）
     * @param groundAlbedo 地面反照率
     * @return 是否成功
     */
    bool createProceduralSky(
        const float3& sunDirection,
        float turbidity = 2.0f,
        float groundAlbedo = 0.3f
    );

    /**
     * @brief 创建常量环境光
     * @param color 环境光颜色
     * @param intensity 强度
     */
    void createConstantEnvironment(const float3& color, float intensity = 1.0f);

    /**
     * @brief 更新环境贴图参数
     * @param rotation 旋转（度）
     * @param intensity 强度
     * @param tint 色调
     */
    void updateParameters(float rotation, float intensity, const float3& tint);

    /**
     * @brief 生成重要性采样 CDF
     * @param enableCPU 是否在 CPU 上生成（false = GPU）
     * @return 是否成功
     */
    bool generateImportanceSamplingCDF(bool enableCPU = true);

    /**
     * @brief 上传 IBL 数据到 GPU
     * @return 设备端 IBLSamplingData 指针
     */
    CUdeviceptr uploadToGPU();

    /**
     * @brief 获取 GPU 缓冲区指针
     */
    CUdeviceptr getGPUBuffer() const { return d_iblData_; }

    /**
     * @brief 是否已加载环境贴图
     */
    bool isLoaded() const { return envTextureLoaded_; }

    /**
     * @brief 获取环境贴图尺寸
     */
    void getSize(unsigned int& width, unsigned int& height) const {
        width = width_;
        height = height_;
    }

    /**
     * @brief 保存预计算的 CDF 到文件
     * @param filepath 文件路径
     */
    bool saveCDF(const char* filepath) const;

    /**
     * @brief 从文件加载预计算的 CDF
     * @param filepath 文件路径
     */
    bool loadCDF(const char* filepath);

private:
    // 环境贴图数据
    cudaTextureObject_t envTexture_;
    cudaArray_t envArray_;
    unsigned int width_;
    unsigned int height_;
    bool envTextureLoaded_;

    // 重要性采样数据
    float* h_cdf_u_;            // 主机端 CDF (U 方向)
    float* h_cdf_v_;            // 主机端 CDF (V 方向)
    float* h_marginalCDF_;      // 主机端边缘 CDF

    CUdeviceptr d_cdf_u_;       // 设备端 CDF
    CUdeviceptr d_cdf_v_;
    CUdeviceptr d_marginalCDF_;

    float integral_;            // 总积分
    bool cdfGenerated_;

    // 参数
    EnvironmentMapParams params_;

    // GPU 数据缓冲区
    CUdeviceptr d_iblData_;
    IBLSamplingData h_iblData_;

    /**
     * @brief 加载 HDR 图像（.hdr, .exr）
     */
    bool loadHDRImage(const char* filepath, float** outPixels, unsigned int& width, unsigned int& height);

    /**
     * @brief 创建 CUDA 纹理对象
     */
    bool createTextureObject(const float* pixels, unsigned int width, unsigned int height);

    /**
     * @brief 在 CPU 上生成 CDF
     */
    void generateCDF_CPU(const float* pixels);

    /**
     * @brief 在 GPU 上生成 CDF（使用 CUDA kernel）
     */
    void generateCDF_GPU();

    /**
     * @brief 计算像素的亮度
     */
    static float luminance(const float* pixel) {
        return 0.2126f * pixel[0] + 0.7152f * pixel[1] + 0.0722f * pixel[2];
    }

    /**
     * @brief 生成程序化天空像素
     */
    void generateProceduralSkyPixels(
        const float3& sunDirection,
        float turbidity,
        float groundAlbedo,
        float** outPixels,
        unsigned int width,
        unsigned int height
    );
};

/**
 * @brief 环境光采样辅助函数（设备端使用）
 */
namespace IBLSampling {

/**
 * @brief 将方向转换为等距柱状投影 UV 坐标
 * @param direction 归一化方向向量
 * @return UV 坐标 [0,1]^2
 */
__device__ __forceinline__ float2 directionToEquirectangularUV(const float3& direction) {
    float phi = atan2f(direction.z, direction.x);
    float theta = acosf(fmaxf(fminf(direction.y, 1.0f), -1.0f));

    float u = (phi + M_PI) / (2.0f * M_PI);
    float v = theta / M_PI;

    return make_float2(u, v);
}

/**
 * @brief 将 UV 坐标转换为方向
 * @param uv UV 坐标 [0,1]^2
 * @return 归一化方向向量
 */
__device__ __forceinline__ float3 equirectangularUVToDirection(const float2& uv) {
    float phi = uv.x * 2.0f * M_PI - M_PI;
    float theta = uv.y * M_PI;

    float sinTheta = sinf(theta);
    float cosTheta = cosf(theta);
    float sinPhi = sinf(phi);
    float cosPhi = cosf(phi);

    return make_float3(
        sinTheta * cosPhi,
        cosTheta,
        sinTheta * sinPhi
    );
}

/**
 * @brief 采样环境贴图（使用重要性采样）
 * @param iblData IBL 采样数据
 * @param xi 随机数 [0,1)^2
 * @param outDirection 输出方向
 * @param outRadiance 输出辐射度
 * @param outPdf 输出 PDF
 */
__device__ void sampleEnvironmentMap(
    const IBLSamplingData& iblData,
    const float2& xi,
    float3& outDirection,
    float3& outRadiance,
    float& outPdf
);

/**
 * @brief 评估环境贴图（给定方向）
 * @param iblData IBL 采样数据
 * @param direction 方向
 * @return 辐射度
 */
__device__ float3 evaluateEnvironmentMap(
    const IBLSamplingData& iblData,
    const float3& direction
);

/**
 * @brief 计算给定方向的 PDF
 * @param iblData IBL 采样数据
 * @param direction 方向
 * @return PDF 值
 */
__device__ float environmentMapPDF(
    const IBLSamplingData& iblData,
    const float3& direction
);

} // namespace IBLSampling

} // namespace PhysicsRender

#endif // IBL_MANAGER_H
