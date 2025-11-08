// PerformanceOptimizer.cpp - 性能优化系统实现

#include "rendering/PerformanceOptimizer.h"
#include "config.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <map>

namespace PhysicsRender {

// ============================================================================
// StreamManager 实现
// ============================================================================

StreamManager::StreamManager(int numStreams)
    : nextStreamIndex_(0)
{
    streams_.resize(numStreams);

    for (int i = 0; i < numStreams; ++i) {
        cudaError_t err = cudaStreamCreate(&streams_[i]);
        if (err != cudaSuccess) {
            std::cerr << "Failed to create CUDA stream " << i << ": "
                      << cudaGetErrorString(err) << std::endl;
            streams_[i] = 0;
        }
    }

    std::cout << "Created " << numStreams << " CUDA streams" << std::endl;
}

StreamManager::~StreamManager() {
    for (auto stream : streams_) {
        if (stream) {
            cudaStreamDestroy(stream);
        }
    }
}

CUstream StreamManager::getAvailableStream() {
    // 简单的轮询策略
    CUstream stream = streams_[nextStreamIndex_];
    nextStreamIndex_ = (nextStreamIndex_ + 1) % streams_.size();
    return stream;
}

CUstream StreamManager::getStream(int index) const {
    if (index >= 0 && index < static_cast<int>(streams_.size())) {
        return streams_[index];
    }
    return 0;
}

void StreamManager::synchronizeAll() {
    for (auto stream : streams_) {
        if (stream) {
            cudaStreamSynchronize(stream);
        }
    }
}

void StreamManager::synchronize(int index) {
    if (index >= 0 && index < static_cast<int>(streams_.size())) {
        cudaStreamSynchronize(streams_[index]);
    }
}

// ============================================================================
// AccelStructureCompactor 实现
// ============================================================================

AccelStructureCompactor::AccelStructureCompactor(OptixDeviceContext context)
    : optixContext_(context)
{
    stats_.originalSize = 0;
    stats_.compactedSize = 0;
    stats_.compressionRatio = 1.0f;
    stats_.numCompacted = 0;
}

AccelStructureCompactor::~AccelStructureCompactor() {
    cleanup();
}

OptixTraversableHandle AccelStructureCompactor::buildAndCompact(
    const OptixBuildInput& buildInput,
    const OptixAccelBuildOptions& buildOptions,
    CUstream stream
) {
    // 1. 配置压缩选项
    OptixAccelBuildOptions compactOptions = buildOptions;
    compactOptions.buildFlags |= OPTIX_BUILD_FLAG_ALLOW_COMPACTION;

    // 2. 准备发射属性（用于查询压缩大小）
    OptixAccelEmitDesc emitProperty = {};
    emitProperty.type = OPTIX_PROPERTY_TYPE_COMPACTED_SIZE;

    CUdeviceptr d_compactedSizeBuffer = 0;
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_compactedSizeBuffer), sizeof(size_t)));
    emitProperty.result = d_compactedSizeBuffer;

    // 3. 查询内存需求
    OptixAccelBufferSizes bufferSizes;
    OPTIX_CHECK(optixAccelComputeMemoryUsage(
        optixContext_,
        &compactOptions,
        &buildInput,
        1,
        &bufferSizes
    ));

    stats_.originalSize += bufferSizes.outputSizeInBytes;

    // 4. 分配临时和输出缓冲区
    CUdeviceptr d_tempBuffer = 0;
    CUdeviceptr d_outputBuffer = 0;

    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_tempBuffer), bufferSizes.tempSizeInBytes));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_outputBuffer), bufferSizes.outputSizeInBytes));

    tempBuffers_.push_back(d_tempBuffer);
    outputBuffers_.push_back(d_outputBuffer);

    // 5. 构建加速结构
    OptixTraversableHandle tempHandle = 0;

    OPTIX_CHECK(optixAccelBuild(
        optixContext_,
        stream,
        &compactOptions,
        &buildInput,
        1,
        d_tempBuffer,
        bufferSizes.tempSizeInBytes,
        d_outputBuffer,
        bufferSizes.outputSizeInBytes,
        &tempHandle,
        &emitProperty,
        1
    ));

    // 6. 等待构建完成
    CUDA_CHECK(cudaStreamSynchronize(stream));

    // 7. 获取压缩后的大小
    size_t compactedSize = 0;
    CUDA_CHECK(cudaMemcpy(
        &compactedSize,
        reinterpret_cast<void*>(d_compactedSizeBuffer),
        sizeof(size_t),
        cudaMemcpyDeviceToHost
    ));

    stats_.compactedSize += compactedSize;
    stats_.numCompacted++;

    // 8. 如果压缩能节省空间，执行压缩
    OptixTraversableHandle compactedHandle = tempHandle;

    if (compactedSize < bufferSizes.outputSizeInBytes) {
        CUdeviceptr d_compactedBuffer = 0;
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_compactedBuffer), compactedSize));

        OPTIX_CHECK(optixAccelCompact(
            optixContext_,
            stream,
            tempHandle,
            d_compactedBuffer,
            compactedSize,
            &compactedHandle
        ));

        CUDA_CHECK(cudaStreamSynchronize(stream));

        // 释放未压缩的缓冲区
        cudaFree(reinterpret_cast<void*>(d_outputBuffer));
        outputBuffers_.pop_back();

        std::cout << "  BLAS compacted: " << bufferSizes.outputSizeInBytes
                  << " -> " << compactedSize << " bytes ("
                  << (100.0f * compactedSize / bufferSizes.outputSizeInBytes)
                  << "%)" << std::endl;
    }

    // 9. 清理
    cudaFree(reinterpret_cast<void*>(d_compactedSizeBuffer));
    cudaFree(reinterpret_cast<void*>(d_tempBuffer));
    tempBuffers_.pop_back();

    // 更新统计
    stats_.compressionRatio = stats_.compactedSize / static_cast<float>(stats_.originalSize);

    return compactedHandle;
}

void AccelStructureCompactor::cleanup() {
    for (auto buffer : tempBuffers_) {
        if (buffer) {
            cudaFree(reinterpret_cast<void*>(buffer));
        }
    }
    tempBuffers_.clear();

    for (auto buffer : outputBuffers_) {
        if (buffer) {
            cudaFree(reinterpret_cast<void*>(buffer));
        }
    }
    outputBuffers_.clear();
}

// ============================================================================
// AsyncGeometryUpdater 实现
// ============================================================================

AsyncGeometryUpdater::AsyncGeometryUpdater(
    OptixDeviceContext context,
    int numBuffers
)
    : optixContext_(context)
    , numBuffers_(numBuffers)
    , currentBufferIndex_(0)
    , writeBufferIndex_(1 % numBuffers)
{
    std::cout << "AsyncGeometryUpdater initialized with " << numBuffers << " buffers" << std::endl;
}

AsyncGeometryUpdater::~AsyncGeometryUpdater() {
    waitForCompletion();
}

void AsyncGeometryUpdater::submitUpdate(const UpdateTask& task, CUstream stream) {
    updateQueue_.push(task);
}

void AsyncGeometryUpdater::performBLASUpdate(const UpdateTask& task, CUstream stream) {
    // 使用 OPTIX_BUILD_OPERATION_UPDATE 更新现有 BLAS
    // 这比重建更快，但几何拓扑必须相同

    OptixBuildInput buildInput = {};
    buildInput.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
    buildInput.triangleArray.vertexBuffers = &task.vertexData;
    buildInput.triangleArray.numVertices = static_cast<unsigned int>(task.vertexCount);
    buildInput.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
    buildInput.triangleArray.indexBuffer = task.indexData;
    buildInput.triangleArray.numIndexTriplets = static_cast<unsigned int>(task.indexCount / 3);
    buildInput.triangleArray.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;

    OptixAccelBuildOptions accelOptions = {};
    accelOptions.buildFlags = OPTIX_BUILD_FLAG_ALLOW_UPDATE;
    accelOptions.operation = OPTIX_BUILD_OPERATION_UPDATE;

    // 查询内存需求
    OptixAccelBufferSizes bufferSizes;
    OPTIX_CHECK(optixAccelComputeMemoryUsage(
        optixContext_,
        &accelOptions,
        &buildInput,
        1,
        &bufferSizes
    ));

    // 分配临时缓冲区
    CUdeviceptr d_tempBuffer = 0;
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_tempBuffer), bufferSizes.tempUpdateSizeInBytes));

    // 执行更新
    OptixTraversableHandle updatedHandle = 0;
    OPTIX_CHECK(optixAccelBuild(
        optixContext_,
        stream,
        &accelOptions,
        &buildInput,
        1,
        d_tempBuffer,
        bufferSizes.tempUpdateSizeInBytes,
        task.blasHandle,  // 原地更新
        bufferSizes.outputSizeInBytes,
        &updatedHandle,
        nullptr,
        0
    ));

    // 清理
    cudaFree(reinterpret_cast<void*>(d_tempBuffer));
}

void AsyncGeometryUpdater::performBLASRebuild(const UpdateTask& task, CUstream stream) {
    // 完整重建 BLAS
    OptixBuildInput buildInput = {};
    buildInput.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
    buildInput.triangleArray.vertexBuffers = &task.vertexData;
    buildInput.triangleArray.numVertices = static_cast<unsigned int>(task.vertexCount);
    buildInput.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
    buildInput.triangleArray.indexBuffer = task.indexData;
    buildInput.triangleArray.numIndexTriplets = static_cast<unsigned int>(task.indexCount / 3);
    buildInput.triangleArray.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;

    OptixAccelBuildOptions accelOptions = {};
    accelOptions.buildFlags = OPTIX_BUILD_FLAG_ALLOW_UPDATE;
    accelOptions.operation = OPTIX_BUILD_OPERATION_BUILD;

    // 查询内存需求
    OptixAccelBufferSizes bufferSizes;
    OPTIX_CHECK(optixAccelComputeMemoryUsage(
        optixContext_,
        &accelOptions,
        &buildInput,
        1,
        &bufferSizes
    ));

    // 分配缓冲区
    CUdeviceptr d_tempBuffer = 0;
    CUdeviceptr d_outputBuffer = 0;

    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_tempBuffer), bufferSizes.tempSizeInBytes));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_outputBuffer), bufferSizes.outputSizeInBytes));

    // 执行构建
    OptixTraversableHandle newHandle = 0;
    OPTIX_CHECK(optixAccelBuild(
        optixContext_,
        stream,
        &accelOptions,
        &buildInput,
        1,
        d_tempBuffer,
        bufferSizes.tempSizeInBytes,
        d_outputBuffer,
        bufferSizes.outputSizeInBytes,
        &newHandle,
        nullptr,
        0
    ));

    // 清理临时缓冲区
    cudaFree(reinterpret_cast<void*>(d_tempBuffer));

    // 注意：d_outputBuffer 不应该在这里释放，因为它包含新的 BLAS
}

int AsyncGeometryUpdater::processPendingUpdates() {
    int processedCount = 0;

    while (!updateQueue_.empty()) {
        UpdateTask task = updateQueue_.front();
        updateQueue_.pop();

        if (task.needsRebuild) {
            performBLASRebuild(task, 0);
        } else {
            performBLASUpdate(task, 0);
        }

        processedCount++;
    }

    return processedCount;
}

void AsyncGeometryUpdater::waitForCompletion() {
    CUDA_CHECK(cudaDeviceSynchronize());
}

void AsyncGeometryUpdater::swapBuffers() {
    currentBufferIndex_ = writeBufferIndex_;
    writeBufferIndex_ = (writeBufferIndex_ + 1) % numBuffers_;
}

// ============================================================================
// MemoryPoolManager 实现
// ============================================================================

MemoryPoolManager::MemoryPoolManager(size_t initialSize)
    : poolSize_(initialSize)
{
    stats_.totalAllocated = 0;
    stats_.totalUsed = 0;
    stats_.totalFree = 0;
    stats_.numAllocations = 0;
    stats_.numDeallocations = 0;

    // 初始分配
    expandPool(initialSize);
}

MemoryPoolManager::~MemoryPoolManager() {
    clear();
}

CUdeviceptr MemoryPoolManager::allocate(size_t size) {
    // 查找合适的空闲块
    int blockIndex = findFreeBlock(size);

    if (blockIndex >= 0) {
        blocks_[blockIndex].inUse = true;
        stats_.totalUsed += blocks_[blockIndex].size;
        stats_.totalFree -= blocks_[blockIndex].size;
        stats_.numAllocations++;
        return blocks_[blockIndex].ptr;
    }

    // 没有合适的块，扩展池
    expandPool(std::max(size, poolSize_ / 2));

    // 再次尝试
    blockIndex = findFreeBlock(size);
    if (blockIndex >= 0) {
        blocks_[blockIndex].inUse = true;
        stats_.totalUsed += blocks_[blockIndex].size;
        stats_.totalFree -= blocks_[blockIndex].size;
        stats_.numAllocations++;
        return blocks_[blockIndex].ptr;
    }

    std::cerr << "MemoryPool: Failed to allocate " << size << " bytes" << std::endl;
    return 0;
}

void MemoryPoolManager::deallocate(CUdeviceptr ptr) {
    for (auto& block : blocks_) {
        if (block.ptr == ptr && block.inUse) {
            block.inUse = false;
            stats_.totalUsed -= block.size;
            stats_.totalFree += block.size;
            stats_.numDeallocations++;
            return;
        }
    }

    std::cerr << "MemoryPool: Attempted to deallocate unknown pointer" << std::endl;
}

void MemoryPoolManager::clear() {
    for (auto& block : blocks_) {
        if (block.ptr) {
            cudaFree(reinterpret_cast<void*>(block.ptr));
        }
    }
    blocks_.clear();

    stats_.totalAllocated = 0;
    stats_.totalUsed = 0;
    stats_.totalFree = 0;
}

void MemoryPoolManager::expandPool(size_t additionalSize) {
    CUdeviceptr newBlock = 0;
    cudaError_t err = cudaMalloc(reinterpret_cast<void**>(&newBlock), additionalSize);

    if (err == cudaSuccess) {
        MemoryBlock block;
        block.ptr = newBlock;
        block.size = additionalSize;
        block.inUse = false;

        blocks_.push_back(block);

        stats_.totalAllocated += additionalSize;
        stats_.totalFree += additionalSize;

        std::cout << "MemoryPool expanded by " << additionalSize
                  << " bytes (total: " << stats_.totalAllocated << ")" << std::endl;
    } else {
        std::cerr << "MemoryPool: Failed to expand pool: "
                  << cudaGetErrorString(err) << std::endl;
    }
}

int MemoryPoolManager::findFreeBlock(size_t size) {
    // 最佳适配策略
    int bestIndex = -1;
    size_t bestSize = SIZE_MAX;

    for (int i = 0; i < static_cast<int>(blocks_.size()); ++i) {
        if (!blocks_[i].inUse && blocks_[i].size >= size) {
            if (blocks_[i].size < bestSize) {
                bestSize = blocks_[i].size;
                bestIndex = i;
            }
        }
    }

    return bestIndex;
}

// ============================================================================
// PerformanceProfiler 实现
// ============================================================================

PerformanceProfiler::PerformanceProfiler() {
}

PerformanceProfiler::~PerformanceProfiler() {
    // 销毁所有事件
    for (auto& pair : timers_) {
        if (pair.second.startEvent) {
            cudaEventDestroy(pair.second.startEvent);
        }
        if (pair.second.stopEvent) {
            cudaEventDestroy(pair.second.stopEvent);
        }
    }
}

void PerformanceProfiler::startTimer(const char* name) {
    auto it = timers_.find(name);

    if (it == timers_.end()) {
        // 创建新计时器
        TimerData timer;
        timer.totalTime = 0.0f;
        timer.sampleCount = 0;
        timer.minTime = FLT_MAX;
        timer.maxTime = 0.0f;

        cudaEventCreate(&timer.startEvent);
        cudaEventCreate(&timer.stopEvent);

        timers_[name] = timer;
        it = timers_.find(name);
    }

    cudaEventRecord(it->second.startEvent);
}

void PerformanceProfiler::stopTimer(const char* name) {
    auto it = timers_.find(name);
    if (it == timers_.end()) {
        std::cerr << "PerformanceProfiler: Timer '" << name << "' not found" << std::endl;
        return;
    }

    cudaEventRecord(it->second.stopEvent);
    cudaEventSynchronize(it->second.stopEvent);

    float elapsedTime = 0.0f;
    cudaEventElapsedTime(&elapsedTime, it->second.startEvent, it->second.stopEvent);

    it->second.totalTime += elapsedTime;
    it->second.sampleCount++;
    it->second.minTime = std::min(it->second.minTime, elapsedTime);
    it->second.maxTime = std::max(it->second.maxTime, elapsedTime);
}

void PerformanceProfiler::recordEvent(const char* name, float value) {
    // 简单记录（不使用 CUDA 事件）
    auto it = timers_.find(name);

    if (it == timers_.end()) {
        TimerData timer;
        timer.totalTime = value;
        timer.sampleCount = 1;
        timer.minTime = value;
        timer.maxTime = value;
        timer.startEvent = nullptr;
        timer.stopEvent = nullptr;

        timers_[name] = timer;
    } else {
        it->second.totalTime += value;
        it->second.sampleCount++;
        it->second.minTime = std::min(it->second.minTime, value);
        it->second.maxTime = std::max(it->second.maxTime, value);
    }
}

float PerformanceProfiler::getAverageTime(const char* name) const {
    auto it = timers_.find(name);
    if (it != timers_.end() && it->second.sampleCount > 0) {
        return it->second.totalTime / it->second.sampleCount;
    }
    return 0.0f;
}

void PerformanceProfiler::printReport() const {
    std::cout << "\n========== Performance Report ==========" << std::endl;

    for (const auto& pair : timers_) {
        if (pair.second.sampleCount > 0) {
            float avgTime = pair.second.totalTime / pair.second.sampleCount;

            std::cout << pair.first << ":" << std::endl;
            std::cout << "  Avg: " << avgTime << " ms" << std::endl;
            std::cout << "  Min: " << pair.second.minTime << " ms" << std::endl;
            std::cout << "  Max: " << pair.second.maxTime << " ms" << std::endl;
            std::cout << "  Samples: " << pair.second.sampleCount << std::endl;
        }
    }

    std::cout << "========================================\n" << std::endl;
}

void PerformanceProfiler::reset() {
    for (auto& pair : timers_) {
        pair.second.totalTime = 0.0f;
        pair.second.sampleCount = 0;
        pair.second.minTime = FLT_MAX;
        pair.second.maxTime = 0.0f;
    }
}

// ============================================================================
// PerformanceOptimizer 实现
// ============================================================================

PerformanceOptimizer::PerformanceOptimizer(OptixDeviceContext context)
    : optixContext_(context)
    , autoOptimize_(false)
{
    streamManager_ = std::make_unique<StreamManager>(4);
    compactor_ = std::make_unique<AccelStructureCompactor>(context);
    asyncUpdater_ = std::make_unique<AsyncGeometryUpdater>(context, 2);
    memoryPool_ = std::make_unique<MemoryPoolManager>(64 * 1024 * 1024);
    profiler_ = std::make_unique<PerformanceProfiler>();

    std::cout << "PerformanceOptimizer initialized" << std::endl;
}

PerformanceOptimizer::~PerformanceOptimizer() {
}

void PerformanceOptimizer::runAutoOptimization() {
    if (!autoOptimize_) {
        return;
    }

    // 自动优化策略（简化版）
    // 1. 检查压缩统计
    auto compactionStats = compactor_->getStats();
    if (compactionStats.compressionRatio < 0.8f) {
        std::cout << "Auto-optimization: BLAS compaction is effective ("
                  << (compactionStats.compressionRatio * 100.0f) << "%)" << std::endl;
    }

    // 2. 检查内存池效率
    auto poolStats = memoryPool_->getStats();
    if (poolStats.totalUsed > poolStats.totalAllocated * 0.9f) {
        std::cout << "Auto-optimization: Memory pool is nearly full, consider expanding" << std::endl;
    }

    // 3. 处理待处理的几何更新
    int processedUpdates = asyncUpdater_->processPendingUpdates();
    if (processedUpdates > 0) {
        std::cout << "Auto-optimization: Processed " << processedUpdates << " geometry updates" << std::endl;
    }
}

} // namespace PhysicsRender
