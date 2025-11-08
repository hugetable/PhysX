// PhysicsRenderer.cpp - 物理渲染器完整实现

#include "rendering/PhysicsRenderer.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <optix_function_table_definition.h>
#include <optix_stubs.h>

namespace PhysicsRender {

// OptiX 日志回调
static void optixLogCallback(unsigned int level, const char* tag, const char* message, void*) {
    std::cerr << "[" << level << "][" << tag << "]: " << message << std::endl;
}

// CUDA 错误检查宏
#define CUDA_CHECK(call) \
    do { \
        cudaError_t error = call; \
        if (error != cudaSuccess) { \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ \
                      << " - " << cudaGetErrorString(error) << std::endl; \
            return false; \
        } \
    } while(0)

#define CUDA_CHECK_VOID(call) \
    do { \
        cudaError_t error = call; \
        if (error != cudaSuccess) { \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ \
                      << " - " << cudaGetErrorString(error) << std::endl; \
        } \
    } while(0)

// OptiX 错误检查宏
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

    CUDA_CHECK(cudaMalloc(&outputBuffer_, bufferSize));
    CUDA_CHECK(cudaMemset(outputBuffer_, 0, bufferSize));

    std::cout << "PhysicsRenderer initialized successfully!" << std::endl;
    return true;
}

void PhysicsRenderer::setDynamicGeometry(const std::vector<RenderableObject>& renderables) {
    // 清空现有几何
    dynamicGeometries_.clear();

    // 为每个对象创建几何
    for (const auto& obj : renderables) {
        if (obj.type == RenderableObject::RIGID_BODY) {
            DynamicGeometry geom;
            geom.geometryID = obj.geometryID;
            geom.needsRebuild = true;

            // 存储变换
            geom.transforms.resize(12);
            std::memcpy(geom.transforms.data(), obj.transform, 12 * sizeof(float));

            // 构建 BLAS（如果需要）
            buildBLAS(obj, geom);

            dynamicGeometries_.push_back(geom);
        }
    }

    // 重建 TLAS
    buildTLAS();
}

void PhysicsRenderer::updateTransforms(const std::vector<float*>& transforms) {
    if (transforms.empty() || transforms.size() != dynamicGeometries_.size()) {
        return;
    }

    // 更新实例变换矩阵
    for (size_t i = 0; i < transforms.size(); ++i) {
        std::memcpy(dynamicGeometries_[i].transforms.data(),
                    transforms[i],
                    12 * sizeof(float));
    }

    // 重建 TLAS（使用新的变换）
    buildTLAS();
}

void PhysicsRenderer::rebuildTLAS() {
    buildTLAS();
}

void PhysicsRenderer::rebuildParticleGeometry(
    CUdeviceptr positions,
    CUdeviceptr radii,
    int count
) {
    particleGeom_.positionsBuffer = positions;
    particleGeom_.radiiBuffer = radii;
    particleGeom_.count = count;
    particleGeom_.handle = 0;

    // TODO: 构建粒子 BLAS（sphere 原语）
    // 需要使用 OPTIX_BUILD_INPUT_TYPE_SPHERES
}

void PhysicsRenderer::updateParticlePositions(CUdeviceptr positions, int count) {
    particleGeom_.positionsBuffer = positions;
    particleGeom_.count = count;

    // TODO: 更新粒子几何（使用 OPTIX_BUILD_FLAG_ALLOW_UPDATE）
}

int PhysicsRenderer::addPhysicsMaterial(const PhysicsMaterialDefinition& material) {
    int materialID = static_cast<int>(physicsMaterials_.size());
    physicsMaterials_.push_back(material);

    // 重新分配并上传材质缓冲区
    if (materialBuffer_) {
        CUDA_CHECK_VOID(cudaFree(reinterpret_cast<void*>(materialBuffer_)));
    }

    size_t materialBufferSize = physicsMaterials_.size() * sizeof(PhysicsMaterialDefinition);
    CUDA_CHECK_VOID(cudaMalloc(&materialBuffer_, materialBufferSize));
    CUDA_CHECK_VOID(cudaMemcpy(
        reinterpret_cast<void*>(materialBuffer_),
        physicsMaterials_.data(),
        materialBufferSize,
        cudaMemcpyHostToDevice
    ));

    return materialID;
}

void PhysicsRenderer::updatePhysicsMaterial(
    int materialID,
    const PhysicsMaterialDefinition& material
) {
    if (materialID >= 0 && materialID < static_cast<int>(physicsMaterials_.size())) {
        physicsMaterials_[materialID] = material;

        // 更新 GPU 材质数据
        if (materialBuffer_) {
            CUDA_CHECK_VOID(cudaMemcpy(
                reinterpret_cast<void*>(materialBuffer_ + materialID * sizeof(PhysicsMaterialDefinition)),
                &material,
                sizeof(PhysicsMaterialDefinition),
                cudaMemcpyHostToDevice
            ));
        }
    }
}

unsigned int PhysicsRenderer::render() {
    if (!optixContext_ || !pipeline_ || topLevelAS_ == 0) {
        std::cerr << "OptiX not properly initialized!" << std::endl;
        return 0;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // 准备系统数据
    PhysicsSystemData sysData;
    std::memset(&sysData, 0, sizeof(sysData));

    sysData.topObject = topLevelAS_;
    sysData.outputBuffer = outputBuffer_;
    sysData.resolution = config_.resolution;
    sysData.pathLengths = make_int2(2, config_.maxPathLength);
    sysData.sceneEpsilon = config_.sceneEpsilon;
    sysData.iterationIndex = 0;
    sysData.samplesSqrt = 2; // 4 samples per pixel
    sysData.materialDefinitions = reinterpret_cast<PhysicsMaterialDefinition*>(materialBuffer_);
    sysData.numMaterials = static_cast<int>(physicsMaterials_.size());

    // TODO: 设置相机和光源数据
    sysData.numCameras = 0;
    sysData.numLights = 0;

    // 上传系统数据到常量内存
    // 注意：这需要在着色器中定义 __constant__ PhysicsSystemData sysData;
    // 实际上，我们需要通过 launch params 传递

    // 执行光线追踪
    OPTIX_CHECK(optixLaunch(
        pipeline_,
        0,  // stream
        reinterpret_cast<CUdeviceptr>(&sysData),
        sizeof(PhysicsSystemData),
        &sbt_,
        config_.resolution.x,
        config_.resolution.y,
        1  // depth
    ));

    // 等待完成
    CUDA_CHECK(cudaDeviceSynchronize());

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    return static_cast<unsigned int>(duration.count());
}

// 私有辅助函数实现

bool PhysicsRenderer::createOptixContext() {
    std::cout << "  Creating OptiX context..." << std::endl;

    // 1. 初始化 CUDA
    CUDA_CHECK(cudaFree(0));  // 初始化 CUDA 运行时

    // 2. 初始化 OptiX
    OPTIX_CHECK(optixInit());

    // 3. 创建 OptiX 设备上下文
    CUcontext cuCtx = 0;  // 0 表示使用当前上下文

    OptixDeviceContextOptions options = {};
    options.logCallbackFunction = &optixLogCallback;
    options.logCallbackLevel = 4;  // 打印所有日志

    OPTIX_CHECK(optixDeviceContextCreate(cuCtx, &options, &optixContext_));

    std::cout << "  OptiX context created successfully" << std::endl;
    return true;
}

bool PhysicsRenderer::createModule() {
    std::cout << "  Creating OptiX module..." << std::endl;

    // 1. 加载 PTX/OptiX-IR 文件
    // 这里我们假设着色器已经编译到特定位置
    std::string ptxFilename = std::string(MODULE_TARGET_DIR) + "/raygeneration_core.ptx";

    std::ifstream ptxFile(ptxFilename, std::ios::binary);
    if (!ptxFile.is_open()) {
        std::cerr << "Failed to open PTX file: " << ptxFilename << std::endl;
        // 继续，但警告
        std::cerr << "Warning: Using empty PTX (module creation will fail)" << std::endl;
    }

    std::string ptxSource;
    if (ptxFile.is_open()) {
        ptxFile.seekg(0, std::ios::end);
        ptxSource.resize(ptxFile.tellg());
        ptxFile.seekg(0, std::ios::beg);
        ptxFile.read(&ptxSource[0], ptxSource.size());
        ptxFile.close();
    }

    // 2. 模块编译选项
    OptixModuleCompileOptions moduleCompileOptions = {};
    moduleCompileOptions.maxRegisterCount = OPTIX_COMPILE_DEFAULT_MAX_REGISTER_COUNT;
    moduleCompileOptions.optLevel = OPTIX_COMPILE_OPTIMIZATION_DEFAULT;
    moduleCompileOptions.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_MODERATE;

    // 3. 管线编译选项
    OptixPipelineCompileOptions pipelineCompileOptions = {};
    pipelineCompileOptions.usesMotionBlur = false;
    pipelineCompileOptions.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_LEVEL_INSTANCING;
    pipelineCompileOptions.numPayloadValues = 8;  // 3 (radiance) + 3 (throughput) + 1 (depth) + 1 (seed)
    pipelineCompileOptions.numAttributeValues = 3;  // 法线
    pipelineCompileOptions.exceptionFlags = OPTIX_EXCEPTION_FLAG_NONE;
    pipelineCompileOptions.pipelineLaunchParamsVariableName = "sysData";

    // 4. 创建模块
    char log[2048];
    size_t logSize = sizeof(log);

    if (!ptxSource.empty()) {
        OPTIX_CHECK(optixModuleCreateFromPTX(
            optixContext_,
            &moduleCompileOptions,
            &pipelineCompileOptions,
            ptxSource.c_str(),
            ptxSource.size(),
            log,
            &logSize,
            &optixModule_
        ));

        if (logSize > 1) {
            std::cout << "  Module creation log: " << log << std::endl;
        }
    } else {
        std::cerr << "  Warning: PTX source is empty, skipping module creation" << std::endl;
        optixModule_ = nullptr;
        return true;  // 暂时返回 true 以便继续测试其他部分
    }

    std::cout << "  OptiX module created successfully" << std::endl;
    return true;
}

bool PhysicsRenderer::createPipeline() {
    std::cout << "  Creating OptiX pipeline..." << std::endl;

    if (!optixModule_) {
        std::cerr << "  Warning: No module available, skipping pipeline creation" << std::endl;
        return true;
    }

    // 1. 创建程序组

    // Raygen 程序组
    OptixProgramGroupOptions pgOptions = {};
    OptixProgramGroupDesc raygenPGDesc = {};
    raygenPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
    raygenPGDesc.raygen.module = optixModule_;
    raygenPGDesc.raygen.entryFunctionName = "__raygen__pinhole_camera";

    char log[2048];
    size_t logSize = sizeof(log);

    OPTIX_CHECK(optixProgramGroupCreate(
        optixContext_,
        &raygenPGDesc,
        1,
        &pgOptions,
        log,
        &logSize,
        &raygenPG_
    ));

    // Miss 程序组
    OptixProgramGroupDesc missPGDesc = {};
    missPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
    missPGDesc.miss.module = optixModule_;
    missPGDesc.miss.entryFunctionName = "__miss__env_constant";

    OPTIX_CHECK(optixProgramGroupCreate(
        optixContext_,
        &missPGDesc,
        1,
        &pgOptions,
        log,
        &logSize,
        &missPG_
    ));

    // Hit 程序组
    OptixProgramGroupDesc hitPGDesc = {};
    hitPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    hitPGDesc.hitgroup.moduleCH = optixModule_;
    hitPGDesc.hitgroup.entryFunctionNameCH = "__closesthit__physics_radiance";
    hitPGDesc.hitgroup.moduleAH = nullptr;  // 暂时不使用 anyhit
    hitPGDesc.hitgroup.entryFunctionNameAH = nullptr;

    OPTIX_CHECK(optixProgramGroupCreate(
        optixContext_,
        &hitPGDesc,
        1,
        &pgOptions,
        log,
        &logSize,
        &hitPG_
    ));

    // 2. 创建管线
    OptixProgramGroup programGroups[] = { raygenPG_, missPG_, hitPG_ };

    OptixPipelineLinkOptions pipelineLinkOptions = {};
    pipelineLinkOptions.maxTraceDepth = config_.maxPathLength;
    pipelineLinkOptions.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_MODERATE;

    OPTIX_CHECK(optixPipelineCreate(
        optixContext_,
        nullptr,  // 使用 module 中的编译选项
        &pipelineLinkOptions,
        programGroups,
        3,
        log,
        &logSize,
        &pipeline_
    ));

    // 3. 设置栈大小
    OptixStackSizes stackSizes = {};
    for (auto pg : programGroups) {
        OPTIX_CHECK(optixUtilAccumulateStackSizes(pg, &stackSizes));
    }

    uint32_t maxTraversableGraphDepth = 2;
    uint32_t maxContinuationStackSize;
    uint32_t directStackSize;

    OPTIX_CHECK(optixUtilComputeStackSizes(
        &stackSizes,
        config_.maxPathLength,
        maxContinuationStackSize,
        maxTraversableGraphDepth,
        &directStackSize,
        &maxContinuationStackSize
    ));

    OPTIX_CHECK(optixPipelineSetStackSize(
        pipeline_,
        directStackSize,
        maxContinuationStackSize,
        maxContinuationStackSize,
        maxTraversableGraphDepth
    ));

    std::cout << "  OptiX pipeline created successfully" << std::endl;
    return true;
}

bool PhysicsRenderer::createSBT() {
    std::cout << "  Creating Shader Binding Table..." << std::endl;

    if (!pipeline_ || !raygenPG_ || !missPG_ || !hitPG_) {
        std::cerr << "  Error: Pipeline or program groups not available" << std::endl;
        return false;
    }

    // SBT 记录结构（对齐到 OPTIX_SBT_RECORD_ALIGNMENT）
    template<typename T>
    struct alignas(OPTIX_SBT_RECORD_ALIGNMENT) SbtRecord {
        char header[OPTIX_SBT_RECORD_HEADER_SIZE];
        T data;
    };

    // 1. Raygen 记录
    using RaygenRecord = SbtRecord<int>;  // 简化：不需要额外数据
    RaygenRecord raygenRecord;
    OPTIX_CHECK(optixSbtRecordPackHeader(raygenPG_, &raygenRecord));
    raygenRecord.data = 0;  // placeholder

    CUdeviceptr d_raygenRecord;
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_raygenRecord), sizeof(RaygenRecord)));
    CUDA_CHECK(cudaMemcpy(
        reinterpret_cast<void*>(d_raygenRecord),
        &raygenRecord,
        sizeof(RaygenRecord),
        cudaMemcpyHostToDevice
    ));

    // 2. Miss 记录
    using MissRecord = SbtRecord<float3>;  // 背景颜色
    MissRecord missRecord;
    OPTIX_CHECK(optixSbtRecordPackHeader(missPG_, &missRecord));
    missRecord.data = make_float3(0.1f, 0.1f, 0.15f);  // 深蓝色背景

    CUdeviceptr d_missRecord;
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_missRecord), sizeof(MissRecord)));
    CUDA_CHECK(cudaMemcpy(
        reinterpret_cast<void*>(d_missRecord),
        &missRecord,
        sizeof(MissRecord),
        cudaMemcpyHostToDevice
    ));

    // 3. Hit 记录
    struct HitGroupData {
        int materialID;
        int geometryID;
    };
    using HitRecord = SbtRecord<HitGroupData>;

    HitRecord hitRecord;
    OPTIX_CHECK(optixSbtRecordPackHeader(hitPG_, &hitRecord));
    hitRecord.data.materialID = 0;  // 默认材质
    hitRecord.data.geometryID = 0;  // 默认几何

    CUdeviceptr d_hitRecord;
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_hitRecord), sizeof(HitRecord)));
    CUDA_CHECK(cudaMemcpy(
        reinterpret_cast<void*>(d_hitRecord),
        &hitRecord,
        sizeof(HitRecord),
        cudaMemcpyHostToDevice
    ));

    // 4. 配置 SBT
    sbt_.raygenRecord = d_raygenRecord;

    sbt_.missRecordBase = d_missRecord;
    sbt_.missRecordStrideInBytes = sizeof(MissRecord);
    sbt_.missRecordCount = 1;

    sbt_.hitgroupRecordBase = d_hitRecord;
    sbt_.hitgroupRecordStrideInBytes = sizeof(HitRecord);
    sbt_.hitgroupRecordCount = 1;

    std::cout << "  Shader Binding Table created successfully" << std::endl;
    return true;
}

// buildBLAS(), updateBLAS(), buildTLAS() 实现在 PhysicsRenderer_AS.cpp 中

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
