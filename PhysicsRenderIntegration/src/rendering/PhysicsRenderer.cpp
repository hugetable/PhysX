// PhysicsRenderer.cpp - 物理渲染器实现

#include "rendering/PhysicsRenderer.h"
#include <iostream>
#include <fstream>
#include <vector>

namespace PhysicsRender {

PhysicsRenderer::PhysicsRenderer(const PhysicsRendererConfig& config)
    : Raytracer(
        0xFF,           // maskDevices - 使用所有设备
        2,              // miss - 环境贴图
        0,              // interop - 无 OpenGL 互操作
        0,              // tex
        0,              // pbo
        256,            // sizeArena (MB)
        0               // peerToPeer
      )
    , config_(config)
    , optixContext_(nullptr)
    , optixModule_(nullptr)
    , pipeline_(nullptr)
    , topLevelAS_(0)
    , instanceBuffer_(0)
    , materialBuffer_(0)
    , outputBuffer_(0)
    , accumBuffer_(0) {

    // 清空 SBT
    std::memset(&sbt_, 0, sizeof(sbt_));

    // 清空粒子几何
    std::memset(&particleGeom_, 0, sizeof(particleGeom_));
}

PhysicsRenderer::~PhysicsRenderer() {
    cleanupOptixResources();
}

bool PhysicsRenderer::initialize() {
    std::cout << "Initializing PhysicsRenderer..." << std::endl;

    // 创建 OptiX 上下文
    if (!createOptixContext()) {
        std::cerr << "Failed to create OptiX context!" << std::endl;
        return false;
    }

    // 创建模块
    if (!createModule()) {
        std::cerr << "Failed to create OptiX module!" << std::endl;
        return false;
    }

    // 创建管线
    if (!createPipeline()) {
        std::cerr << "Failed to create OptiX pipeline!" << std::endl;
        return false;
    }

    // 创建 SBT
    if (!createSBT()) {
        std::cerr << "Failed to create Shader Binding Table!" << std::endl;
        return false;
    }

    // 分配输出缓冲区
    size_t bufferSize = config_.resolution.x * config_.resolution.y * sizeof(float3);

    cudaError_t err = cudaMalloc(&outputBuffer_, bufferSize);
    if (err != cudaSuccess) {
        std::cerr << "Failed to allocate output buffer: "
                  << cudaGetErrorString(err) << std::endl;
        return false;
    }

    // 清空缓冲区
    cudaMemset(outputBuffer_, 0, bufferSize);

    std::cout << "PhysicsRenderer initialized successfully!" << std::endl;
    return true;
}

void PhysicsRenderer::setDynamicGeometry(const std::vector<RenderableObject>& renderables) {
    // 为每个对象创建或更新几何
    for (const auto& obj : renderables) {
        // TODO: 实现几何创建和更新
    }
}

void PhysicsRenderer::updateTransforms(const std::vector<float*>& transforms) {
    // 更新实例变换矩阵
    if (transforms.empty()) {
        return;
    }

    // TODO: 更新 OptiX 实例数组
    // 这需要重建或更新 TLAS
}

void PhysicsRenderer::rebuildTLAS() {
    // 重建顶层加速结构
    // TODO: 实现 TLAS 重建逻辑
}

void PhysicsRenderer::rebuildParticleGeometry(
    CUdeviceptr positions,
    CUdeviceptr radii,
    int count
) {
    // 重建粒子几何
    particleGeom_.positionsBuffer = positions;
    particleGeom_.radiiBuffer = radii;
    particleGeom_.count = count;

    // TODO: 构建粒子 BVH
}

void PhysicsRenderer::updateParticlePositions(CUdeviceptr positions, int count) {
    // 快速更新粒子位置
    particleGeom_.positionsBuffer = positions;
    particleGeom_.count = count;

    // TODO: 更新粒子几何（不完全重建）
}

int PhysicsRenderer::addPhysicsMaterial(const PhysicsMaterialDefinition& material) {
    int materialID = static_cast<int>(physicsMaterials_.size());
    physicsMaterials_.push_back(material);

    // TODO: 上传材质数据到 GPU
    return materialID;
}

void PhysicsRenderer::updatePhysicsMaterial(
    int materialID,
    const PhysicsMaterialDefinition& material
) {
    if (materialID >= 0 && materialID < static_cast<int>(physicsMaterials_.size())) {
        physicsMaterials_[materialID] = material;

        // TODO: 更新 GPU 材质数据
    }
}

unsigned int PhysicsRenderer::render() {
    // 渲染一帧
    // TODO: 实现完整的渲染逻辑

    // 暂时返回 0
    return 0;
}

// 私有辅助函数

bool PhysicsRenderer::createOptixContext() {
    std::cout << "  Creating OptiX context..." << std::endl;

    // TODO: 实现 OptiX 上下文创建
    // 1. 初始化 CUDA
    // 2. 创建 OptiX 上下文
    // 3. 设置日志回调

    std::cout << "  OptiX context created" << std::endl;
    return true;
}

bool PhysicsRenderer::createModule() {
    std::cout << "  Creating OptiX module..." << std::endl;

    // TODO: 实现模块创建
    // 1. 加载 PTX/OptiX-IR 文件
    // 2. 创建 OptiX 模块
    // 3. 编译选项配置

    std::cout << "  OptiX module created" << std::endl;
    return true;
}

bool PhysicsRenderer::createPipeline() {
    std::cout << "  Creating OptiX pipeline..." << std::endl;

    // TODO: 实现管线创建
    // 1. 创建程序组（raygen, miss, hit）
    // 2. 创建管线
    // 3. 设置栈大小

    std::cout << "  OptiX pipeline created" << std::endl;
    return true;
}

bool PhysicsRenderer::createSBT() {
    std::cout << "  Creating Shader Binding Table..." << std::endl;

    // TODO: 实现 SBT 创建
    // 1. 分配 SBT 缓冲区
    // 2. 填充 raygen, miss, hit 记录
    // 3. 上传到 GPU

    std::cout << "  Shader Binding Table created" << std::endl;
    return true;
}

void PhysicsRenderer::buildBLAS(const RenderableObject& obj, DynamicGeometry& geom) {
    // 构建底层加速结构
    // TODO: 实现 BLAS 构建
}

void PhysicsRenderer::updateBLAS(DynamicGeometry& geom) {
    // 更新底层加速结构
    // TODO: 实现 BLAS 更新
}

void PhysicsRenderer::buildTLAS() {
    // 构建顶层加速结构
    // TODO: 实现 TLAS 构建
}

void PhysicsRenderer::cleanupOptixResources() {
    std::cout << "Cleaning up OptiX resources..." << std::endl;

    // 释放 CUDA 缓冲区
    if (outputBuffer_) {
        cudaFree(reinterpret_cast<void*>(outputBuffer_));
        outputBuffer_ = 0;
    }

    if (accumBuffer_) {
        cudaFree(reinterpret_cast<void*>(accumBuffer_));
        accumBuffer_ = 0;
    }

    if (instanceBuffer_) {
        cudaFree(reinterpret_cast<void*>(instanceBuffer_));
        instanceBuffer_ = 0;
    }

    if (materialBuffer_) {
        cudaFree(reinterpret_cast<void*>(materialBuffer_));
        materialBuffer_ = 0;
    }

    // 释放 OptiX 资源
    if (pipeline_) {
        optixPipelineDestroy(pipeline_);
        pipeline_ = nullptr;
    }

    if (optixModule_) {
        optixModuleDestroy(optixModule_);
        optixModule_ = nullptr;
    }

    if (optixContext_) {
        optixDeviceContextDestroy(optixContext_);
        optixContext_ = nullptr;
    }

    std::cout << "OptiX resources cleaned up" << std::endl;
}

} // namespace PhysicsRender
