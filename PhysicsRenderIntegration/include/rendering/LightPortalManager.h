// LightPortalManager.h - 光线门户系统
// 优化室内场景的光照采样

#ifndef LIGHT_PORTAL_MANAGER_H
#define LIGHT_PORTAL_MANAGER_H

#include <cuda.h>
#include <optix.h>
#include <vector>

namespace PhysicsRender {

/**
 * @brief 光线门户类型
 */
enum class PortalType {
    RECTANGLE,      // 矩形门户（窗户）
    CIRCLE,         // 圆形门户
    POLYGON         // 多边形门户
};

/**
 * @brief 光线门户参数
 */
struct LightPortalParams {
    PortalType type = PortalType::RECTANGLE;

    // 位置和方向
    float3 position = make_float3(0.0f, 0.0f, 0.0f);
    float3 normal = make_float3(0.0f, 0.0f, 1.0f);    // 指向室内
    float3 tangent = make_float3(1.0f, 0.0f, 0.0f);

    // 尺寸
    float width = 1.0f;
    float height = 1.0f;
    float radius = 0.5f;  // 用于圆形

    // 多边形顶点（用于 POLYGON 类型）
    std::vector<float3> vertices;

    // 重要性权重
    float importance = 1.0f;

    // 是否启用
    bool enabled = true;

    int portalID = -1;
};

/**
 * @brief 光线门户（设备端）
 */
struct LightPortal {
    int type;  // PortalType

    float3 position;
    float3 normal;
    float3 tangent;
    float3 bitangent;

    float width;
    float height;
    float radius;
    float area;

    float importance;
    bool enabled;

    // 对于多边形门户
    int vertexCount;
    int vertexOffset;  // 在顶点缓冲区中的偏移
};

/**
 * @brief 光线门户管理器
 *
 * 管理光线门户，用于优化室内场景的环境光采样
 */
class LightPortalManager {
public:
    /**
     * @brief 构造函数
     */
    LightPortalManager();

    /**
     * @brief 析构函数
     */
    ~LightPortalManager();

    /**
     * @brief 添加矩形门户（窗户）
     * @param position 位置
     * @param normal 法线（指向室内）
     * @param width 宽度
     * @param height 高度
     * @param importance 重要性权重
     * @return 门户索引
     */
    int addRectanglePortal(
        const float3& position,
        const float3& normal,
        float width,
        float height,
        float importance = 1.0f
    );

    /**
     * @brief 添加圆形门户
     * @param position 位置
     * @param normal 法线
     * @param radius 半径
     * @param importance 重要性权重
     * @return 门户索引
     */
    int addCirclePortal(
        const float3& position,
        const float3& normal,
        float radius,
        float importance = 1.0f
    );

    /**
     * @brief 添加自定义门户
     * @param params 门户参数
     * @return 门户索引
     */
    int addPortal(const LightPortalParams& params);

    /**
     * @brief 更新门户
     * @param index 门户索引
     * @param params 新参数
     */
    void updatePortal(int index, const LightPortalParams& params);

    /**
     * @brief 移除门户
     * @param index 门户索引
     */
    void removePortal(int index);

    /**
     * @brief 启用/禁用门户
     * @param index 门户索引
     * @param enabled 是否启用
     */
    void setPortalEnabled(int index, bool enabled);

    /**
     * @brief 获取门户数量
     */
    int getPortalCount() const { return static_cast<int>(portals_.size()); }

    /**
     * @brief 上传门户数据到 GPU
     * @return GPU 设备指针
     */
    CUdeviceptr uploadToGPU();

    /**
     * @brief 获取 GPU 缓冲区指针
     */
    CUdeviceptr getGPUBuffer() const { return d_portalBuffer_; }

    /**
     * @brief 自动检测门户
     * @param scene 场景几何
     * @param threshold 检测阈值
     * @return 检测到的门户数量
     */
    int autoDetectPortals(const void* scene, float threshold = 0.5f);

    /**
     * @brief 启用全局门户系统
     */
    void setGlobalEnabled(bool enabled) { globalEnabled_ = enabled; }
    bool isGlobalEnabled() const { return globalEnabled_; }

    /**
     * @brief 设置重要性采样策略
     */
    enum class SamplingStrategy {
        UNIFORM,            // 均匀采样所有门户
        IMPORTANCE,         // 基于重要性采样
        ADAPTIVE            // 自适应（基于可见性）
    };

    void setSamplingStrategy(SamplingStrategy strategy) {
        samplingStrategy_ = strategy;
    }

private:
    std::vector<LightPortalParams> portals_;
    std::vector<LightPortal> devicePortals_;
    std::vector<float3> portalVertices_;  // 用于多边形门户

    // GPU 缓冲区
    CUdeviceptr d_portalBuffer_;
    CUdeviceptr d_vertexBuffer_;
    size_t bufferSize_;
    size_t vertexBufferSize_;

    // 全局设置
    bool globalEnabled_;
    SamplingStrategy samplingStrategy_;

    // 重要性采样数据
    std::vector<float> portalCDF_;
    CUdeviceptr d_portalCDF_;

    /**
     * @brief 将主机端参数转换为设备端门户
     */
    LightPortal convertToDevicePortal(const LightPortalParams& params, int vertexOffset);

    /**
     * @brief 计算门户面积
     */
    float calculateArea(const LightPortalParams& params);

    /**
     * @brief 生成门户重要性采样 CDF
     */
    void generatePortalCDF();
};

/**
 * @brief 光线门户采样函数（设备端）
 */
namespace PortalSampling {

/**
 * @brief 采样光线门户
 * @param portals 门户数组
 * @param numPortals 门户数量
 * @param position 着色点位置
 * @param normal 着色点法线
 * @param xi 随机数 [0,1)^3
 * @param outDirection 输出方向
 * @param outPdf 输出 PDF
 * @return 是否成功采样
 */
__device__ bool sampleLightPortal(
    const LightPortal* portals,
    int numPortals,
    const float3& position,
    const float3& normal,
    const float3& xi,
    float3& outDirection,
    float& outPdf
);

/**
 * @brief 采样单个矩形门户
 * @param portal 门户
 * @param position 着色点位置
 * @param xi 随机数 [0,1)^2
 * @param outDirection 输出方向
 * @param outPdf 输出 PDF
 */
__device__ void sampleRectanglePortal(
    const LightPortal& portal,
    const float3& position,
    const float2& xi,
    float3& outDirection,
    float& outPdf
);

/**
 * @brief 计算门户的 PDF
 * @param portal 门户
 * @param position 着色点位置
 * @param direction 方向
 * @return PDF 值
 */
__device__ float portalPDF(
    const LightPortal& portal,
    const float3& position,
    const float3& direction
);

/**
 * @brief 检查射线是否与门户相交
 * @param portal 门户
 * @param rayOrigin 光线起点
 * @param rayDirection 光线方向
 * @param tMin 最小 t
 * @param tMax 最大 t
 * @param outT 输出交点 t
 * @return 是否相交
 */
__device__ bool intersectPortal(
    const LightPortal& portal,
    const float3& rayOrigin,
    const float3& rayDirection,
    float tMin,
    float tMax,
    float& outT
);

/**
 * @brief 计算门户的可见性权重
 * @param portal 门户
 * @param position 着色点位置
 * @param normal 着色点法线
 * @return 可见性权重 [0,1]
 */
__device__ float portalVisibilityWeight(
    const LightPortal& portal,
    const float3& position,
    const float3& normal
);

} // namespace PortalSampling

} // namespace PhysicsRender

#endif // LIGHT_PORTAL_MANAGER_H
