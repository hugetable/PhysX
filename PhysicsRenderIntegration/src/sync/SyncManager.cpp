// SyncManager.cpp - 同步管理器实现

#include "sync/SyncManager.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace PhysicsRender {

SyncManager::SyncManager(const SyncConfig& config)
    : config_(config)
    , stream_(nullptr)
    , frameCounter_(0) {

    // 清空统计信息
    std::memset(&stats_, 0, sizeof(stats_));

    buffers_.swapped = false;
}

SyncManager::~SyncManager() {
    if (stream_) {
        cudaStreamDestroy(stream_);
    }
}

bool SyncManager::initialize() {
    std::cout << "Initializing SyncManager..." << std::endl;

    // 创建 CUDA 流（用于异步操作）
    if (config_.strategy == SyncStrategy::ASYNC) {
        cudaError_t err = cudaStreamCreate(&stream_);
        if (err != cudaSuccess) {
            std::cerr << "Failed to create CUDA stream: "
                      << cudaGetErrorString(err) << std::endl;
            return false;
        }
        std::cout << "  Created CUDA stream for async sync" << std::endl;
    }

    std::cout << "SyncManager initialized successfully!" << std::endl;
    return true;
}

void SyncManager::sync(PhysicsSimulator& physics, PhysicsRenderer& renderer) {
    auto start = startTimer();

    // 根据策略选择同步方法
    switch (config_.strategy) {
        case SyncStrategy::IMMEDIATE:
            syncImmediate(physics, renderer);
            break;

        case SyncStrategy::BUFFERED:
            syncBuffered(physics, renderer);
            break;

        case SyncStrategy::ASYNC:
            syncAsync(physics, renderer);
            break;
    }

    // 更新统计信息
    float totalTime = endTimer(start);
    (void)totalTime; // 避免未使用警告

    frameCounter_++;
}

// 私有同步方法

void SyncManager::syncImmediate(PhysicsSimulator& physics, PhysicsRenderer& renderer) {
    // 立即同步 - 每帧都更新
    auto renderables = physics.getRenderables();

    auto geometryStart = startTimer();
    syncRigidBodies(renderables, renderer);
    stats_.geometrySyncTime = endTimer(geometryStart);

    auto materialStart = startTimer();
    syncMaterials(renderables, renderer);
    stats_.materialSyncTime = endTimer(materialStart);

    auto asStart = startTimer();
    renderer.rebuildTLAS();
    stats_.asBuildTime = endTimer(asStart);

    stats_.numUpdatedObjects = static_cast<int>(renderables.size());
}

void SyncManager::syncBuffered(PhysicsSimulator& physics, PhysicsRenderer& renderer) {
    // 缓冲同步 - 使用双缓冲
    auto& currentBuffer = buffers_.swapped ? buffers_.front : buffers_.back;
    auto& nextBuffer = buffers_.swapped ? buffers_.back : buffers_.front;

    // 获取新数据
    nextBuffer = physics.getRenderables();

    // 检查是否需要更新
    bool needsUpdate = hasObjectsMoved(currentBuffer, nextBuffer);

    if (needsUpdate || (frameCounter_ % config_.updateFrequency == 0)) {
        auto geometryStart = startTimer();
        syncRigidBodies(nextBuffer, renderer);
        stats_.geometrySyncTime = endTimer(geometryStart);

        auto materialStart = startTimer();
        syncMaterials(nextBuffer, renderer);
        stats_.materialSyncTime = endTimer(materialStart);

        auto asStart = startTimer();
        renderer.rebuildTLAS();
        stats_.asBuildTime = endTimer(asStart);

        stats_.numUpdatedObjects = static_cast<int>(nextBuffer.size());

        // 交换缓冲区
        swapBuffers();
    } else {
        stats_.numUpdatedObjects = 0;
    }
}

void SyncManager::syncAsync(PhysicsSimulator& physics, PhysicsRenderer& renderer) {
    // 异步同步 - 使用 CUDA 流
    auto renderables = physics.getRenderables();

    // TODO: 实现真正的异步同步
    // 目前回退到立即同步
    syncImmediate(physics, renderer);
}

void SyncManager::syncRigidBodies(
    const std::vector<RenderableObject>& objects,
    PhysicsRenderer& renderer
) {
    // 收集所有刚体的变换矩阵
    std::vector<float*> transforms;
    transforms.reserve(objects.size());

    for (const auto& obj : objects) {
        if (obj.type == RenderableObject::RIGID_BODY) {
            // 注意: 这里需要 const_cast，因为变换数据需要上传到 GPU
            transforms.push_back(const_cast<float*>(obj.transform));
        }
    }

    if (!transforms.empty()) {
        renderer.updateTransforms(transforms);
    }
}

void SyncManager::syncParticles(
    const std::vector<RenderableObject>& objects,
    PhysicsRenderer& renderer
) {
    // 收集粒子数据
    int particleCount = 0;
    for (const auto& obj : objects) {
        if (obj.type == RenderableObject::PARTICLE) {
            particleCount++;
        }
    }

    if (particleCount > 0) {
        // TODO: 实现粒子数据的批量传输
        // 需要将粒子位置和半径打包成 GPU buffer
        stats_.numUpdatedParticles = particleCount;
    } else {
        stats_.numUpdatedParticles = 0;
    }
}

void SyncManager::syncBlastChunks(
    const std::vector<RenderableObject>& objects,
    PhysicsRenderer& renderer
) {
    // 处理 Blast 碎片
    for (const auto& obj : objects) {
        if (obj.type == RenderableObject::CHUNK) {
            // TODO: 处理碎片的动态创建和销毁
        }
    }
}

void SyncManager::syncMaterials(
    const std::vector<RenderableObject>& objects,
    PhysicsRenderer& renderer
) {
    // 更新材质参数
    // 例如：破损程度、湿润度等动态属性
    for (const auto& obj : objects) {
        // TODO: 根据物理状态更新材质参数
        // renderer.updatePhysicsMaterial(obj.materialID, updatedMaterial);
    }
}

bool SyncManager::hasObjectsMoved(
    const std::vector<RenderableObject>& oldObjects,
    const std::vector<RenderableObject>& newObjects
) {
    // 检查对象数量是否变化
    if (oldObjects.size() != newObjects.size()) {
        return true;
    }

    // 检查是否有对象移动
    const float EPSILON = 1e-5f;

    for (size_t i = 0; i < oldObjects.size(); ++i) {
        const auto& oldObj = oldObjects[i];
        const auto& newObj = newObjects[i];

        // 比较变换矩阵（只检查位置部分）
        float dx = std::abs(oldObj.transform[3] - newObj.transform[3]);
        float dy = std::abs(oldObj.transform[7] - newObj.transform[7]);
        float dz = std::abs(oldObj.transform[11] - newObj.transform[11]);

        if (dx > EPSILON || dy > EPSILON || dz > EPSILON) {
            return true;
        }
    }

    return false;
}

void SyncManager::swapBuffers() {
    buffers_.swapped = !buffers_.swapped;
}

// 计时器辅助函数

SyncManager::TimePoint SyncManager::startTimer() {
    return Clock::now();
}

float SyncManager::endTimer(TimePoint start) {
    auto end = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return duration.count() / 1000.0f;  // 转换为毫秒
}

} // namespace PhysicsRender
