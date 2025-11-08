// PerformanceOptimizer.h - 性能优化系统
// CUDA 流、BLAS compaction、异步更新

#ifndef PERFORMANCE_OPTIMIZER_H
#define PERFORMANCE_OPTIMIZER_H

#include <cuda.h>
#include <optix.h>
#include <vector>
#include <memory>
#include <queue>

namespace PhysicsRender {

/**
 * @brief CUDA 流管理器
 *
 * 管理多个 CUDA 流以实现并行操作
 */
class StreamManager {
public:
    /**
     * @brief 构造函数
     * @param numStreams 流的数量
     */
    explicit StreamManager(int numStreams = 4);

    /**
     * @brief 析构函数
     */
    ~StreamManager();

    /**
     * @brief 获取可用的流
     * @return CUDA 流句柄
     */
    CUstream getAvailableStream();

    /**
     * @brief 获取特定索引的流
     * @param index 流索引
     * @return CUDA 流句柄
     */
    CUstream getStream(int index) const;

    /**
     * @brief 同步所有流
     */
    void synchronizeAll();

    /**
     * @brief 同步特定流
     * @param index 流索引
     */
    void synchronize(int index);

    /**
     * @brief 获取流数量
     */
    int getStreamCount() const { return static_cast<int>(streams_.size()); }

private:
    std::vector<CUstream> streams_;
    int nextStreamIndex_;
};

/**
 * @brief 加速结构压缩器
 *
 * 实现 BLAS compaction 以减少内存使用
 */
class AccelStructureCompactor {
public:
    /**
     * @brief 构造函数
     * @param context OptiX 上下文
     */
    explicit AccelStructureCompactor(OptixDeviceContext context);

    /**
     * @brief 析构函数
     */
    ~AccelStructureCompactor();

    /**
     * @brief 构建并压缩 BLAS
     * @param buildInput 构建输入
     * @param buildOptions 构建选项
     * @param stream CUDA 流
     * @return 压缩后的 BLAS 句柄
     */
    OptixTraversableHandle buildAndCompact(
        const OptixBuildInput& buildInput,
        const OptixAccelBuildOptions& buildOptions,
        CUstream stream = 0
    );

    /**
     * @brief 获取压缩统计信息
     */
    struct CompactionStats {
        size_t originalSize;
        size_t compactedSize;
        float compressionRatio;
        int numCompacted;
    };

    CompactionStats getStats() const { return stats_; }

    /**
     * @brief 清理临时缓冲区
     */
    void cleanup();

private:
    OptixDeviceContext optixContext_;
    CompactionStats stats_;

    // 临时缓冲区管理
    std::vector<CUdeviceptr> tempBuffers_;
    std::vector<CUdeviceptr> outputBuffers_;
};

/**
 * @brief 异步几何更新管理器
 *
 * 管理几何的异步更新，避免阻塞渲染
 */
class AsyncGeometryUpdater {
public:
    /**
     * @brief 构造函数
     * @param context OptiX 上下文
     * @param numBuffers 双缓冲/三缓冲数量
     */
    explicit AsyncGeometryUpdater(
        OptixDeviceContext context,
        int numBuffers = 2
    );

    /**
     * @brief 析构函数
     */
    ~AsyncGeometryUpdater();

    /**
     * @brief 几何更新任务
     */
    struct UpdateTask {
        int geometryID;
        CUdeviceptr vertexData;
        size_t vertexCount;
        CUdeviceptr indexData;
        size_t indexCount;
        OptixTraversableHandle blasHandle;
        bool needsRebuild;  // true = rebuild, false = update
    };

    /**
     * @brief 提交更新任务
     * @param task 更新任务
     * @param stream CUDA 流
     */
    void submitUpdate(const UpdateTask& task, CUstream stream = 0);

    /**
     * @brief 处理所有待处理的更新
     * @return 处理的任务数量
     */
    int processPendingUpdates();

    /**
     * @brief 检查是否有待处理的更新
     */
    bool hasPendingUpdates() const { return !updateQueue_.empty(); }

    /**
     * @brief 等待所有更新完成
     */
    void waitForCompletion();

    /**
     * @brief 获取当前缓冲区索引（用于渲染）
     */
    int getCurrentBufferIndex() const { return currentBufferIndex_; }

    /**
     * @brief 交换缓冲区
     */
    void swapBuffers();

private:
    OptixDeviceContext optixContext_;
    int numBuffers_;
    int currentBufferIndex_;
    int writeBufferIndex_;

    std::queue<UpdateTask> updateQueue_;

    /**
     * @brief 执行 BLAS 更新
     */
    void performBLASUpdate(const UpdateTask& task, CUstream stream);

    /**
     * @brief 执行 BLAS 重建
     */
    void performBLASRebuild(const UpdateTask& task, CUstream stream);
};

/**
 * @brief 内存池管理器
 *
 * 管理临时 GPU 内存分配，减少分配开销
 */
class MemoryPoolManager {
public:
    /**
     * @brief 构造函数
     * @param initialSize 初始池大小（字节）
     */
    explicit MemoryPoolManager(size_t initialSize = 64 * 1024 * 1024);  // 64 MB

    /**
     * @brief 析构函数
     */
    ~MemoryPoolManager();

    /**
     * @brief 分配内存
     * @param size 大小（字节）
     * @return GPU 设备指针
     */
    CUdeviceptr allocate(size_t size);

    /**
     * @brief 释放内存
     * @param ptr GPU 设备指针
     */
    void deallocate(CUdeviceptr ptr);

    /**
     * @brief 清空池（释放所有未使用的内存）
     */
    void clear();

    /**
     * @brief 获取统计信息
     */
    struct PoolStats {
        size_t totalAllocated;
        size_t totalUsed;
        size_t totalFree;
        int numAllocations;
        int numDeallocations;
    };

    PoolStats getStats() const { return stats_; }

private:
    struct MemoryBlock {
        CUdeviceptr ptr;
        size_t size;
        bool inUse;
    };

    std::vector<MemoryBlock> blocks_;
    size_t poolSize_;
    PoolStats stats_;

    /**
     * @brief 扩展内存池
     */
    void expandPool(size_t additionalSize);

    /**
     * @brief 查找合适的空闲块
     */
    int findFreeBlock(size_t size);
};

/**
 * @brief 性能分析器
 *
 * 跟踪和报告性能指标
 */
class PerformanceProfiler {
public:
    /**
     * @brief 构造函数
     */
    PerformanceProfiler();

    /**
     * @brief 析构函数
     */
    ~PerformanceProfiler();

    /**
     * @brief 开始计时
     * @param name 计时器名称
     */
    void startTimer(const char* name);

    /**
     * @brief 结束计时
     * @param name 计时器名称
     */
    void stopTimer(const char* name);

    /**
     * @brief 记录事件
     * @param name 事件名称
     * @param value 数值
     */
    void recordEvent(const char* name, float value);

    /**
     * @brief 获取平均时间
     * @param name 计时器名称
     * @return 平均时间（毫秒）
     */
    float getAverageTime(const char* name) const;

    /**
     * @brief 打印报告
     */
    void printReport() const;

    /**
     * @brief 重置所有计时器
     */
    void reset();

private:
    struct TimerData {
        float totalTime;
        int sampleCount;
        float minTime;
        float maxTime;
        cudaEvent_t startEvent;
        cudaEvent_t stopEvent;
    };

    std::map<std::string, TimerData> timers_;
};

/**
 * @brief 性能优化器（主类）
 *
 * 集成所有性能优化功能
 */
class PerformanceOptimizer {
public:
    /**
     * @brief 构造函数
     * @param context OptiX 上下文
     */
    explicit PerformanceOptimizer(OptixDeviceContext context);

    /**
     * @brief 析构函数
     */
    ~PerformanceOptimizer();

    /**
     * @brief 获取流管理器
     */
    StreamManager& getStreamManager() { return *streamManager_; }

    /**
     * @brief 获取加速结构压缩器
     */
    AccelStructureCompactor& getCompactor() { return *compactor_; }

    /**
     * @brief 获取异步更新器
     */
    AsyncGeometryUpdater& getAsyncUpdater() { return *asyncUpdater_; }

    /**
     * @brief 获取内存池管理器
     */
    MemoryPoolManager& getMemoryPool() { return *memoryPool_; }

    /**
     * @brief 获取性能分析器
     */
    PerformanceProfiler& getProfiler() { return *profiler_; }

    /**
     * @brief 启用/禁用自动优化
     */
    void setAutoOptimize(bool enable) { autoOptimize_ = enable; }

    /**
     * @brief 运行自动优化
     */
    void runAutoOptimization();

private:
    OptixDeviceContext optixContext_;
    bool autoOptimize_;

    std::unique_ptr<StreamManager> streamManager_;
    std::unique_ptr<AccelStructureCompactor> compactor_;
    std::unique_ptr<AsyncGeometryUpdater> asyncUpdater_;
    std::unique_ptr<MemoryPoolManager> memoryPool_;
    std::unique_ptr<PerformanceProfiler> profiler_;
};

} // namespace PhysicsRender

#endif // PERFORMANCE_OPTIMIZER_H
