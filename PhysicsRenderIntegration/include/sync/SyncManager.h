// SyncManager.h - 物理与渲染同步管理器

#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include "config.h"
#include "physics/PhysicsSimulator.h"
#include "rendering/PhysicsRenderer.h"

#include <cuda_runtime.h>
#include <vector>
#include <chrono>

namespace PhysicsRender {

/**
 * @brief 同步策略
 */
enum class SyncStrategy {
    IMMEDIATE,      // 立即同步（每帧）
    BUFFERED,       // 缓冲同步（双缓冲）
    ASYNC           // 异步同步（CUDA流）
};

/**
 * @brief 同步配置
 */
struct SyncConfig {
    SyncStrategy strategy = SyncStrategy::BUFFERED;
    bool useGPUDirect = true;        // GPU 直接通信
    bool incrementalUpdate = true;   // 增量更新
    int updateFrequency = 1;         // 更新频率（每 N 帧）
};

/**
 * @brief 同步统计信息
 */
struct SyncStatistics {
    float geometrySyncTime;     // 几何同步时间 (ms)
    float materialSyncTime;     // 材质同步时间 (ms)
    float asBuildTime;          // AS 构建时间 (ms)
    int numUpdatedObjects;      // 更新的对象数量
    int numUpdatedParticles;    // 更新的粒子数量
};

/**
 * @brief 同步管理器
 *
 * 负责将物理模拟结果同步到渲染系统
 */
class SyncManager {
public:
    /**
     * @brief 构造函数
     * @param config 同步配置
     */
    explicit SyncManager(const SyncConfig& config = SyncConfig());

    /**
     * @brief 析构函数
     */
    ~SyncManager();

    /**
     * @brief 初始化同步管理器
     * @return 成功返回 true
     */
    bool initialize();

    /**
     * @brief 同步物理到渲染
     * @param physics 物理模拟器
     * @param renderer 渲染器
     */
    void sync(PhysicsSimulator& physics, PhysicsRenderer& renderer);

    /**
     * @brief 获取同步统计信息
     * @return 统计信息
     */
    const SyncStatistics& getStatistics() const { return stats_; }

    /**
     * @brief 设置同步策略
     * @param strategy 同步策略
     */
    void setStrategy(SyncStrategy strategy) { config_.strategy = strategy; }

private:
    SyncConfig config_;
    SyncStatistics stats_;

    // CUDA 流（用于异步操作）
    cudaStream_t stream_;

    // 双缓冲
    struct BufferPair {
        std::vector<RenderableObject> front;
        std::vector<RenderableObject> back;
        bool swapped;
    };
    BufferPair buffers_;

    int frameCounter_;

    // 同步方法
    void syncImmediate(PhysicsSimulator& physics, PhysicsRenderer& renderer);
    void syncBuffered(PhysicsSimulator& physics, PhysicsRenderer& renderer);
    void syncAsync(PhysicsSimulator& physics, PhysicsRenderer& renderer);

    // 同步功能
    void syncRigidBodies(
        const std::vector<RenderableObject>& objects,
        PhysicsRenderer& renderer
    );

    void syncParticles(
        const std::vector<RenderableObject>& objects,
        PhysicsRenderer& renderer
    );

    void syncBlastChunks(
        const std::vector<RenderableObject>& objects,
        PhysicsRenderer& renderer
    );

    void syncMaterials(
        const std::vector<RenderableObject>& objects,
        PhysicsRenderer& renderer
    );

    // 辅助函数
    bool hasObjectsMoved(
        const std::vector<RenderableObject>& oldObjects,
        const std::vector<RenderableObject>& newObjects
    );

    void swapBuffers();

    // 计时器
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    TimePoint startTimer();
    float endTimer(TimePoint start);  // 返回毫秒
};

} // namespace PhysicsRender

#endif // SYNC_MANAGER_H
