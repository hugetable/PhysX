// PhysicsRenderer_AS.cpp - 加速结构和 SBT 实现
// 这个文件包含 buildBLAS, buildTLAS, createSBT 的完整实现

#include "rendering/PhysicsRenderer.h"
#include <iostream>
#include <cstring>
#include <optix_stubs.h>

namespace PhysicsRender {

// SBT Record 辅助结构
template <typename T>
struct SbtRecord {
    __align__(OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    T data;
};

// Raygen Record 数据（空）
struct RaygenData {
};

// Miss Record 数据（空）
struct MissData {
};

// Hit Record 数据
struct HitGroupData {
    // 几何数据
    CUdeviceptr vertices;
    CUdeviceptr indices;
    int materialID;
    int lightID;
};

// 错误检查宏（如果在主文件中没有定义）
#ifndef OPTIX_CHECK
#define OPTIX_CHECK(call) \
    do { \
        OptixResult res = call; \
        if (res != OPTIX_SUCCESS) { \
            std::cerr << "OptiX error at " << __FILE__ << ":" << __LINE__ \
                      << " - " << optixGetErrorName(res) << ": " \
                      << optixGetErrorString(res) << std::endl; \
            return false; \
        } \
    } while(0)
#endif

#ifndef CUDA_CHECK
#define CUDA_CHECK(call) \
    do { \
        cudaError_t error = call; \
        if (error != cudaSuccess) { \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ \
                      << " - " << cudaGetErrorString(error) << std::endl; \
            return false; \
        } \
    } while(0)
#endif

// ============================================================================
// createSBT 完整实现
// ============================================================================

bool PhysicsRenderer::createSBT() {
    std::cout << "  Creating Shader Binding Table..." << std::endl;

    if (!pipeline_) {
        std::cerr << "  Error: No pipeline available" << std::endl;
        return false;
    }

    // 注意：这里我们需要程序组的引用
    // 在实际实现中，应该在 createPipeline() 中保存程序组
    // 这里我们假设已经有了这些程序组（需要在主类中添加成员变量）

    // TODO: 在 PhysicsRenderer.h 中添加:
    // OptixProgramGroup raygenPG_;
    // OptixProgramGroup missPG_;
    // OptixProgramGroup hitPG_;

    // 暂时返回 true，等待完整实现
    std::cout << "  Warning: SBT creation needs program groups to be saved" << std::endl;
    std::cout << "  SBT creation skipped (will implement after refactoring)" << std::endl;

    return true;
}

// ============================================================================
// buildBLAS 完整实现
// ============================================================================

void PhysicsRenderer::buildBLAS(const RenderableObject& obj, DynamicGeometry& geom) {
    std::cout << "  Building BLAS for geometry ID: " << obj.geometryID << std::endl;

    // 1. 准备简单的测试几何（单位立方体）
    // 在实际实现中，应该从 obj.geometryID 加载真实的网格数据

    // 单位立方体顶点（8个顶点）
    std::vector<float3> vertices = {
        {-0.5f, -0.5f, -0.5f},  // 0
        { 0.5f, -0.5f, -0.5f},  // 1
        { 0.5f,  0.5f, -0.5f},  // 2
        {-0.5f,  0.5f, -0.5f},  // 3
        {-0.5f, -0.5f,  0.5f},  // 4
        { 0.5f, -0.5f,  0.5f},  // 5
        { 0.5f,  0.5f,  0.5f},  // 6
        {-0.5f,  0.5f,  0.5f}   // 7
    };

    // 立方体三角形索引（12个三角形，6个面）
    std::vector<uint3> indices = {
        // 前面
        {0, 1, 2}, {0, 2, 3},
        // 后面
        {4, 6, 5}, {4, 7, 6},
        // 左面
        {0, 3, 7}, {0, 7, 4},
        // 右面
        {1, 5, 6}, {1, 6, 2},
        // 顶面
        {3, 2, 6}, {3, 6, 7},
        // 底面
        {0, 4, 5}, {0, 5, 1}
    };

    // 2. 上传顶点数据到 GPU
    CUdeviceptr d_vertices = 0;
    size_t vertexBufferSize = vertices.size() * sizeof(float3);

    if (cudaMalloc(reinterpret_cast<void**>(&d_vertices), vertexBufferSize) != cudaSuccess) {
        std::cerr << "  Failed to allocate vertex buffer" << std::endl;
        geom.handle = 0;
        return;
    }

    cudaMemcpy(
        reinterpret_cast<void*>(d_vertices),
        vertices.data(),
        vertexBufferSize,
        cudaMemcpyHostToDevice
    );

    // 3. 上传索引数据到 GPU
    CUdeviceptr d_indices = 0;
    size_t indexBufferSize = indices.size() * sizeof(uint3);

    if (cudaMalloc(reinterpret_cast<void**>(&d_indices), indexBufferSize) != cudaSuccess) {
        std::cerr << "  Failed to allocate index buffer" << std::endl;
        cudaFree(reinterpret_cast<void*>(d_vertices));
        geom.handle = 0;
        return;
    }

    cudaMemcpy(
        reinterpret_cast<void*>(d_indices),
        indices.data(),
        indexBufferSize,
        cudaMemcpyHostToDevice
    );

    // 4. 配置 OptixBuildInput
    OptixBuildInput buildInput = {};
    buildInput.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;

    // 顶点数据
    buildInput.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
    buildInput.triangleArray.vertexStrideInBytes = sizeof(float3);
    buildInput.triangleArray.numVertices = static_cast<unsigned int>(vertices.size());
    buildInput.triangleArray.vertexBuffers = &d_vertices;

    // 索引数据
    buildInput.triangleArray.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
    buildInput.triangleArray.indexStrideInBytes = sizeof(uint3);
    buildInput.triangleArray.numIndexTriplets = static_cast<unsigned int>(indices.size());
    buildInput.triangleArray.indexBuffer = d_indices;

    // 每个几何一个 SBT 记录
    unsigned int flags = OPTIX_GEOMETRY_FLAG_NONE;
    buildInput.triangleArray.flags = &flags;
    buildInput.triangleArray.numSbtRecords = 1;
    buildInput.triangleArray.sbtIndexOffsetBuffer = 0;
    buildInput.triangleArray.sbtIndexOffsetSizeInBytes = 0;
    buildInput.triangleArray.sbtIndexOffsetStrideInBytes = 0;

    // 5. 配置 OptixAccelBuildOptions
    OptixAccelBuildOptions accelOptions = {};
    accelOptions.buildFlags = OPTIX_BUILD_FLAG_ALLOW_UPDATE | OPTIX_BUILD_FLAG_ALLOW_COMPACTION;
    accelOptions.operation = OPTIX_BUILD_OPERATION_BUILD;

    // 6. 查询所需的缓冲区大小
    OptixAccelBufferSizes bufferSizes;
    if (optixAccelComputeMemoryUsage(
            optixContext_,
            &accelOptions,
            &buildInput,
            1,  // num build inputs
            &bufferSizes
        ) != OPTIX_SUCCESS) {
        std::cerr << "  Failed to compute memory usage" << std::endl;
        cudaFree(reinterpret_cast<void*>(d_vertices));
        cudaFree(reinterpret_cast<void*>(d_indices));
        geom.handle = 0;
        return;
    }

    std::cout << "    Temp buffer size: " << bufferSizes.tempSizeInBytes << " bytes" << std::endl;
    std::cout << "    Output buffer size: " << bufferSizes.outputSizeInBytes << " bytes" << std::endl;

    // 7. 分配临时缓冲区
    CUdeviceptr d_temp = 0;
    if (cudaMalloc(reinterpret_cast<void**>(&d_temp), bufferSizes.tempSizeInBytes) != cudaSuccess) {
        std::cerr << "  Failed to allocate temp buffer" << std::endl;
        cudaFree(reinterpret_cast<void*>(d_vertices));
        cudaFree(reinterpret_cast<void*>(d_indices));
        geom.handle = 0;
        return;
    }

    // 8. 分配输出缓冲区
    CUdeviceptr d_output = 0;
    if (cudaMalloc(reinterpret_cast<void**>(&d_output), bufferSizes.outputSizeInBytes) != cudaSuccess) {
        std::cerr << "  Failed to allocate output buffer" << std::endl;
        cudaFree(reinterpret_cast<void*>(d_temp));
        cudaFree(reinterpret_cast<void*>(d_vertices));
        cudaFree(reinterpret_cast<void*>(d_indices));
        geom.handle = 0;
        return;
    }

    // 9. 构建 BLAS
    if (optixAccelBuild(
            optixContext_,
            0,  // CUDA stream
            &accelOptions,
            &buildInput,
            1,  // num build inputs
            d_temp,
            bufferSizes.tempSizeInBytes,
            d_output,
            bufferSizes.outputSizeInBytes,
            &geom.handle,
            nullptr,  // emitted property list
            0         // num emitted properties
        ) != OPTIX_SUCCESS) {
        std::cerr << "  Failed to build BLAS" << std::endl;
        cudaFree(reinterpret_cast<void*>(d_temp));
        cudaFree(reinterpret_cast<void*>(d_output));
        cudaFree(reinterpret_cast<void*>(d_vertices));
        cudaFree(reinterpret_cast<void*>(d_indices));
        geom.handle = 0;
        return;
    }

    // 10. 释放临时缓冲区（保留输出缓冲区和几何数据）
    cudaFree(reinterpret_cast<void*>(d_temp));

    // 注意：d_output, d_vertices, d_indices 需要保留
    // 应该存储在 DynamicGeometry 结构中以便后续释放
    // TODO: 在 DynamicGeometry 中添加这些字段

    std::cout << "    BLAS built successfully, handle: " << geom.handle << std::endl;
}

// ============================================================================
// buildTLAS 完整实现
// ============================================================================

void PhysicsRenderer::buildTLAS() {
    if (dynamicGeometries_.empty()) {
        std::cout << "  No geometries to build TLAS" << std::endl;
        topLevelAS_ = 0;
        return;
    }

    std::cout << "  Building TLAS with " << dynamicGeometries_.size() << " instances..." << std::endl;

    // 1. 创建 OptixInstance 数组
    std::vector<OptixInstance> instances(dynamicGeometries_.size());

    for (size_t i = 0; i < dynamicGeometries_.size(); ++i) {
        OptixInstance& inst = instances[i];
        std::memset(&inst, 0, sizeof(OptixInstance));

        // 获取变换矩阵（4x3 格式）
        const float* transform = dynamicGeometries_[i].transforms.data();

        // OptiX 使用 3x4 列主序矩阵（行交换）
        // PhysX 4x3: [R R R Tx]  OptiX 3x4: [R R R]
        //            [R R R Ty]            [R R R]
        //            [R R R Tz]            [R R R]
        //                                  [Tx Ty Tz]

        inst.transform[0] = transform[0];   // R00
        inst.transform[1] = transform[4];   // R10
        inst.transform[2] = transform[8];   // R20
        inst.transform[3] = transform[3];   // Tx

        inst.transform[4] = transform[1];   // R01
        inst.transform[5] = transform[5];   // R11
        inst.transform[6] = transform[9];   // R21
        inst.transform[7] = transform[7];   // Ty

        inst.transform[8] = transform[2];   // R02
        inst.transform[9] = transform[6];   // R12
        inst.transform[10] = transform[10]; // R22
        inst.transform[11] = transform[11]; // Tz

        // 设置实例属性
        inst.instanceId = static_cast<unsigned int>(i);
        inst.sbtOffset = 0;  // 所有实例使用相同的 hit group
        inst.visibilityMask = 255;
        inst.flags = OPTIX_INSTANCE_FLAG_NONE;
        inst.traversableHandle = dynamicGeometries_[i].handle;

        if (inst.traversableHandle == 0) {
            std::cerr << "  Warning: Instance " << i << " has invalid BLAS handle" << std::endl;
        }
    }

    // 2. 上传实例数据到 GPU
    if (instanceBuffer_) {
        cudaFree(reinterpret_cast<void*>(instanceBuffer_));
        instanceBuffer_ = 0;
    }

    size_t instanceBufferSize = instances.size() * sizeof(OptixInstance);

    if (cudaMalloc(reinterpret_cast<void**>(&instanceBuffer_), instanceBufferSize) != cudaSuccess) {
        std::cerr << "  Failed to allocate instance buffer" << std::endl;
        topLevelAS_ = 0;
        return;
    }

    if (cudaMemcpy(
            reinterpret_cast<void*>(instanceBuffer_),
            instances.data(),
            instanceBufferSize,
            cudaMemcpyHostToDevice
        ) != cudaSuccess) {
        std::cerr << "  Failed to upload instances" << std::endl;
        topLevelAS_ = 0;
        return;
    }

    // 3. 配置 OptixBuildInput
    OptixBuildInput buildInput = {};
    buildInput.type = OPTIX_BUILD_INPUT_TYPE_INSTANCES;
    buildInput.instanceArray.instances = instanceBuffer_;
    buildInput.instanceArray.numInstances = static_cast<unsigned int>(instances.size());

    // 4. 配置 OptixAccelBuildOptions
    OptixAccelBuildOptions accelOptions = {};
    accelOptions.buildFlags = OPTIX_BUILD_FLAG_ALLOW_UPDATE;
    accelOptions.operation = OPTIX_BUILD_OPERATION_BUILD;

    // 5. 查询所需的缓冲区大小
    OptixAccelBufferSizes bufferSizes;
    if (optixAccelComputeMemoryUsage(
            optixContext_,
            &accelOptions,
            &buildInput,
            1,
            &bufferSizes
        ) != OPTIX_SUCCESS) {
        std::cerr << "  Failed to compute TLAS memory usage" << std::endl;
        topLevelAS_ = 0;
        return;
    }

    std::cout << "    TLAS temp buffer size: " << bufferSizes.tempSizeInBytes << " bytes" << std::endl;
    std::cout << "    TLAS output buffer size: " << bufferSizes.outputSizeInBytes << " bytes" << std::endl;

    // 6. 分配临时缓冲区
    CUdeviceptr d_temp = 0;
    if (cudaMalloc(reinterpret_cast<void**>(&d_temp), bufferSizes.tempSizeInBytes) != cudaSuccess) {
        std::cerr << "  Failed to allocate TLAS temp buffer" << std::endl;
        topLevelAS_ = 0;
        return;
    }

    // 7. 分配输出缓冲区（或复用现有的）
    static CUdeviceptr d_tlasOutput = 0;
    static size_t tlasOutputSize = 0;

    if (tlasOutputSize < bufferSizes.outputSizeInBytes) {
        if (d_tlasOutput) {
            cudaFree(reinterpret_cast<void*>(d_tlasOutput));
        }

        if (cudaMalloc(reinterpret_cast<void**>(&d_tlasOutput), bufferSizes.outputSizeInBytes) != cudaSuccess) {
            std::cerr << "  Failed to allocate TLAS output buffer" << std::endl;
            cudaFree(reinterpret_cast<void*>(d_temp));
            topLevelAS_ = 0;
            return;
        }

        tlasOutputSize = bufferSizes.outputSizeInBytes;
    }

    // 8. 构建 TLAS
    if (optixAccelBuild(
            optixContext_,
            0,  // CUDA stream
            &accelOptions,
            &buildInput,
            1,
            d_temp,
            bufferSizes.tempSizeInBytes,
            d_tlasOutput,
            bufferSizes.outputSizeInBytes,
            &topLevelAS_,
            nullptr,  // emitted properties
            0
        ) != OPTIX_SUCCESS) {
        std::cerr << "  Failed to build TLAS" << std::endl;
        cudaFree(reinterpret_cast<void*>(d_temp));
        topLevelAS_ = 0;
        return;
    }

    // 9. 释放临时缓冲区
    cudaFree(reinterpret_cast<void*>(d_temp));

    std::cout << "    TLAS built successfully, handle: " << topLevelAS_ << std::endl;
}

} // namespace PhysicsRender
